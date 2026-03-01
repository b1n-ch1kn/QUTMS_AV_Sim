#ifndef VEHICLE_PLUGINS_GAZEBO_VEHICLE_CONTROL_PLUGIN_INCLUDE_GAZEBO_VEHICLE_CONTROL_PLUGIN_VEHICLE_CONTROL_HPP_
#define VEHICLE_PLUGINS_GAZEBO_VEHICLE_CONTROL_PLUGIN_INCLUDE_GAZEBO_VEHICLE_CONTROL_PLUGIN_VEHICLE_CONTROL_HPP_

#include <memory>
#include <string>

// ROS Includes
#include "rclcpp/rclcpp.hpp"

// ROS msgs
#include "ackermann_msgs/msg/ackermann_drive_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"

// GZ Sim Includes
#include <gz/plugin/Register.hh>
#include <gz/sim/System.hh>
#include <gz/sim/Model.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/Types.hh>

// Local includes
#include "gazebo_vehicle_control_plugin/vehicle_control_component.hpp"

namespace gazebo_plugins {
namespace vehicle_plugins {

/**
 * @brief Vehicle Control Plugin
 * 
 * Handles ROS control input topics (Ackermann and Twist) and writes control
 * commands to ECM VehicleControlInput component for the vehicle dynamics plugin.
 * 
 * This plugin can be enabled/disabled by adding/removing it from the URDF
 * without modifying the core vehicle dynamics plugin.
 * 
 * ECM Components (written by this plugin):
 * - VehicleControlInput - Control commands (velocity, acceleration, steering)
 * 
 * Subscribed Topics:
 * - /sim/control/ackermann_cmd (ackermann_msgs/AckermannDriveStamped)
 * - /sim/control/twist_cmd (geometry_msgs/Twist)
 * 
 * SDF Parameters:
 * - <control_delay> - Command delay in seconds (default: 0.035)
 * - <enable_ackermann> - Enable Ackermann command input (default: true)
 * - <enable_twist> - Enable Twist command input (default: true)
 */
class VehicleControlPlugin : public gz::sim::System,
                              public gz::sim::ISystemConfigure,
                              public gz::sim::ISystemPreUpdate {
public:
    VehicleControlPlugin();
    ~VehicleControlPlugin() override;

    // GZ Sim System interface
    void Configure(const gz::sim::Entity &entity,
                   const std::shared_ptr<const sdf::Element> &sdf,
                   gz::sim::EntityComponentManager &ecm,
                   gz::sim::EventManager &eventMgr) override;

    void PreUpdate(const gz::sim::UpdateInfo &info,
                   gz::sim::EntityComponentManager &ecm) override;

private:
    void loadParameters(const std::shared_ptr<const sdf::Element> &sdf);
    void onAckermannCmd(const ackermann_msgs::msg::AckermannDriveStamped::SharedPtr msg);
    void onTwistCmd(const geometry_msgs::msg::Twist::SharedPtr msg);

    // ROS node
    std::shared_ptr<rclcpp::Node> node;

    // ROS Subscriptions
    rclcpp::Subscription<ackermann_msgs::msg::AckermannDriveStamped>::SharedPtr ackermann_cmd_sub;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr twist_cmd_sub;

    // GZ Sim
    gz::sim::Entity entity_;
    gz::sim::Model model_;

    // Configuration
    double control_delay;
    bool enable_ackermann;
    bool enable_twist;

    // Control state
    VehicleControlData control_data;
    std::chrono::steady_clock::duration last_cmd_time;
    std::chrono::steady_clock::duration last_sim_time;
    bool first_update;
};

}  // namespace vehicle_plugins
}  // namespace gazebo_plugins

#endif  // VEHICLE_PLUGINS_GAZEBO_VEHICLE_CONTROL_PLUGIN_INCLUDE_GAZEBO_VEHICLE_CONTROL_PLUGIN_VEHICLE_CONTROL_HPP_
