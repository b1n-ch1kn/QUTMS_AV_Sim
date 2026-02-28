#include "gazebo_ins_odometry_plugin/ins_odometry.hpp"

#include <gz/sim/components/AngularVelocity.hh>
#include <gz/sim/components/LinearVelocity.hh>
#include <gz/sim/components/Pose.hh>
#include <gz/sim/components/Name.hh>

#include "gazebo_vehicle_plugin/utils.hpp"

namespace gazebo_plugins {
namespace vehicle_plugins {

INSOdometryPlugin::INSOdometryPlugin() 
    : last_published_time(std::chrono::steady_clock::duration::zero()) {}

INSOdometryPlugin::~INSOdometryPlugin() {}

void INSOdometryPlugin::Configure(
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
        RCLCPP_FATAL(rclcpp::get_logger("ins_odometry_plugin"), 
                    "INSOdometry Plugin should be attached to a model entity. Failed to initialize.");
        return;
    }

    // Ensure that ROS2 is initialized
    if (!rclcpp::ok()) {
        rclcpp::init(0, nullptr);
    }

    this->node = std::make_shared<rclcpp::Node>("ins_odometry_plugin_node");
    RCLCPP_INFO(this->node->get_logger(), "Created ROS node inside INSOdometryPlugin.");

    // Load parameters from SDF
    loadParameters(sdf);

    // Create ROS publishers
    odometry_pub = node->create_publisher<nav_msgs::msg::Odometry>(topic_name, 1);
    RCLCPP_INFO(node->get_logger(), "INS Odometry publishing to: %s", topic_name.c_str());

    if (enable_ground_truth) {
        ground_truth_pub = node->create_publisher<nav_msgs::msg::Odometry>(ground_truth_topic_name, 1);
        RCLCPP_INFO(node->get_logger(), "INS Ground Truth publishing to: %s", ground_truth_topic_name.c_str());
    }

    RCLCPP_INFO(this->node->get_logger(), 
                "INS Odometry Plugin initialized - publish_rate=%.1f Hz, ground_truth=%s",
                publish_rate, enable_ground_truth ? "enabled" : "disabled");
}

void INSOdometryPlugin::loadParameters(const std::shared_ptr<const sdf::Element> &sdf) {
    // Read noise parameters from SDF
    if (sdf->HasElement("noise_params")) {
        std::string noise_yaml_name = sdf->Get<std::string>("noise_params");
        RCLCPP_INFO(node->get_logger(), "Loading noise params from: %s", noise_yaml_name.c_str());
        motion_noise = std::make_unique<Noise>(noise_yaml_name);
    } else {
        RCLCPP_FATAL(node->get_logger(), 
                    "INSOdometry plugin missing <noise_params> parameter in URDF, cannot proceed");
        exit(1);
    }
    
    // Get other parameters from SDF with defaults
    publish_rate = sdf->Get<double>("publish_rate", 50.0).first;
    odom_frame = sdf->Get<std::string>("odom_frame", "track").first;
    base_frame = sdf->Get<std::string>("base_frame", "base_footprint").first;
    topic_name = sdf->Get<std::string>("topic_name", "odometry").first;
    ground_truth_topic_name = sdf->Get<std::string>("ground_truth_topic_name", "odometry/ground_truth").first;
    enable_ground_truth = sdf->Get<bool>("enable_ground_truth", true).first;
    
    RCLCPP_INFO(node->get_logger(), 
                "INS params: publish_rate=%.1f, odom_frame=%s, base_frame=%s, enable_gt=%s", 
                publish_rate, odom_frame.c_str(), base_frame.c_str(), 
                enable_ground_truth ? "true" : "false");
}

void INSOdometryPlugin::PostUpdate(const gz::sim::UpdateInfo &info,
                                    const gz::sim::EntityComponentManager &ecm)
{
    // Skip if paused
    if (info.paused) {
        return;
    }

    // Publish at configured rate    
    double dt = std::chrono::duration<double>(info.simTime - last_published_time).count();
    if (dt >= 1.0 / publish_rate) {
        publishOdometry(info, ecm);
        last_published_time = info.simTime;
    }

    // Spin ROS node to process callbacks
    rclcpp::spin_some(node);
}

void INSOdometryPlugin::publishOdometry(const gz::sim::UpdateInfo &info,
                                         const gz::sim::EntityComponentManager &ecm)
{
    // Read vehicle state from ECM components
    const auto *pose_comp = ecm.Component<gz::sim::components::Pose>(entity_);
    const auto *lin_vel_comp = ecm.Component<gz::sim::components::LinearVelocity>(entity_);
    const auto *ang_vel_comp = ecm.Component<gz::sim::components::AngularVelocity>(entity_);

    if (!pose_comp || !lin_vel_comp || !ang_vel_comp) {
        RCLCPP_WARN_THROTTLE(node->get_logger(), *node->get_clock(), 1000,
                            "Missing required ECM components for odometry");
        return;
    }

    // Extract data from components
    const gz::math::Pose3d &pose = pose_comp->Data();
    const gz::math::Vector3d &lin_vel = lin_vel_comp->Data();
    const gz::math::Vector3d &ang_vel = ang_vel_comp->Data();

    // Convert simulation time to ROS time
    rclcpp::Time stamp(std::chrono::duration_cast<std::chrono::nanoseconds>(info.simTime).count(),
                       RCL_ROS_TIME);

    // Create ground truth odometry message
    nav_msgs::msg::Odometry odom;
    odom.header.stamp = stamp;
    odom.header.frame_id = odom_frame;
    odom.child_frame_id = base_frame;

    // Position
    odom.pose.pose.position.x = pose.Pos().X();
    odom.pose.pose.position.y = pose.Pos().Y();
    odom.pose.pose.position.z = pose.Pos().Z();

    // Orientation
    odom.pose.pose.orientation.x = pose.Rot().X();
    odom.pose.pose.orientation.y = pose.Rot().Y();
    odom.pose.pose.orientation.z = pose.Rot().Z();
    odom.pose.pose.orientation.w = pose.Rot().W();

    // Linear velocity
    odom.twist.twist.linear.x = lin_vel.X();
    odom.twist.twist.linear.y = lin_vel.Y();
    odom.twist.twist.linear.z = lin_vel.Z();

    // Angular velocity
    odom.twist.twist.angular.x = ang_vel.X();
    odom.twist.twist.angular.y = ang_vel.Y();
    odom.twist.twist.angular.z = ang_vel.Z();

    // Apply noise to create simulated INS sensor data
    nav_msgs::msg::Odometry odom_noisy = motion_noise->applyNoise(odom);

    // Publish noisy odometry
    if (odometry_pub->get_subscription_count() > 0) {
        odometry_pub->publish(odom_noisy);
    }

    // Publish ground truth if enabled
    if (enable_ground_truth && ground_truth_pub && ground_truth_pub->get_subscription_count() > 0) {
        ground_truth_pub->publish(odom);
    }
}

}  // namespace vehicle_plugins
}  // namespace gazebo_plugins

// Register the plugin
#include <gz/plugin/Register.hh>
GZ_ADD_PLUGIN(
    gazebo_plugins::vehicle_plugins::INSOdometryPlugin,
    gz::sim::System,
    gazebo_plugins::vehicle_plugins::INSOdometryPlugin::ISystemConfigure,
    gazebo_plugins::vehicle_plugins::INSOdometryPlugin::ISystemPostUpdate)
