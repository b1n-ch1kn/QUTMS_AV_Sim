#pragma once

#include <driverless_msgs/msg/cone.hpp>
#include <driverless_msgs/msg/cone_detection_stamped.hpp>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/Model.hh>
#include <gz/sim/Link.hh>
#include <gz/sim/Util.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>
#include <gz/sim/components/Link.hh>
#include <gz/sim/components/Model.hh>
#include <gz/math/Pose3.hh>
#include <string>
#include <chrono>

namespace gazebo_plugins {
namespace vehicle_plugins {

typedef struct SensorConfig {
    std::string frame_id;
    double min_view_distance;
    double max_view_distance;
    double fov;
    bool detects_colour;
    double offset_x;
    double offset_z;
} SensorConfig_t;

bool is_initalised(rclcpp::PublisherBase::SharedPtr publisher) { return (bool)publisher; }

bool has_subscribers(rclcpp::PublisherBase::SharedPtr publisher) {
    return is_initalised(publisher) && publisher->get_subscription_count() > 0;
}

double cone_dist(driverless_msgs::msg::Cone cone) {
    return sqrt(cone.location.x * cone.location.x + cone.location.y * cone.location.y);
}

double cone_angle(driverless_msgs::msg::Cone cone) { return atan2(cone.location.y, cone.location.x); }

// Translate a cone to the car's frame from the world frame
driverless_msgs::msg::Cone convert_cone_to_car_frame(const gz::math::Pose3d car_pose,
                                                     const driverless_msgs::msg::Cone cone, const double offset_x = 0,
                                                     const double offset_z = 0) {
    driverless_msgs::msg::Cone translated_cone = cone;

    double x = cone.location.x - car_pose.Pos().X();
    double y = cone.location.y - car_pose.Pos().Y();
    double yaw = car_pose.Rot().Yaw();

    // Rotate the points using the yaw of the car (x and y are the other way around)
    translated_cone.location.y = (cos(yaw) * y) - (sin(yaw) * x);
    translated_cone.location.x = (sin(yaw) * y) + (cos(yaw) * x) - offset_x;
    translated_cone.location.z -= offset_z;

    return translated_cone;
}

// Calculate the distance of a cone from the car
driverless_msgs::msg::ConeDetectionStamped get_sensor_detection(SensorConfig_t sensor_config, gz::math::Pose3d car_pose, 
                                                                driverless_msgs::msg::ConeDetectionStamped ground_truth_track) {

    driverless_msgs::msg::ConeDetectionStamped detection;
    detection.header = ground_truth_track.header;
    detection.header.frame_id = sensor_config.frame_id;

    for (auto const &cone : ground_truth_track.cones) {
        auto translated_cone = cone;
        translated_cone = convert_cone_to_car_frame(car_pose, cone, sensor_config.offset_x, sensor_config.offset_z);

        double dist = cone_dist(translated_cone);
        if (dist < sensor_config.min_view_distance || dist > sensor_config.max_view_distance) {
            continue;
        }

        double angle = cone_angle(translated_cone);
        if (abs(angle) > sensor_config.fov / 2) {
            continue;
        }

        if (!sensor_config.detects_colour) {
            translated_cone.color = driverless_msgs::msg::Cone::UNKNOWN;
        }

        detection.cones.push_back(translated_cone);
    }

    return detection;
}

// Get the track centered on the car's initial pose
// Used to update visuals when cones are hit
driverless_msgs::msg::ConeDetectionStamped get_track_centered_on_car_initial_pose(gz::math::Pose3d car_initial_pose, 
                                                                                 driverless_msgs::msg::ConeDetectionStamped track) {
    driverless_msgs::msg::ConeDetectionStamped centered_track;
    centered_track.header = track.header;

    for (auto const &cone : track.cones) {
        auto translated_cone = cone;
        translated_cone = convert_cone_to_car_frame(car_initial_pose, cone);
        centered_track.cones.push_back(translated_cone);
    }

    return centered_track;
}

// Get the position of a cone from a link entity using ECM
driverless_msgs::msg::Cone get_cone_from_link(gz::sim::EntityComponentManager &ecm, gz::sim::Entity link_entity) {
    driverless_msgs::msg::Cone cone;
    
    // Get world pose using utility function (global coordinates)
    auto pose = gz::sim::worldPose(link_entity, ecm);
    cone.location.x = pose.Pos().X();
    cone.location.y = pose.Pos().Y();
    cone.location.z = 0.15;
    
    cone.color = driverless_msgs::msg::Cone::UNKNOWN;

    // Get name from ECM
    auto nameComp = ecm.Component<gz::sim::components::Name>(link_entity);
    if (nameComp) {
        std::string link_name = nameComp->Data();

        if (link_name.substr(0, 9) == "blue_cone") {
            cone.color = driverless_msgs::msg::Cone::BLUE;
        } else if (link_name.substr(0, 11) == "yellow_cone") {
            cone.color = driverless_msgs::msg::Cone::YELLOW;
        } else if (link_name.substr(0, 11) == "orange_cone") {
            cone.color = driverless_msgs::msg::Cone::ORANGE_SMALL;
        } else if (link_name.substr(0, 8) == "big_cone") {
            cone.color = driverless_msgs::msg::Cone::ORANGE_BIG;
            cone.location.z = 0.225;
        }
    }

    return cone;
}

// Get ground truth track from track model using ECM
driverless_msgs::msg::ConeDetectionStamped get_ground_truth_track(gz::sim::EntityComponentManager &ecm,
                                                                  gz::sim::Entity track_model_entity,
                                                                  std::chrono::steady_clock::duration sim_time,
                                                                  std::string track_frame_id) {
    driverless_msgs::msg::ConeDetectionStamped track;

    track.header.frame_id = track_frame_id;
    
    // Convert chrono duration to ROS time
    auto time_sec = std::chrono::duration_cast<std::chrono::seconds>(sim_time);
    auto time_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(sim_time - time_sec);
    track.header.stamp.sec = time_sec.count();
    track.header.stamp.nanosec = time_nsec.count();

    // Track model uses <include> statements, so cones are child models not links
    // Iterate through all models to find cone models (blue_cone_*, yellow_cone_*, etc.)
    ecm.Each<gz::sim::components::Model, gz::sim::components::Name>(
        [&](const gz::sim::Entity &entity,
            const gz::sim::components::Model *,
            const gz::sim::components::Name *name) -> bool {
            // Check if this is a cone model by name
            std::string model_name = name->Data();
            if (model_name.find("cone") != std::string::npos && model_name != "track") {
                // Get the first link of this cone model (the cone geometry)
                gz::sim::Model cone_model(entity);
                auto links = cone_model.Links(ecm);
                if (!links.empty()) {
                    track.cones.push_back(get_cone_from_link(ecm, links[0]));
                }
            }
            return true; // Continue iterating
        });

    return track;
}

} // namespace vehicle_plugins
} // namespace gazebo_plugins  
