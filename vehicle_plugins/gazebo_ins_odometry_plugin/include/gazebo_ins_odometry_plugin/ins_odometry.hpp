#ifndef VEHICLE_PLUGINS_GAZEBO_INS_ODOMETRY_PLUGIN_INCLUDE_GAZEBO_INS_ODOMETRY_PLUGIN_INS_ODOMETRY_HPP_
#define VEHICLE_PLUGINS_GAZEBO_INS_ODOMETRY_PLUGIN_INCLUDE_GAZEBO_INS_ODOMETRY_PLUGIN_INS_ODOMETRY_HPP_

#include <memory>
#include <string>

// ROS Includes
#include "rclcpp/rclcpp.hpp"

// ROS msgs
#include "nav_msgs/msg/odometry.hpp"

// GZ Sim Includes
#include <gz/plugin/Register.hh>
#include <gz/sim/System.hh>
#include <gz/sim/Model.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/Types.hh>

// GZ Math
#include <gz/math/Pose3.hh>
#include <gz/math/Vector3.hh>

// Local includes (shared from vehicle plugin)
#include "gazebo_vehicle_plugin/noise.hpp"

namespace gazebo_plugins {
namespace vehicle_plugins {

/**
 * @brief INS Odometry Plugin
 * 
 * Simulated INS (Inertial Navigation System) sensor that reads vehicle state
 * from ECM components, applies realistic sensor noise, and publishes noisy
 * odometry data to ROS.
 * 
 * This plugin can be enabled/disabled by adding/removing it from the URDF
 * without modifying the core vehicle dynamics plugin.
 * 
 * Expected ECM Components (read by this plugin):
 * - gz::sim::components::Pose - Vehicle pose in world frame
 * - gz::sim::components::LinearVelocity - Linear velocity
 * - gz::sim::components::AngularVelocity - Angular velocity
 * 
 * Published Topics:
 * - /sim/odometry (nav_msgs/Odometry) - Noisy odometry data
 * - /sim/odometry/ground_truth (nav_msgs/Odometry) - Perfect odometry (if enabled)
 * 
 * SDF Parameters:
 * - <noise_params> - Path to YAML file with noise configuration
 * - <publish_rate> - Publish rate in Hz (default: 50.0)
 * - <odom_frame> - Odometry frame ID (default: "track")
 * - <base_frame> - Base frame ID (default: "base_footprint")
 * - <topic_name> - Noisy odometry topic (default: "odometry")
 * - <ground_truth_topic_name> - GT topic (default: "odometry/ground_truth")
 * - <enable_ground_truth> - Publish ground truth (default: true)
 */
class INSOdometryPlugin : public gz::sim::System,
                          public gz::sim::ISystemConfigure,
                          public gz::sim::ISystemPostUpdate {
public:
    INSOdometryPlugin();
    ~INSOdometryPlugin() override;

    // GZ Sim System interface
    void Configure(const gz::sim::Entity &entity,
                   const std::shared_ptr<const sdf::Element> &sdf,
                   gz::sim::EntityComponentManager &ecm,
                   gz::sim::EventManager &eventMgr) override;

    void PostUpdate(const gz::sim::UpdateInfo &info,
                    const gz::sim::EntityComponentManager &ecm) override;

private:
    void loadParameters(const std::shared_ptr<const sdf::Element> &sdf);
    void publishOdometry(const gz::sim::UpdateInfo &info,
                        const gz::sim::EntityComponentManager &ecm);

    // ROS node
    std::shared_ptr<rclcpp::Node> node;

    // ROS Publishers
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odometry_pub;
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr ground_truth_pub;

    // Noise model
    std::unique_ptr<Noise> motion_noise;

    // GZ Sim
    gz::sim::Entity entity_;
    gz::sim::Model model_;

    // Configuration
    double publish_rate;
    std::string odom_frame;
    std::string base_frame;
    std::string topic_name;
    std::string ground_truth_topic_name;
    bool enable_ground_truth;

    // Timing
    std::chrono::steady_clock::duration last_published_time;
};

}  // namespace vehicle_plugins
}  // namespace gazebo_plugins

#endif  // VEHICLE_PLUGINS_GAZEBO_INS_ODOMETRY_PLUGIN_INCLUDE_GAZEBO_INS_ODOMETRY_PLUGIN_INS_ODOMETRY_HPP_
