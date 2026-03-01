#ifndef VEHICLE_PLUGINS_GAZEBO_TF_BROADCASTER_PLUGIN_INCLUDE_GAZEBO_TF_BROADCASTER_PLUGIN_TF_BROADCASTER_HPP_
#define VEHICLE_PLUGINS_GAZEBO_TF_BROADCASTER_PLUGIN_INCLUDE_GAZEBO_TF_BROADCASTER_PLUGIN_TF_BROADCASTER_HPP_

#include <memory>
#include <string>

// ROS Includes
#include "rclcpp/rclcpp.hpp"

// ROS TF2
#include <tf2/transform_datatypes.h>
#include <tf2_ros/transform_broadcaster.h>
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

// GZ Sim Includes
#include <gz/plugin/Register.hh>
#include <gz/sim/System.hh>
#include <gz/sim/Model.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/Types.hh>

// GZ Math
#include <gz/math/Pose3.hh>

namespace gazebo_plugins {
namespace vehicle_plugins {

/**
 * @brief TF Broadcaster Plugin
 * 
 * Publishes TF transforms for the vehicle based on its pose in the simulation.
 * Reads vehicle state from ECM components and broadcasts transforms to ROS TF tree.
 * 
 * This plugin is independent of vehicle dynamics and sensor plugins, making it
 * easy to enable/disable TF broadcasting or customize frame relationships.
 * 
 * Expected ECM Components (read by this plugin):
 * - gz::sim::components::Pose - Vehicle pose in world frame
 * 
 * Published TF Transforms:
 * - map_frame -> base_frame (vehicle position/orientation)
 * 
 * SDF Parameters:
 * - <publish_rate> - TF publish rate in Hz (default: 50.0)
 * - <map_frame> - Parent frame ID (default: "track")
 * - <odom_frame> - Odometry frame ID (default: "track", can be same as map)
 * - <base_frame> - Vehicle base frame ID (default: "base_footprint")
 * - <publish_odom_tf> - Publish odom->base_link TF (default: false, use map->base)
 */
class TFBroadcasterPlugin : public gz::sim::System,
                            public gz::sim::ISystemConfigure,
                            public gz::sim::ISystemPostUpdate {
public:
    TFBroadcasterPlugin();
    ~TFBroadcasterPlugin() override;

    // GZ Sim System interface
    void Configure(const gz::sim::Entity &entity,
                   const std::shared_ptr<const sdf::Element> &sdf,
                   gz::sim::EntityComponentManager &ecm,
                   gz::sim::EventManager &eventMgr) override;

    void PostUpdate(const gz::sim::UpdateInfo &info,
                    const gz::sim::EntityComponentManager &ecm) override;

private:
    void loadParameters(const std::shared_ptr<const sdf::Element> &sdf);
    void publishTransforms(const gz::sim::UpdateInfo &info,
                          const gz::sim::EntityComponentManager &ecm);

    // ROS node
    std::shared_ptr<rclcpp::Node> node;

    // TF Broadcaster
    std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster;

    // GZ Sim
    gz::sim::Entity entity_;
    gz::sim::Model model_;

    // Configuration
    double publish_rate;
    std::string map_frame;
    std::string odom_frame;
    std::string base_frame;
    bool publish_odom_tf;

    // Timing
    std::chrono::steady_clock::duration last_published_time;
    bool first_update;
};

}  // namespace vehicle_plugins
}  // namespace gazebo_plugins

#endif  // VEHICLE_PLUGINS_GAZEBO_TF_BROADCASTER_PLUGIN_INCLUDE_GAZEBO_TF_BROADCASTER_PLUGIN_TF_BROADCASTER_HPP_
