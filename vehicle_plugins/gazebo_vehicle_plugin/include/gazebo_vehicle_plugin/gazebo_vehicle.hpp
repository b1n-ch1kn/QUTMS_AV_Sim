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
#include "gazebo_vehicle_plugin/utils.hpp"
#include "gazebo_vehicle_plugin/vehicle_model_bike.hpp"
#include "gazebo_vehicle_plugin/vehicle_state.hpp"

// Control component (from control plugin)
#include "gazebo_vehicle_control_plugin/vehicle_control_component.hpp"

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
    void setModelState(gz::sim::EntityComponentManager &ecm);
    void update(const gz::sim::UpdateInfo &info, gz::sim::EntityComponentManager &ecm);

    std::shared_ptr<rclcpp::Node> node;

    // Vehicle Motion
    VehicleModelBikePtr vehicle_model;
    Control input, output;
    State state;
    gz::math::Pose3d offset;

    // GZ Sim
    gz::sim::Entity _entity;
    gz::sim::Model _model;
    std::chrono::steady_clock::duration last_sim_time;

    // Rate to update vehicle dynamics
    double update_rate;

    // Steering joints
    gz::sim::Entity left_steering_joint;
    gz::sim::Entity right_steering_joint;

    // Steering rate limit variables
    double max_steering_rate, steering_lock_time;
    
    // Track initial pose
    gz::math::Pose3d car_initial_pose;
    bool first_update;
};

}  // namespace vehicle_plugins
}  // namespace gazebo_plugins

#endif  // VEHICLE_PLUGINS_GAZEBO_VEHICLE_PLUGIN_INCLUDE_GAZEBO_VEHICLE_PLUGIN_GAZEBO_VEHICLE_HPP_
