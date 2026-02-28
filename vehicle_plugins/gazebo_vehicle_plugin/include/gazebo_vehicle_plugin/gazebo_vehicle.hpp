#ifndef VEHICLE_PLUGINS_GAZEBO_VEHICLE_PLUGIN_INCLUDE_GAZEBO_VEHICLE_PLUGIN_GAZEBO_VEHICLE_HPP_
#define VEHICLE_PLUGINS_GAZEBO_VEHICLE_PLUGIN_INCLUDE_GAZEBO_VEHICLE_PLUGIN_GAZEBO_VEHICLE_HPP_

#include <algorithm>
#include <fstream>
#include <mutex>   // NOLINT(build/c++11)
#include <thread>  // NOLINT(build/c++11)

#include <memory>
#include <queue>
#include <string>
#include <vector>

// ROS Includes
#include "rclcpp/rclcpp.hpp"

// ROS msgs
#include "ackermann_msgs/msg/ackermann_drive_stamped.hpp"
#include "geometry_msgs/msg/vector3.hpp"
#include "geometry_msgs/msg/quaternion.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

// ROS TF2
#include <tf2/transform_datatypes.h>
#include <tf2/utils.h>
#include <tf2_ros/transform_broadcaster.h>
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

// ROS srvs
#include <std_srvs/srv/trigger.hpp>

// GZ Sim Includes
#include <gz/plugin/Register.hh>
#include <gz/sim/System.hh>
#include <gz/sim/Model.hh>
#include <gz/sim/Util.hh>
#include <gz/sim/World.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/Types.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/Joint.hh>
#include <gz/transport/Node.hh>

// GZ Math
#include <gz/math/Pose3.hh>
#include <gz/math/Vector3.hh>

// Local includes
#include "gazebo_vehicle_plugin/noise.hpp"
#include "gazebo_vehicle_plugin/utils.hpp"
#include "gazebo_vehicle_plugin/vehicle_model_bike.hpp"
#include "gazebo_vehicle_plugin/vehicle_state.hpp"

namespace gazebo_plugins {
namespace vehicle_plugins {

class VehiclePlugin : public gz::sim::System,
                      public gz::sim::ISystemConfigure,
                      public gz::sim::ISystemPreUpdate {
   public:
    VehiclePlugin();
    ~VehiclePlugin() override;

    // GZ Sim System interface
    void Configure(const gz::sim::Entity &entity,
                   const std::shared_ptr<const sdf::Element> &sdf,
                   gz::sim::EntityComponentManager &ecm,
                   gz::sim::EventManager &eventMgr) override;

    void PreUpdate(const gz::sim::UpdateInfo &info,
                   gz::sim::EntityComponentManager &ecm) override;

   private:
    void initParams(const std::shared_ptr<const sdf::Element> &sdf);
    void setPositionFromWorld(gz::sim::EntityComponentManager &ecm);
    bool resetVehiclePosition(std::shared_ptr<std_srvs::srv::Trigger::Request> request,
                              std::shared_ptr<std_srvs::srv::Trigger::Response> response);
    void setModelState(gz::sim::EntityComponentManager &ecm);
    void publishVehicleOdom();
    void publishTf();
    void update(const gz::sim::UpdateInfo &info, gz::sim::EntityComponentManager &ecm);
    void onAckermannCmd(const ackermann_msgs::msg::AckermannDriveStamped::SharedPtr msg);
    void onTwistCmd(const geometry_msgs::msg::Twist::SharedPtr msg);

    nav_msgs::msg::Odometry stateToOdom(const State &state);
    State odomToState(const nav_msgs::msg::Odometry &odom);

    std::shared_ptr<rclcpp::Node> node;

    // Vehicle Motion
    VehicleModelBikePtr vehicle_model;
    nav_msgs::msg::Odometry state_odom;
    Control input, output;
    State state;
    std::unique_ptr<Noise> motion_noise;
    gz::math::Pose3d offset;

    // GZ Sim
    gz::sim::Entity _entity;
    gz::sim::Model _model;
    std::chrono::steady_clock::duration last_sim_time, last_cmd_time, last_published_time;

    // Rate to publish ros messages
    double update_rate;
    double publish_rate;

    // ROS TF
    bool pub_tf;
    std::string map_frame;
    std::string odom_frame;
    std::string base_frame;
    std::unique_ptr<tf2_ros::TransformBroadcaster> tf_br;

    // ROS Publishers
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odometry_pub;
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr gt_odometry_pub;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_state_pub;

    // ROS Subscriptions
    rclcpp::Subscription<ackermann_msgs::msg::AckermannDriveStamped>::SharedPtr ackermann_cmd_sub;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr twist_cmd_sub;

    // ROS Services
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr reset_vehicle_pos_srv;

    // Steering joints
    gz::sim::Entity left_steering_joint;
    gz::sim::Entity right_steering_joint;

    // Command queue for control delays
    ackermann_msgs::msg::AckermannDriveStamped last_cmd;
    double control_delay;
    // Steering rate limit variables
    double max_steering_rate, steering_lock_time;
    
    // Track initial pose
    gz::math::Pose3d car_initial_pose;
    bool first_update;
};

}  // namespace vehicle_plugins
}  // namespace gazebo_plugins

#endif  // VEHICLE_PLUGINS_GAZEBO_VEHICLE_PLUGIN_INCLUDE_GAZEBO_VEHICLE_PLUGIN_GAZEBO_VEHICLE_HPP_
