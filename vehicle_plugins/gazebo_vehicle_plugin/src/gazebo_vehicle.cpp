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
    (void)sdf;
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

    tf_br = std::make_unique<tf2_ros::TransformBroadcaster>(node);

    // Initialize parameters
    initParams();

    // ROS Publishers
    // Odometry
    odometry_pub = node->create_publisher<nav_msgs::msg::Odometry>("odometry", 1);
    gt_odometry_pub = node->create_publisher<nav_msgs::msg::Odometry>("odometry/ground_truth", 1);

    // RVIZ joint visuals
    joint_state_pub = node->create_publisher<sensor_msgs::msg::JointState>("joint_states/steering", 1);

    // ROS Subscriptions
    ackermann_cmd_sub = node->create_subscription<ackermann_msgs::msg::AckermannDriveStamped>(
        "control/ackermann_cmd", 1, std::bind(&VehiclePlugin::onAckermannCmd, this, std::placeholders::_1));
    twist_cmd_sub = node->create_subscription<geometry_msgs::msg::Twist>(
        "control/twist_cmd", 1, std::bind(&VehiclePlugin::onTwistCmd, this, std::placeholders::_1));

    reset_vehicle_pos_srv = node->create_service<std_srvs::srv::Trigger>(
        "reset_vehicle",
        std::bind(&VehiclePlugin::resetVehiclePosition, this, std::placeholders::_1, std::placeholders::_2));

    max_steering_rate = (vehicle_model->getParam().input_ranges.delta.max - 
                        vehicle_model->getParam().input_ranges.delta.min) / steering_lock_time;

    // Initialize times
    last_sim_time = std::chrono::steady_clock::duration::zero();
    last_cmd_time = std::chrono::steady_clock::duration::zero();
    last_published_time = std::chrono::steady_clock::duration::zero();

    // Initialize command
    last_cmd.drive.steering_angle = 0;
    last_cmd.drive.acceleration = -100;
    last_cmd.drive.speed = 0;

    RCLCPP_INFO(node->get_logger(), "GZ Sim VehiclePlugin Configured");
}

void VehiclePlugin::initParams() {
    // Get ROS parameters
    update_rate = node->declare_parameter("update_rate", 2.0);
    publish_rate = node->declare_parameter("publish_rate", 50.0);
    map_frame = node->declare_parameter("map_frame", "map");
    odom_frame = node->declare_parameter("odom_frame", "odom");
    base_frame = node->declare_parameter("base_frame", "base_link");
    control_delay = node->declare_parameter("control_delay", 0.5);
    steering_lock_time = node->declare_parameter("steering_lock_time", 1.0);

    // Vehicle model
    std::string vehicle_yaml_name = node->declare_parameter("vehicle_params", "null");
    if (vehicle_yaml_name == "null") {
        RCLCPP_FATAL(node->get_logger(), 
                    "gazebo_vehicle plugin missing <vehicle_params> parameter, cannot proceed");
        exit(1);
    }
    vehicle_model = std::make_unique<VehicleModelBike>(vehicle_yaml_name);
    motion_noise = std::make_unique<Noise>(vehicle_yaml_name);
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
        last_published_time = info.simTime;
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

    state_odom.header.frame_id = odom_frame;
    state_odom.child_frame_id = base_frame;
    state_odom.pose.pose.position.x = offset.Pos().X();
    state_odom.pose.pose.position.y = offset.Pos().Y();
    state_odom.pose.pose.position.z = offset.Pos().Z();

    state_odom.pose.pose.orientation.x = offset.Rot().X();
    state_odom.pose.pose.orientation.y = offset.Rot().Y();
    state_odom.pose.pose.orientation.z = offset.Rot().Z();
    state_odom.pose.pose.orientation.w = offset.Rot().W();

    state_odom.twist.twist.linear.x = 0.0;
    state_odom.twist.twist.linear.y = 0.0;
    state_odom.twist.twist.linear.z = 0.0;

    state_odom.twist.twist.angular.x = 0.0;
    state_odom.twist.twist.angular.y = 0.0;
    state_odom.twist.twist.angular.z = 0.0;

    state = odomToState(state_odom);
}

bool VehiclePlugin::resetVehiclePosition(
    std::shared_ptr<std_srvs::srv::Trigger::Request>,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response) 
{
    state_odom.header.frame_id = odom_frame;
    state_odom.child_frame_id = base_frame;
    state_odom.pose.pose.position.x = offset.Pos().X();
    state_odom.pose.pose.position.y = offset.Pos().Y();
    state_odom.pose.pose.position.z = offset.Pos().Z();

    state_odom.pose.pose.orientation.x = offset.Rot().X();
    state_odom.pose.pose.orientation.y = offset.Rot().Y();
    state_odom.pose.pose.orientation.z = offset.Rot().Z();
    state_odom.pose.pose.orientation.w = offset.Rot().W();

    state_odom.twist.twist.linear.x = 0.0;
    state_odom.twist.twist.linear.y = 0.0;
    state_odom.twist.twist.linear.z = 0.0;

    state_odom.twist.twist.angular.x = 0.0;
    state_odom.twist.twist.angular.y = 0.0;
    state_odom.twist.twist.angular.z = 0.0;

    last_cmd.drive.steering_angle = 0;
    last_cmd.drive.speed = -1;

    state = odomToState(state_odom);

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

void VehiclePlugin::publishVehicleOdom() {
    // Publish ground truth
    if (gt_odometry_pub->get_subscription_count() > 0) {
        gt_odometry_pub->publish(state_odom);
    }

    // Apply noise and publish
    nav_msgs::msg::Odometry odom_noisy = motion_noise->applyNoise(state_odom);
    if (odometry_pub->get_subscription_count() > 0) {
        odometry_pub->publish(odom_noisy);
    }
}

void VehiclePlugin::publishTf() {
    // Base->Odom/Map
    tf2::Transform base_to_odom;
    base_to_odom.setOrigin(tf2::Vector3(state_odom.pose.pose.position.x, 
                                       state_odom.pose.pose.position.y, 0.0));

    // Orientation
    tf2::Quaternion base_odom_q;
    tf2::convert(state_odom.pose.pose.orientation, base_odom_q);
    base_to_odom.setRotation(base_odom_q);

    // Send TF
    geometry_msgs::msg::TransformStamped transform_stamped;
    transform_stamped.header.stamp = state_odom.header.stamp;
    transform_stamped.header.frame_id = map_frame;
    transform_stamped.child_frame_id = base_frame;
    tf2::convert(base_to_odom, transform_stamped.transform);

    tf_br->sendTransform(transform_stamped);
}

nav_msgs::msg::Odometry VehiclePlugin::stateToOdom(const State &state) {
    nav_msgs::msg::Odometry msg;
    msg.header.stamp = node->now();
    msg.header.frame_id = odom_frame;
    msg.child_frame_id = base_frame;

    msg.pose.pose.position.x = state.x;
    msg.pose.pose.position.y = state.y;

    std::vector<double> orientation = {0.0, 0.0, state.yaw};
    msg.pose.pose.orientation = to_quaternion(orientation);

    msg.twist.twist.linear.x = state.v_x;
    msg.twist.twist.linear.y = state.v_y;
    msg.twist.twist.angular.z = state.r_z;

    return msg;
}

State VehiclePlugin::odomToState(const nav_msgs::msg::Odometry &odom) {
    State state;
    state.x = odom.pose.pose.position.x;
    state.y = odom.pose.pose.position.y;
    geometry_msgs::msg::Quaternion q = odom.pose.pose.orientation;
    state.yaw = to_euler(q)[2];
    state.v_x = odom.twist.twist.linear.x;
    state.v_y = odom.twist.twist.linear.y;
    state.r_z = odom.twist.twist.angular.z;

    return state;
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

    input.acceleration = last_cmd.drive.acceleration;
    input.velocity = last_cmd.drive.speed;
    input.steering = last_cmd.drive.steering_angle * M_PI / 180.0; // Convert to radians
    // 90* (max steering angle) = 16* (max wheel angle)
    input.steering *= (16.0 / 90.0);

    double current_speed = std::sqrt(std::pow(state_odom.twist.twist.linear.x, 2) + 
                                    std::pow(state_odom.twist.twist.linear.y, 2));
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

    // Publish joint states
    sensor_msgs::msg::JointState joint_state;
    joint_state.header.stamp = node->now();
    joint_state.name.push_back("left_steering_hinge_joint");
    joint_state.name.push_back("right_steering_hinge_joint");
    joint_state.position.push_back(output.steering);
    joint_state.position.push_back(output.steering);
    joint_state_pub->publish(joint_state);

    setModelState(ecm);

    // Check publish rate
    auto time_since_last_published_duration = info.simTime - last_published_time;
    double time_since_last_published = std::chrono::duration<double>(time_since_last_published_duration).count();
    
    if (time_since_last_published < (1.0 / publish_rate)) {
        return;
    }
    last_published_time = info.simTime;

    state_odom = stateToOdom(state);

    // Publish car states
    publishVehicleOdom();
    publishTf();
}

void VehiclePlugin::onAckermannCmd(const ackermann_msgs::msg::AckermannDriveStamped::SharedPtr msg) {
    auto time_since_cmd_duration = last_sim_time - last_cmd_time;
    double time_since_cmd = std::chrono::duration<double>(time_since_cmd_duration).count();
    
    RCLCPP_DEBUG(node->get_logger(), "Time since last cmd: %f", time_since_cmd);
    
    // Simple control delay - just wait if needed
    if (time_since_cmd < control_delay) {
        return;
    }
    
    last_cmd.drive.acceleration = msg->drive.acceleration;
    last_cmd.drive.speed = msg->drive.speed;
    last_cmd.drive.steering_angle = msg->drive.steering_angle;
    last_cmd_time = last_sim_time;
}

void VehiclePlugin::onTwistCmd(const geometry_msgs::msg::Twist::SharedPtr msg) {
    auto time_since_cmd_duration = last_sim_time - last_cmd_time;
    double time_since_cmd = std::chrono::duration<double>(time_since_cmd_duration).count();
    
    RCLCPP_DEBUG(node->get_logger(), "Time since last cmd: %f", time_since_cmd);
    
    if (time_since_cmd < control_delay) {
        return;
    }
    
    if (msg->linear.x > 0) {
        last_cmd.drive.speed = msg->linear.x;
    } else if (msg->linear.x < 0) {
        last_cmd.drive.speed = -1;
        last_cmd.drive.steering_angle = 0;
    }
    last_cmd.drive.steering_angle += msg->angular.z;

    RCLCPP_INFO(node->get_logger(), "Steering Angle: %f", last_cmd.drive.steering_angle);

    last_cmd_time = last_sim_time;
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
