#include "gazebo_vehicle_control_plugin/vehicle_control.hpp"

namespace gazebo_plugins {
namespace vehicle_plugins {

VehicleControlPlugin::VehicleControlPlugin() 
    : last_cmd_time(std::chrono::steady_clock::duration::zero()),
      last_sim_time(std::chrono::steady_clock::duration::zero()),
      first_update(true) {}

VehicleControlPlugin::~VehicleControlPlugin() {}

void VehicleControlPlugin::Configure(
    const gz::sim::Entity &entity,
    const std::shared_ptr<const sdf::Element> &sdf,
    gz::sim::EntityComponentManager &ecm,
    gz::sim::EventManager &eventMgr)
{
    // Currently unused
    (void)eventMgr;

    // Store entity and model
    this->entity_ = entity;
    this->model_ = gz::sim::Model(entity);
    
    // Error checking
    if (!this->model_.Valid(ecm)) {
        RCLCPP_FATAL(rclcpp::get_logger("vehicle_control_plugin"), 
                    "VehicleControl Plugin should be attached to a model entity. Failed to initialize.");
        return;
    }

    // Ensure that ROS2 is initialized
    if (!rclcpp::ok()) {
        rclcpp::init(0, nullptr);
    }

    this->node = std::make_shared<rclcpp::Node>("vehicle_control_plugin_node");
    RCLCPP_INFO(this->node->get_logger(), "Created ROS node inside VehicleControlPlugin.");

    // Load parameters from SDF
    loadParameters(sdf);

    // Create ROS subscriptions based on configuration
    if (enable_ackermann) {
        ackermann_cmd_sub = node->create_subscription<ackermann_msgs::msg::AckermannDriveStamped>(
            "control/ackermann_cmd", 1, 
            std::bind(&VehicleControlPlugin::onAckermannCmd, this, std::placeholders::_1));
        RCLCPP_INFO(node->get_logger(), "Ackermann command subscription enabled");
    }

    if (enable_twist) {
        twist_cmd_sub = node->create_subscription<geometry_msgs::msg::Twist>(
            "control/twist_cmd", 1, 
            std::bind(&VehicleControlPlugin::onTwistCmd, this, std::placeholders::_1));
        RCLCPP_INFO(node->get_logger(), "Twist command subscription enabled");
    }

    // Initialize control data with safe defaults
    control_data.velocity = 0.0;
    control_data.acceleration = -100.0;  // Strong braking by default
    control_data.steering = 0.0;
    control_data.timestamp = 0.0;

    // Create the VehicleControlInput component on this entity
    if (!ecm.Component<gz::sim::components::VehicleControlInput>(entity_)) {
        ecm.CreateComponent(entity_, gz::sim::components::VehicleControlInput(control_data));
        RCLCPP_INFO(node->get_logger(), "Created VehicleControlInput ECM component");
    }

    RCLCPP_INFO(this->node->get_logger(), 
                "Vehicle Control Plugin initialized - control_delay=%.3f s",
                control_delay);
}

void VehicleControlPlugin::loadParameters(const std::shared_ptr<const sdf::Element> &sdf) {
    // Get parameters from SDF with defaults
    control_delay = sdf->Get<double>("control_delay", 0.035).first;
    enable_ackermann = sdf->Get<bool>("enable_ackermann", true).first;
    enable_twist = sdf->Get<bool>("enable_twist", true).first;
    
    RCLCPP_INFO(node->get_logger(), 
                "Control plugin params: control_delay=%.3f, ackermann=%s, twist=%s", 
                control_delay, 
                enable_ackermann ? "enabled" : "disabled",
                enable_twist ? "enabled" : "disabled");
}

void VehicleControlPlugin::PreUpdate(const gz::sim::UpdateInfo &info,
                                      gz::sim::EntityComponentManager &ecm)
{
    // Skip if paused
    if (info.paused) {
        return;
    }

    if (first_update) {
        last_sim_time = info.simTime;
        first_update = false;
        return;
    }

    last_sim_time = info.simTime;

    // Update control component in ECM with latest command data
    auto *control_comp = ecm.Component<gz::sim::components::VehicleControlInput>(entity_);
    if (control_comp) {
        // Update timestamp with current sim time
        control_data.timestamp = std::chrono::duration<double>(info.simTime).count();
        *control_comp = gz::sim::components::VehicleControlInput(control_data);
    } else {
        RCLCPP_WARN_THROTTLE(node->get_logger(), *node->get_clock(), 1000,
                            "VehicleControlInput component not found on entity");
    }

    // Spin ROS node to process callbacks
    rclcpp::spin_some(node);
}

void VehicleControlPlugin::onAckermannCmd(const ackermann_msgs::msg::AckermannDriveStamped::SharedPtr msg) {
    auto time_since_cmd_duration = last_sim_time - last_cmd_time;
    double time_since_cmd = std::chrono::duration<double>(time_since_cmd_duration).count();
    
    RCLCPP_DEBUG(node->get_logger(), "Ackermann cmd received - time since last: %.3f s", time_since_cmd);
    
    // Simple control delay - just wait if needed
    if (time_since_cmd < control_delay) {
        return;
    }
    
    // Update control data from Ackermann message
    control_data.acceleration = msg->drive.acceleration;
    control_data.velocity = msg->drive.speed;
    control_data.steering = msg->drive.steering_angle;  // Store in degrees, will convert in vehicle plugin
    
    last_cmd_time = last_sim_time;
    
    RCLCPP_DEBUG(node->get_logger(), "Control updated: v=%.2f, a=%.2f, steer=%.2f deg", 
                control_data.velocity, control_data.acceleration, control_data.steering);
}

void VehicleControlPlugin::onTwistCmd(const geometry_msgs::msg::Twist::SharedPtr msg) {
    auto time_since_cmd_duration = last_sim_time - last_cmd_time;
    double time_since_cmd = std::chrono::duration<double>(time_since_cmd_duration).count();
    
    RCLCPP_DEBUG(node->get_logger(), "Twist cmd received - time since last: %.3f s", time_since_cmd);
    
    if (time_since_cmd < control_delay) {
        return;
    }
    
    // Convert Twist to control data
    if (msg->linear.x > 0) {
        control_data.velocity = msg->linear.x;
    } else if (msg->linear.x < 0) {
        control_data.velocity = -1;
        control_data.steering = 0;
    }
    control_data.steering += msg->angular.z;

    RCLCPP_INFO(node->get_logger(), "Twist control - steering angle: %.2f", control_data.steering);

    last_cmd_time = last_sim_time;
}

}  // namespace vehicle_plugins
}  // namespace gazebo_plugins

// Register the plugin
#include <gz/plugin/Register.hh>
GZ_ADD_PLUGIN(
    gazebo_plugins::vehicle_plugins::VehicleControlPlugin,
    gz::sim::System,
    gazebo_plugins::vehicle_plugins::VehicleControlPlugin::ISystemConfigure,
    gazebo_plugins::vehicle_plugins::VehicleControlPlugin::ISystemPreUpdate)
