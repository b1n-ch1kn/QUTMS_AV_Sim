#ifndef GAZEBO_SIM_RESET_PLUGIN_HPP
#define GAZEBO_SIM_RESET_PLUGIN_HPP

#include <gz/sim/System.hh>
#include <gz/sim/Model.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/math/Pose3.hh>

#include <rclcpp/rclcpp.hpp>
#include <std_srvs/srv/trigger.hpp>

#include <memory>

namespace gazebo_plugins {
namespace vehicle_plugins {

class SimResetPlugin : public gz::sim::System,
                       public gz::sim::ISystemConfigure,
                       public gz::sim::ISystemPreUpdate
{
public:
    SimResetPlugin();
    ~SimResetPlugin() override;

    void Configure(
        const gz::sim::Entity &entity,
        const std::shared_ptr<const sdf::Element> &sdf,
        gz::sim::EntityComponentManager &ecm,
        gz::sim::EventManager &eventMgr) override;

    void PreUpdate(
        const gz::sim::UpdateInfo &info,
        gz::sim::EntityComponentManager &ecm) override;

private:
    void initParams(const std::shared_ptr<const sdf::Element> &sdf);
    
    bool resetSimulation(
        std::shared_ptr<std_srvs::srv::Trigger::Request> request,
        std::shared_ptr<std_srvs::srv::Trigger::Response> response);

    void resetVehicle(gz::sim::EntityComponentManager &ecm);
    void resetCones(gz::sim::EntityComponentManager &ecm);

    // ROS
    rclcpp::Node::SharedPtr node;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr reset_sim_srv;

    // Entity tracking
    gz::sim::Entity _entity;
    gz::sim::Model _model;
    
    // Initial states
    gz::math::Pose3d vehicle_initial_pose;
    std::vector<gz::math::Pose3d> cone_initial_poses;
    std::vector<gz::sim::Entity> cone_canonical_links;
    
    // Configuration
    std::string vehicle_base_frame;
    
    // State
    bool first_update;
    bool reset_requested;
};

}  // namespace vehicle_plugins
}  // namespace gazebo_plugins

#endif  // GAZEBO_SIM_RESET_PLUGIN_HPP
