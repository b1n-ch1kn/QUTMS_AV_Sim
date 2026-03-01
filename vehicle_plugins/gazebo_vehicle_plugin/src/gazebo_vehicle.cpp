#include "gazebo_vehicle_plugin/gazebo_vehicle.hpp"

#include <gz/sim/components/AngularVelocity.hh>
#include <gz/sim/components/Joint.hh>
#include <gz/sim/components/JointPosition.hh>
#include <gz/sim/components/LinearVelocity.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>
#include <gz/sim/components/World.hh>

namespace gazebo_plugins {
namespace vehicle_plugins {

VehiclePlugin::VehiclePlugin() : first_update(true) {}

VehiclePlugin::~VehiclePlugin() {}

void VehiclePlugin::Configure(
    const gz::sim::Entity &entity,
    const std::shared_ptr<const sdf::Element> &sdf,
    gz::sim::EntityComponentManager &ecm,
    gz::sim::EventManager &eventMgr)
{
    // Currently unused
    (void)eventMgr;

    // Storage for later
    this->_entity = entity;
    this->_model = gz::sim::Model(entity);
    
    // Error checking
    if (!this->_model.Valid(ecm)) {
        RCLCPP_ERROR(rclcpp::get_logger("vehicle_plugin"), "Invalid model entity. Plugin won't run.");
        return;
    }

    // Ensure that ROS2 is initialized
    if (!rclcpp::ok()) {
        rclcpp::init(0, nullptr);
    }

    this->node = std::make_shared<rclcpp::Node>("vehicle_plugin_node");
    RCLCPP_INFO(this->node->get_logger(), "Created ROS node inside VehiclePlugin.");

    // Initialize parameters from SDF
    initParams(sdf);

    // ROS Publishers
    // RVIZ joint visuals
    joint_state_pub = node->create_publisher<sensor_msgs::msg::JointState>("joint_states/steering", 1);

    reset_vehicle_pos_srv = node->create_service<std_srvs::srv::Trigger>(
        "reset_vehicle",
        std::bind(&VehiclePlugin::resetVehiclePosition, this, std::placeholders::_1, std::placeholders::_2));

    max_steering_rate = (vehicle_model->getParam().input_ranges.delta.max - 
                        vehicle_model->getParam().input_ranges.delta.min) / steering_lock_time;

    // Initialize times
    last_sim_time = std::chrono::steady_clock::duration::zero();

    RCLCPP_INFO(node->get_logger(), "GZ Sim VehiclePlugin Configured");
}

void VehiclePlugin::initParams(const std::shared_ptr<const sdf::Element> &sdf) {
    // Read parameters from SDF (plugin configuration in URDF)
    if (sdf->HasElement("vehicle_params")) {
        std::string vehicle_yaml_name = sdf->Get<std::string>("vehicle_params");
        RCLCPP_INFO(node->get_logger(), "Loading vehicle params from: %s", vehicle_yaml_name.c_str());
        vehicle_model = std::make_unique<VehicleModelBike>(vehicle_yaml_name);
    } else {
        RCLCPP_FATAL(node->get_logger(), 
                    "gazebo_vehicle plugin missing <vehicle_params> parameter in URDF, cannot proceed");
        exit(1);
    }
    
    // Get other parameters from SDF with defaults
    update_rate = sdf->Get<double>("update_rate", 50.0).first;
    steering_lock_time = sdf->Get<double>("steering_lock_time", 1.5).first;
    
    RCLCPP_INFO(node->get_logger(), "Vehicle plugin params loaded: update_rate=%.1f", update_rate);
}

void VehiclePlugin::PreUpdate(const gz::sim::UpdateInfo &info,
                               gz::sim::EntityComponentManager &ecm)
{
    // First update - set initial position
    if (first_update) {
        setPositionFromWorld(ecm);
        
        // Get steering joints
        left_steering_joint = _model.JointByName(ecm, "left_steering_hinge_joint");
        right_steering_joint = _model.JointByName(ecm, "right_steering_hinge_joint");
        
        first_update = false;
        last_sim_time = info.simTime;
        return;
    }

    // Spin ROS callbacks
    rclcpp::spin_some(node);

    update(info, ecm);
}

void VehiclePlugin::setPositionFromWorld(gz::sim::EntityComponentManager &ecm) {
    // Get initial pose from simulation
    auto pose = ecm.Component<gz::sim::components::Pose>(_entity);
    if (pose) {
        offset = pose->Data();
        car_initial_pose = offset;
    }

    RCLCPP_DEBUG(node->get_logger(), "Got starting offset %f %f %f", 
                offset.Pos().X(), offset.Pos().Y(), offset.Pos().Z());

    // Initialize state directly from pose
    state.x = offset.Pos().X();
    state.y = offset.Pos().Y();
    state.z = offset.Pos().Z();
    state.yaw = offset.Rot().Yaw();
    state.v_x = 0.0;
    state.v_y = 0.0;
    state.v_z = 0.0;
    state.r_x = 0.0;
    state.r_y = 0.0;
    state.r_z = 0.0;
}

bool VehiclePlugin::resetVehiclePosition(
    std::shared_ptr<std_srvs::srv::Trigger::Request>,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response) 
{
    // Reset state directly to initial pose
    state.x = offset.Pos().X();
    state.y = offset.Pos().Y();
    state.z = offset.Pos().Z();
    state.yaw = offset.Rot().Yaw();
    state.v_x = 0.0;
    state.v_y = 0.0;
    state.v_z = 0.0;
    state.r_x = 0.0;
    state.r_y = 0.0;
    state.r_z = 0.0;
    
    // Note: Control commands are now handled by the vehicle_control_plugin
    // Vehicle will stop naturally when no commands are received

    // NOTE: In GZ Sim, setting pose/velocity requires WorldPoseCmd and related components
    // This will be handled in the update loop
    
    response->success = true;
    return true;
}

void VehiclePlugin::setModelState(gz::sim::EntityComponentManager &ecm) {
    double yaw = state.yaw + offset.Rot().Yaw();

    double x = offset.Pos().X() + state.x * cos(offset.Rot().Yaw()) - 
               state.y * sin(offset.Rot().Yaw());
    double y = offset.Pos().Y() + state.x * sin(offset.Rot().Yaw()) + 
               state.y * cos(offset.Rot().Yaw());
    double z = state.z;

    double vx = state.v_x * cos(yaw) - state.v_y * sin(yaw);
    double vy = state.v_x * sin(yaw) + state.v_y * cos(yaw);

    const gz::math::Pose3d pose(x, y, z, 0, 0.0, yaw);
    const gz::math::Vector3d vel(vx, vy, 0.0);
    const gz::math::Vector3d angular(0.0, 0.0, state.r_z);

    // Set pose using ECM
    auto poseComp = ecm.Component<gz::sim::components::Pose>(_entity);
    if (poseComp) {
        *poseComp = gz::sim::components::Pose(pose);
    } else {
        ecm.CreateComponent(_entity, gz::sim::components::Pose(pose));
    }

    // Set linear velocity
    auto linVelComp = ecm.Component<gz::sim::components::LinearVelocity>(_entity);
    if (linVelComp) {
        *linVelComp = gz::sim::components::LinearVelocity(vel);
    } else {
        ecm.CreateComponent(_entity, gz::sim::components::LinearVelocity(vel));
    }

    // Set angular velocity
    auto angVelComp = ecm.Component<gz::sim::components::AngularVelocity>(_entity);
    if (angVelComp) {
        *angVelComp = gz::sim::components::AngularVelocity(angular);
    } else {
        ecm.CreateComponent(_entity, gz::sim::components::AngularVelocity(angular));
    }
}

void VehiclePlugin::update(const gz::sim::UpdateInfo &info, 
                           gz::sim::EntityComponentManager &ecm) 
{
    // Check against update rate
    auto dt_duration = info.simTime - last_sim_time;
    double dt = std::chrono::duration<double>(dt_duration).count();
    
    if (dt < (1.0 / update_rate)) {
        return;
    }

    last_sim_time = info.simTime;

    // Read control inputs from ECM (provided by vehicle_control_plugin)
    auto controlData = ecm.Component<gz::sim::components::VehicleControlInput>(_entity);
    if (controlData) {
        input.acceleration = controlData->Data().acceleration;
        input.velocity = controlData->Data().velocity;
        input.steering = controlData->Data().steering; // Already in radians
    } else {
        // No control data available - use safe defaults
        input.acceleration = -100.0;
        input.velocity = 0.0;
        input.steering = 0.0;
    }

    double current_speed = std::sqrt(std::pow(state.v_x, 2) + std::pow(state.v_y, 2));
    output.acceleration = (input.velocity - current_speed) / dt;

    // Make sure steering rate is within limits
    output.steering += (input.steering - output.steering >= 0 ? 1 : -1) *
                        std::min(max_steering_rate * dt, std::abs(input.steering - output.steering));

    // Ensure vehicle can drive
    if (input.velocity < 0) {
        output.acceleration = -100.0;
        output.velocity = 0.0;
        output.steering = 0.0;
    }

    // Update z value from simulation
    auto pose = ecm.Component<gz::sim::components::Pose>(_entity);
    if (pose) {
        state.z = pose->Data().Pos().Z();
    }

    // Update state
    vehicle_model->updateState(state, output, dt);

    // Set steering joint positions
    if (left_steering_joint != gz::sim::kNullEntity) {
        auto leftJointPos = ecm.Component<gz::sim::components::JointPosition>(left_steering_joint);
        if (!leftJointPos) {
            ecm.CreateComponent(left_steering_joint, 
                              gz::sim::components::JointPosition(std::vector<double>{output.steering}));
        } else {
            leftJointPos->Data()[0] = output.steering;
        }
    }

    if (right_steering_joint != gz::sim::kNullEntity) {
        auto rightJointPos = ecm.Component<gz::sim::components::JointPosition>(right_steering_joint);
        if (!rightJointPos) {
            ecm.CreateComponent(right_steering_joint, 
                              gz::sim::components::JointPosition(std::vector<double>{output.steering}));
        } else {
            rightJointPos->Data()[0] = output.steering;
        }
    }

    // Convert sim time to ROS time
    rclcpp::Time current_time(std::chrono::duration_cast<std::chrono::nanoseconds>(info.simTime).count());
    
    // Publish joint states
    sensor_msgs::msg::JointState joint_state;
    joint_state.header.stamp = current_time;
    joint_state.name.push_back("left_steering_hinge_joint");
    joint_state.name.push_back("right_steering_hinge_joint");
    joint_state.position.push_back(output.steering);
    joint_state.position.push_back(output.steering);
    joint_state_pub->publish(joint_state);

    setModelState(ecm);
}



}  // namespace vehicle_plugins
}  // namespace gazebo_plugins

// Register the plugin
#include <gz/plugin/Register.hh>
GZ_ADD_PLUGIN(
    gazebo_plugins::vehicle_plugins::VehiclePlugin,
    gz::sim::System,
    gazebo_plugins::vehicle_plugins::VehiclePlugin::ISystemConfigure,
    gazebo_plugins::vehicle_plugins::VehiclePlugin::ISystemPreUpdate)
