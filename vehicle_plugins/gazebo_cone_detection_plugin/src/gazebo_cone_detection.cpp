#include "gazebo_cone_detection_plugin/gazebo_cone_detection.hpp"

#include <gz/sim/Util.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>
#include <gz/sim/components/Link.hh>
#include <gz/sim/components/LinearVelocity.hh>
#include <gz/sim/components/AngularVelocity.hh>
#include <gz/sim/components/Model.hh>

namespace gazebo_plugins {
namespace vehicle_plugins {

ConeDetectionPlugin::ConeDetectionPlugin() : first_update(true) {}

ConeDetectionPlugin::~ConeDetectionPlugin() {}

void ConeDetectionPlugin::Configure(
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
        RCLCPP_ERROR(rclcpp::get_logger("cone_detection_plugin"), "Invalid model entity. Plugin won't run.");
        return;
    }

    // Ensure that ROS2 is initialized
    if (!rclcpp::ok()) {
        rclcpp::init(0, nullptr);
    }

    this->node = std::make_shared<rclcpp::Node>("cone_detection_plugin_node");
    RCLCPP_INFO(this->node->get_logger(), "Created ROS node inside ConeDetectionPlugin.");

    // Initialize parameters from SDF
    initParams(sdf);

    // Publishers
    track_pub = node->create_publisher<driverless_msgs::msg::ConeDetectionStamped>(("track/ground_truth"), 1);
    detection_pub = node->create_publisher<driverless_msgs::msg::ConeDetectionStamped>(("detections/ground_truth"), 1);

    track_marker_pub = node->create_publisher<visualization_msgs::msg::MarkerArray>(("visuals/track_markers"), 1);
    detection_marker_pub = node->create_publisher<visualization_msgs::msg::MarkerArray>(("visuals/detection_markers"), 1);

    // Initialize times
    last_track_update = std::chrono::steady_clock::duration::zero();
    last_detection_update = std::chrono::steady_clock::duration::zero();

    // Visuals
    initMarkers();

    RCLCPP_INFO(node->get_logger(), "GZ Sim ConeDetectionPlugin Configured");
}

void ConeDetectionPlugin::initParams(const std::shared_ptr<const sdf::Element> &sdf) {
    // Read parameters from SDF (plugin configuration in URDF)
    map_frame = sdf->Get<std::string>("map_frame", "track").first;
    base_frame = sdf->Get<std::string>("base_frame", "base_footprint").first;
    
    detection_update_rate = sdf->Get<double>("lidar_update_rate", 10.0).first;
    track_update_rate = sdf->Get<double>("track_update_rate", 20.0).first;

    detection_config = {
        sdf->Get<std::string>("lidar_frame_id", "velodyne").first,
        sdf->Get<double>("lidar_min_view_distance", 1.0).first,
        sdf->Get<double>("lidar_max_view_distance", 20.0).first,
        sdf->Get<double>("lidar_fov", 3.141593).first,
        sdf->Get<bool>("lidar_detects_colour", false).first,
        sdf->Get<double>("lidar_offset_x", 1.59).first,
        sdf->Get<double>("lidar_offset_z", 0.25).first
    };
    
    RCLCPP_INFO(node->get_logger(), "Cone detection plugin params loaded: lidar_fov=%.2f, max_dist=%.1f", 
                detection_config.fov, detection_config.max_view_distance);
}

void ConeDetectionPlugin::PreUpdate(const gz::sim::UpdateInfo &info,
                                     gz::sim::EntityComponentManager &ecm)
{
    // First update - find track and car link entities
    if (first_update) {
        // Find track model by name
        ecm.Each<gz::sim::components::Model, gz::sim::components::Name>(
            [&](const gz::sim::Entity &entity,
                const gz::sim::components::Model *,
                const gz::sim::components::Name *name) -> bool {
                if (name->Data() == "track") {
                    track_model = entity;
                    RCLCPP_INFO(node->get_logger(), "Found track model entity: %lu", track_model);
                    return false; // Stop searching
                }
                return true; // Continue searching
            });

        if (track_model == gz::sim::kNullEntity) {
            RCLCPP_FATAL(node->get_logger(), "Could not find required model 'track'. Exiting.");
            return;
        }

        // Find car link
        car_link = _model.LinkByName(ecm, base_frame);
        if (car_link == gz::sim::kNullEntity) {
            RCLCPP_FATAL(node->get_logger(), 
                        "Could not find required link <%s> on model. Exiting.", base_frame.c_str());
            return;
        }

        // Get initial car world pose (global coordinates)
        car_initial_pose = gz::sim::worldPose(car_link, ecm);

        first_update = false;
        last_track_update = info.simTime;
        last_detection_update = info.simTime;
        
        RCLCPP_INFO(node->get_logger(), "ConeDetectionPlugin initialized");
        return;
    }

    // Spin ROS callbacks
    rclcpp::spin_some(node);

    update(info, ecm);
}

void ConeDetectionPlugin::update(const gz::sim::UpdateInfo &info, 
                                 gz::sim::EntityComponentManager &ecm) 
{
    auto curr_time = info.simTime;
    
    // Get ground truth track (shared by both track and detection publishing)
    auto ground_truth_track = get_ground_truth_track(ecm, track_model, curr_time, map_frame);
    
    // Publish track at configured rate
    auto dt_track_duration = curr_time - last_track_update;
    double dt_track = std::chrono::duration<double>(dt_track_duration).count();
    
    if (dt_track >= (1.0 / track_update_rate)) {
        last_track_update = curr_time;
        
        if (has_subscribers(track_pub)) {
            auto centered_ground_truth = get_track_centered_on_car_initial_pose(car_initial_pose, ground_truth_track);
            track_pub->publish(centered_ground_truth);
        }
        if (has_subscribers(track_marker_pub)) {
            auto centered_ground_truth = get_track_centered_on_car_initial_pose(car_initial_pose, ground_truth_track);
            publishMarkerArray(centered_ground_truth, true);
        }
    }

    // Publish detection at configured rate (independent of track rate)
    auto dt_detection_duration = curr_time - last_detection_update;
    double dt_detection = std::chrono::duration<double>(dt_detection_duration).count();
    
    if (dt_detection >= (1.0 / detection_update_rate)) {
        last_detection_update = curr_time;
        
        // Get current car world pose (global coordinates)
        gz::math::Pose3d car_pose = gz::sim::worldPose(car_link, ecm);

        if (has_subscribers(detection_pub)) {
            auto lidar_detection = get_sensor_detection(detection_config, car_pose, ground_truth_track);
            detection_pub->publish(lidar_detection);
        }
        if (has_subscribers(detection_marker_pub)) {
            auto lidar_detection = get_sensor_detection(detection_config, car_pose, ground_truth_track);
            publishMarkerArray(lidar_detection, false);
        }
    }
}

// VISUALS

void ConeDetectionPlugin::initMarkers() {
    delete_all_marker.action = visualization_msgs::msg::Marker::DELETEALL;

    blue_cone_marker.action = visualization_msgs::msg::Marker::ADD;
    blue_cone_marker.type = visualization_msgs::msg::Marker::CYLINDER;
    blue_cone_marker.pose.orientation.x = 0.0;
    blue_cone_marker.pose.orientation.y = 0.0;
    blue_cone_marker.pose.orientation.z = 0.0;
    blue_cone_marker.pose.orientation.w = 1.0;
    blue_cone_marker.scale.x = 0.2;
    blue_cone_marker.scale.y = 0.2;
    blue_cone_marker.scale.z = 0.3;
    blue_cone_marker.color.r = 0.0;
    blue_cone_marker.color.g = 0.0;
    blue_cone_marker.color.b = 1.0;
    blue_cone_marker.color.a = 1.0;
    blue_cone_marker.ns = "cone";

    yellow_cone_marker.action = visualization_msgs::msg::Marker::ADD;
    yellow_cone_marker.type = visualization_msgs::msg::Marker::CYLINDER;
    yellow_cone_marker.pose.orientation.x = 0.0;
    yellow_cone_marker.pose.orientation.y = 0.0;
    yellow_cone_marker.pose.orientation.z = 0.0;
    yellow_cone_marker.pose.orientation.w = 1.0;
    yellow_cone_marker.scale.x = 0.2;
    yellow_cone_marker.scale.y = 0.2;
    yellow_cone_marker.scale.z = 0.3;
    yellow_cone_marker.color.r = 1.0;
    yellow_cone_marker.color.g = 1.0;
    yellow_cone_marker.color.b = 0.0;
    yellow_cone_marker.color.a = 1.0;
    yellow_cone_marker.ns = "cone";

    orange_cone_marker.action = visualization_msgs::msg::Marker::ADD;
    orange_cone_marker.type = visualization_msgs::msg::Marker::CYLINDER;
    orange_cone_marker.pose.orientation.x = 0.0;
    orange_cone_marker.pose.orientation.y = 0.0;
    orange_cone_marker.pose.orientation.z = 0.0;
    orange_cone_marker.pose.orientation.w = 1.0;
    orange_cone_marker.scale.x = 0.2;
    orange_cone_marker.scale.y = 0.2;
    orange_cone_marker.scale.z = 0.3;
    orange_cone_marker.color.r = 1.0;
    orange_cone_marker.color.g = 0.549;
    orange_cone_marker.color.b = 0.0;
    orange_cone_marker.color.a = 1.0;
    orange_cone_marker.ns = "cone";

    big_orange_cone_marker.action = visualization_msgs::msg::Marker::ADD;
    big_orange_cone_marker.type = visualization_msgs::msg::Marker::CYLINDER;
    big_orange_cone_marker.pose.orientation.x = 0.0;
    big_orange_cone_marker.pose.orientation.y = 0.0;
    big_orange_cone_marker.pose.orientation.z = 0.0;
    big_orange_cone_marker.pose.orientation.w = 1.0;
    big_orange_cone_marker.scale.x = 0.2;
    big_orange_cone_marker.scale.y = 0.2;
    big_orange_cone_marker.scale.z = 0.45;
    big_orange_cone_marker.color.r = 1.0;
    big_orange_cone_marker.color.g = 0.271;
    big_orange_cone_marker.color.b = 0.0;
    big_orange_cone_marker.color.a = 1.0;
    big_orange_cone_marker.ns = "cone";

    unknown_cone_marker.action = visualization_msgs::msg::Marker::ADD;
    unknown_cone_marker.type = visualization_msgs::msg::Marker::CYLINDER;
    unknown_cone_marker.pose.orientation.x = 0.0;
    unknown_cone_marker.pose.orientation.y = 0.0;
    unknown_cone_marker.pose.orientation.z = 0.0;
    unknown_cone_marker.pose.orientation.w = 1.0;
    unknown_cone_marker.scale.x = 0.2;
    unknown_cone_marker.scale.y = 0.2;
    unknown_cone_marker.scale.z = 0.3;
    unknown_cone_marker.color.r = 0.0;
    unknown_cone_marker.color.g = 1.0;
    unknown_cone_marker.color.b = 0.0;
    unknown_cone_marker.color.a = 1.0;
    unknown_cone_marker.ns = "cone";
}

void ConeDetectionPlugin::setConeMarker(const driverless_msgs::msg::Cone &cone,
                                        const std_msgs::msg::Header &header, const int &id,
                                        visualization_msgs::msg::Marker *marker) {
    marker->id = id;
    marker->header = header;
    marker->pose.position.x = cone.location.x;
    marker->pose.position.y = cone.location.y;
    marker->pose.position.z = cone.location.z;
}

void ConeDetectionPlugin::publishMarkerArray(driverless_msgs::msg::ConeDetectionStamped msg, bool is_track) {
    delete_all_marker.header = msg.header;
    delete_all_marker.id = id;
    marker_array.markers.push_back(delete_all_marker);

    if (is_track) {
        track_marker_pub->publish(marker_array);
    } else {
        detection_marker_pub->publish(marker_array);
    }
    marker_array.markers.clear();

    for (const auto &cone : msg.cones) {
        visualization_msgs::msg::Marker cone_marker;
        switch (cone.color) {
            case driverless_msgs::msg::Cone::BLUE:
                cone_marker = blue_cone_marker;
                break;
            case driverless_msgs::msg::Cone::YELLOW:
                cone_marker = yellow_cone_marker;
                break;
            case driverless_msgs::msg::Cone::ORANGE_BIG:
                cone_marker = big_orange_cone_marker;
                break;
            case driverless_msgs::msg::Cone::ORANGE_SMALL:
                cone_marker = orange_cone_marker;
                break;
            default:
                cone_marker = unknown_cone_marker;
                break;
        }
        setConeMarker(cone, msg.header, id, &cone_marker);
        marker_array.markers.push_back(cone_marker);
        id++;
    }

    if (is_track) {
        track_marker_pub->publish(marker_array);
    } else {
        detection_marker_pub->publish(marker_array);
    }

    marker_array.markers.clear();
}

}  // namespace vehicle_plugins
}  // namespace gazebo_plugins

// Register the plugin
#include <gz/plugin/Register.hh>
GZ_ADD_PLUGIN(
    gazebo_plugins::vehicle_plugins::ConeDetectionPlugin,
    gz::sim::System,
    gazebo_plugins::vehicle_plugins::ConeDetectionPlugin::ISystemConfigure,
    gazebo_plugins::vehicle_plugins::ConeDetectionPlugin::ISystemPreUpdate)
