#include "gazebo_tf_broadcaster_plugin/tf_broadcaster.hpp"

#include <gz/sim/components/Pose.hh>
#include <gz/sim/components/Name.hh>

namespace gazebo_plugins {
namespace vehicle_plugins {

TFBroadcasterPlugin::TFBroadcasterPlugin() 
    : last_published_time(std::chrono::steady_clock::duration::zero()),
      first_update(true) {}

TFBroadcasterPlugin::~TFBroadcasterPlugin() {}

void TFBroadcasterPlugin::Configure(
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
        RCLCPP_FATAL(rclcpp::get_logger("tf_broadcaster_plugin"), 
                    "TF Broadcaster Plugin should be attached to a model entity. Failed to initialize.");
        return;
    }

    // Ensure that ROS2 is initialized
    if (!rclcpp::ok()) {
        rclcpp::init(0, nullptr);
    }

    this->node = std::make_shared<rclcpp::Node>("tf_broadcaster_plugin_node");
    RCLCPP_INFO(this->node->get_logger(), "Created ROS node inside TFBroadcasterPlugin.");

    // Load parameters from SDF
    loadParameters(sdf);

    // Create TF broadcaster
    tf_broadcaster = std::make_unique<tf2_ros::TransformBroadcaster>(node);

    RCLCPP_INFO(this->node->get_logger(), 
                "TF Broadcaster Plugin initialized - %s -> %s at %.1f Hz",
                map_frame.c_str(), base_frame.c_str(), publish_rate);
}

void TFBroadcasterPlugin::loadParameters(const std::shared_ptr<const sdf::Element> &sdf) {
    // Get parameters from SDF with defaults
    publish_rate = sdf->Get<double>("publish_rate", 50.0).first;
    map_frame = sdf->Get<std::string>("map_frame", "track").first;
    odom_frame = sdf->Get<std::string>("odom_frame", "track").first;
    base_frame = sdf->Get<std::string>("base_frame", "base_footprint").first;
    publish_odom_tf = sdf->Get<bool>("publish_odom_tf", false).first;
    
    RCLCPP_INFO(node->get_logger(), 
                "TF params: publish_rate=%.1f, map=%s, odom=%s, base=%s, publish_odom_tf=%s", 
                publish_rate, map_frame.c_str(), odom_frame.c_str(), base_frame.c_str(),
                publish_odom_tf ? "true" : "false");
}

void TFBroadcasterPlugin::PostUpdate(const gz::sim::UpdateInfo &info,
                                      const gz::sim::EntityComponentManager &ecm)
{
    // Skip if paused
    if (info.paused) {
        return;
    }

    // Always publish on first update to initialize TF tree
    if (first_update) {
        publishTransforms(info, ecm);
        last_published_time = info.simTime;
        first_update = false;
        rclcpp::spin_some(node);
        return;
    }

    // Publish at configured rate    
    double dt = std::chrono::duration<double>(info.simTime - last_published_time).count();
    if (dt >= 1.0 / publish_rate) {
        publishTransforms(info, ecm);
        last_published_time = info.simTime;
    }

    // Spin ROS node to process callbacks
    rclcpp::spin_some(node);
}

void TFBroadcasterPlugin::publishTransforms(const gz::sim::UpdateInfo &info,
                                             const gz::sim::EntityComponentManager &ecm)
{
    // Read vehicle pose from ECM
    const auto *pose_comp = ecm.Component<gz::sim::components::Pose>(entity_);

    if (!pose_comp) {
        RCLCPP_WARN_THROTTLE(node->get_logger(), *node->get_clock(), 1000,
                            "Missing Pose component in ECM for TF broadcasting");
        return;
    }

    // Extract pose data
    const gz::math::Pose3d &pose = pose_comp->Data();

    // Convert simulation time to ROS time
    rclcpp::Time stamp(std::chrono::duration_cast<std::chrono::nanoseconds>(info.simTime).count(),
                       RCL_ROS_TIME);

    // Create transform (map_frame -> base_frame)
    geometry_msgs::msg::TransformStamped transform_stamped;
    transform_stamped.header.stamp = stamp;
    transform_stamped.header.frame_id = map_frame;
    transform_stamped.child_frame_id = base_frame;

    // Position
    transform_stamped.transform.translation.x = pose.Pos().X();
    transform_stamped.transform.translation.y = pose.Pos().Y();
    transform_stamped.transform.translation.z = pose.Pos().Z();

    // Orientation
    transform_stamped.transform.rotation.x = pose.Rot().X();
    transform_stamped.transform.rotation.y = pose.Rot().Y();
    transform_stamped.transform.rotation.z = pose.Rot().Z();
    transform_stamped.transform.rotation.w = pose.Rot().W();

    // Broadcast transform
    tf_broadcaster->sendTransform(transform_stamped);

    // Optionally publish odom -> base_link transform (for nav stack compatibility)
    if (publish_odom_tf && odom_frame != map_frame) {
        geometry_msgs::msg::TransformStamped odom_transform;
        odom_transform.header.stamp = stamp;
        odom_transform.header.frame_id = odom_frame;
        odom_transform.child_frame_id = base_frame;
        
        // Same pose
        odom_transform.transform = transform_stamped.transform;
        
        tf_broadcaster->sendTransform(odom_transform);
    }
}

}  // namespace vehicle_plugins
}  // namespace gazebo_plugins

// Register the plugin
#include <gz/plugin/Register.hh>
GZ_ADD_PLUGIN(
    gazebo_plugins::vehicle_plugins::TFBroadcasterPlugin,
    gz::sim::System,
    gazebo_plugins::vehicle_plugins::TFBroadcasterPlugin::ISystemConfigure,
    gazebo_plugins::vehicle_plugins::TFBroadcasterPlugin::ISystemPostUpdate)
