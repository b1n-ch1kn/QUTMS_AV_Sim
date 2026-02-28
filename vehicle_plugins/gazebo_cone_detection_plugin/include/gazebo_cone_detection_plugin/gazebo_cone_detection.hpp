#ifndef VEHICLE_PLUGINS_GAZEBO_VEHICLE_PLUGIN_INCLUDE_GAZEBO_CONE_DETECTION_PLUGIN_GAZEBO_CONE_DETECTION_HPP_
#define VEHICLE_PLUGINS_GAZEBO_VEHICLE_PLUGIN_INCLUDE_GAZEBO_CONE_DETECTION_PLUGIN_GAZEBO_CONE_DETECTION_HPP_

#include <rclcpp/rclcpp.hpp>

// GZ Sim Includes
#include <gz/plugin/Register.hh>
#include <gz/sim/System.hh>
#include <gz/sim/Model.hh>
#include <gz/sim/World.hh>
#include <gz/sim/Util.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/Types.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/Link.hh>
#include <gz/transport/Node.hh>

// GZ Math
#include <gz/math/Pose3.hh>
#include <gz/math/Vector3.hh>

// ROS msgs
#include <visualization_msgs/msg/marker.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

// ROS srvs
#include <std_srvs/srv/trigger.hpp>

// QUTMS messages
#include <driverless_msgs/msg/cone.hpp>
#include <driverless_msgs/msg/cone_detection_stamped.hpp>

#include "gazebo_cone_detection_plugin/utils.hpp"

namespace gazebo_plugins {
namespace vehicle_plugins {

enum ConeColorOption { CONE = 0, FLAT = 1 };

class ConeDetectionPlugin : public gz::sim::System,
                             public gz::sim::ISystemConfigure,
                             public gz::sim::ISystemPreUpdate {
   public:
    ConeDetectionPlugin();
    ~ConeDetectionPlugin() override;

    // GZ Sim System interface
    void Configure(const gz::sim::Entity &entity,
                   const std::shared_ptr<const sdf::Element> &sdf,
                   gz::sim::EntityComponentManager &ecm,
                   gz::sim::EventManager &eventMgr) override;

    void PreUpdate(const gz::sim::UpdateInfo &info,
                   gz::sim::EntityComponentManager &ecm) override;

   private:
    void update(const gz::sim::UpdateInfo &info, gz::sim::EntityComponentManager &ecm);
    void initParams(const std::shared_ptr<const sdf::Element> &sdf);
    bool resetConePosition(std::shared_ptr<std_srvs::srv::Trigger::Request> request,
                           std::shared_ptr<std_srvs::srv::Trigger::Response> response);

    // GZ Sim
    gz::sim::Entity _entity;
    gz::sim::Model _model;
    gz::sim::Entity _world_entity;
    std::shared_ptr<rclcpp::Node> node;

    std::string map_frame;
    std::string base_frame;

    gz::sim::Entity track_model;
    gz::sim::Entity car_link;
    gz::math::Pose3d car_initial_pose;

    double track_update_rate;
    double detection_update_rate;
    std::chrono::steady_clock::duration last_track_update;
    std::chrono::steady_clock::duration last_detection_update;
    driverless_msgs::msg::ConeDetectionStamped initial_track;

    // ROS Publishers
    rclcpp::Publisher<driverless_msgs::msg::ConeDetectionStamped>::SharedPtr track_pub;
    rclcpp::Publisher<driverless_msgs::msg::ConeDetectionStamped>::SharedPtr detection_pub;
    rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr track_marker_pub;
    rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr detection_marker_pub;

    // ROS Services
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr reset_cone_pos_srv;

    SensorConfig_t detection_config;

    // Visuals
    void initMarkers();
    void setConeMarker(const driverless_msgs::msg::Cone &cone, const std_msgs::msg::Header &header, const int &id,
                       visualization_msgs::msg::Marker *marker);
    void publishMarkerArray(driverless_msgs::msg::ConeDetectionStamped msg, bool is_track);

    int id;
    ConeColorOption cone_color_option;

    visualization_msgs::msg::Marker blue_cone_marker;
    visualization_msgs::msg::Marker yellow_cone_marker;
    visualization_msgs::msg::Marker orange_cone_marker;
    visualization_msgs::msg::Marker big_orange_cone_marker;
    visualization_msgs::msg::Marker unknown_cone_marker;
    visualization_msgs::msg::Marker covariance_marker;
    visualization_msgs::msg::Marker delete_all_marker;
    visualization_msgs::msg::MarkerArray marker_array;
    
    bool first_update;
    
    // Store ECM for reset
    gz::sim::EntityComponentManager* ecm_ptr;
};

}  // namespace vehicle_plugins
}  // namespace gazebo_plugins

#endif  // VEHICLE_PLUGINS_GAZEBO_VEHICLE_PLUGIN_INCLUDE_GAZEBO_CONE_DETECTION_PLUGIN_GAZEBO_CONE_DETECTION_HPP_