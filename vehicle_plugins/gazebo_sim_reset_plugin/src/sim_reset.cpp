#include "gazebo_sim_reset_plugin/sim_reset.hpp"
#include "gazebo_vehicle_plugin/VehicleState.hh"
#include "gazebo_vehicle_plugin/vehicle_state.hpp"

#include <gz/sim/components/AngularVelocity.hh>
#include <gz/sim/components/AngularVelocityCmd.hh>
#include <gz/sim/components/LinearVelocity.hh>
#include <gz/sim/components/LinearVelocityCmd.hh>
#include <gz/sim/components/Model.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>
#include <gz/sim/components/PoseCmd.hh>
#include <gz/sim/components/Link.hh>
#include <gz/sim/Util.hh>

namespace gazebo_plugins {
namespace vehicle_plugins {

SimResetPlugin::SimResetPlugin() : first_update(true), ecm_ptr(nullptr) {}

SimResetPlugin::~SimResetPlugin() {}

void SimResetPlugin::Configure(
    const gz::sim::Entity &entity,
    const std::shared_ptr<const sdf::Element> &sdf,
    gz::sim::EntityComponentManager &ecm,
    gz::sim::EventManager &eventMgr)
{
    // Currently unused
    (void)eventMgr;

    // Storage for later
    this->_entity = entity;
    this->_model = gz::sim::Model(entity);
    
    // Error checking
    if (!this->_model.Valid(ecm)) {
        RCLCPP_ERROR(rclcpp::get_logger("sim_reset_plugin"), "Invalid model entity. Plugin won't run.");
        return;
    }

    // Ensure that ROS2 is initialized
    if (!rclcpp::ok()) {
        rclcpp::init(0, nullptr);
    }

    this->node = std::make_shared<rclcpp::Node>("sim_reset_plugin_node");
    RCLCPP_INFO(this->node->get_logger(), "Created ROS node inside SimResetPlugin.");

    // Store ECM pointer for reset service
    ecm_ptr = &ecm;

    // Initialize parameters from SDF
    initParams(sdf);

    // Reset service
    reset_sim_srv = node->create_service<std_srvs::srv::Trigger>("reset_simulation",
        std::bind(&SimResetPlugin::resetSimulation, this, std::placeholders::_1, std::placeholders::_2));

    RCLCPP_INFO(node->get_logger(), "GZ Sim SimResetPlugin Configured");
}

void SimResetPlugin::initParams(const std::shared_ptr<const sdf::Element> &sdf) {
    // Read parameters from SDF
    vehicle_base_frame = sdf->Get<std::string>("base_frame", "base_footprint").first;
    
    RCLCPP_INFO(node->get_logger(), "Sim reset plugin params loaded: base_frame=%s", 
                vehicle_base_frame.c_str());
}

void SimResetPlugin::PreUpdate(const gz::sim::UpdateInfo &info,
                                gz::sim::EntityComponentManager &ecm)
{
    // First update - store initial states
    if (first_update) {
        // Get vehicle initial pose
        auto vehiclePose = ecm.Component<gz::sim::components::Pose>(_entity);
        if (vehiclePose) {
            vehicle_initial_pose = vehiclePose->Data();
            RCLCPP_INFO(node->get_logger(), "Stored vehicle initial pose: (%.2f, %.2f, %.2f)",
                       vehicle_initial_pose.Pos().X(), 
                       vehicle_initial_pose.Pos().Y(), 
                       vehicle_initial_pose.Pos().Z());
        }

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
            RCLCPP_WARN(node->get_logger(), 
                       "Could not find track model. Cone reset will be unavailable.");
        } else {
            // Get all cone links and their initial poses
            gz::sim::Model track(track_model);
            auto links = track.Links(ecm);
            
            for (const auto &link : links) {
                auto linkName = ecm.Component<gz::sim::components::Name>(link);
                if (linkName && linkName->Data().find("cone") != std::string::npos) {
                    cone_links.push_back(link);
                    
                    // Store initial pose
                    gz::math::Pose3d conePose = gz::sim::worldPose(link, ecm);
                    cone_initial_poses.push_back(conePose);
                }
            }
            
            RCLCPP_INFO(node->get_logger(), "Stored %zu cone initial poses", cone_initial_poses.size());
        }

        first_update = false;
        return;
    }

    // Spin ROS callbacks
    rclcpp::spin_some(node);
}

bool SimResetPlugin::resetSimulation(
    std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
    (void)request; // Unused
    
    if (!ecm_ptr) {
        response->success = false;
        response->message = "ECM not available";
        RCLCPP_ERROR(node->get_logger(), "Reset failed: ECM not available");
        return false;
    }

    auto &ecm = *ecm_ptr;
    
    RCLCPP_INFO(node->get_logger(), "Resetting simulation...");
    
    // Reset vehicle
    resetVehicle(ecm);
    
    // Reset cones
    resetCones(ecm);
    
    response->success = true;
    response->message = "Simulation reset successfully";
    RCLCPP_INFO(node->get_logger(), "Simulation reset complete");
    
    return true;
}

void SimResetPlugin::resetVehicle(gz::sim::EntityComponentManager &ecm)
{
    // Reset the vehicle state to initial conditions
    // Create initial state with zeros for velocities and accelerations
    State initial_state;
    initial_state.x = vehicle_initial_pose.Pos().X();
    initial_state.y = vehicle_initial_pose.Pos().Y();
    initial_state.z = vehicle_initial_pose.Pos().Z();
    initial_state.yaw = vehicle_initial_pose.Rot().Yaw();
    initial_state.v_x = 0.0;
    initial_state.v_y = 0.0;
    initial_state.v_z = 0.0;
    initial_state.r_x = 0.0;
    initial_state.r_y = 0.0;
    initial_state.r_z = 0.0;
    initial_state.a_x = 0.0;
    initial_state.a_y = 0.0;
    initial_state.a_z = 0.0;
    
    // Update the VehicleState component
    auto stateComp = ecm.Component<gz::sim::components::VehicleState>(_entity);
    if (stateComp) {
        *stateComp = gz::sim::components::VehicleState(initial_state);
    } else {
        RCLCPP_WARN(node->get_logger(), "VehicleState component not found");
    }
    
    // Also set the simulation pose and velocity commands immediately
    // This ensures the vehicle's physical position updates right away
    const gz::math::Vector3d zero_vel(0.0, 0.0, 0.0);
    
    // Set WorldPoseCmd to move vehicle to initial position
    auto worldPoseCmd = ecm.Component<gz::sim::components::WorldPoseCmd>(_entity);
    if (worldPoseCmd) {
        *worldPoseCmd = gz::sim::components::WorldPoseCmd(vehicle_initial_pose);
    } else {
        ecm.CreateComponent(_entity, gz::sim::components::WorldPoseCmd(vehicle_initial_pose));
    }
    
    // Set WorldLinearVelocityCmd to zero
    auto linVelCmd = ecm.Component<gz::sim::components::WorldLinearVelocityCmd>(_entity);
    if (linVelCmd) {
        *linVelCmd = gz::sim::components::WorldLinearVelocityCmd(zero_vel);
    } else {
        ecm.CreateComponent(_entity, gz::sim::components::WorldLinearVelocityCmd(zero_vel));
    }
    
    // Set WorldAngularVelocityCmd to zero
    auto angVelCmd = ecm.Component<gz::sim::components::WorldAngularVelocityCmd>(_entity);
    if (angVelCmd) {
        *angVelCmd = gz::sim::components::WorldAngularVelocityCmd(zero_vel);
    } else {
        ecm.CreateComponent(_entity, gz::sim::components::WorldAngularVelocityCmd(zero_vel));
    }
    
    RCLCPP_INFO(node->get_logger(), "Vehicle reset to initial pose (%.2f, %.2f, %.2f)",
               vehicle_initial_pose.Pos().X(), 
               vehicle_initial_pose.Pos().Y(), 
               vehicle_initial_pose.Pos().Z());
}

void SimResetPlugin::resetCones(gz::sim::EntityComponentManager &ecm)
{
    if (cone_links.empty() || cone_initial_poses.empty()) {
        RCLCPP_DEBUG(node->get_logger(), "No cones to reset");
        return;
    }

    // Zero velocity
    const gz::math::Vector3d zero_vel(0.0, 0.0, 0.0);

    // Loop through all cones and reset to initial positions
    for (size_t i = 0; i < cone_links.size() && i < cone_initial_poses.size(); i++) {
        gz::sim::Entity cone_link = cone_links[i];
        gz::math::Pose3d initial_pose = cone_initial_poses[i];

        // Set cone position using ECM (using world pose for accuracy)
        auto poseComp = ecm.Component<gz::sim::components::Pose>(cone_link);
        if (poseComp) {
            *poseComp = gz::sim::components::Pose(initial_pose);
        } else {
            ecm.CreateComponent(cone_link, gz::sim::components::Pose(initial_pose));
        }

        // Set linear velocity to zero
        auto linVelComp = ecm.Component<gz::sim::components::LinearVelocity>(cone_link);
        if (linVelComp) {
            *linVelComp = gz::sim::components::LinearVelocity(zero_vel);
        } else {
            ecm.CreateComponent(cone_link, gz::sim::components::LinearVelocity(zero_vel));
        }

        // Set angular velocity to zero
        auto angVelComp = ecm.Component<gz::sim::components::AngularVelocity>(cone_link);
        if (angVelComp) {
            *angVelComp = gz::sim::components::AngularVelocity(zero_vel);
        } else {
            ecm.CreateComponent(cone_link, gz::sim::components::AngularVelocity(zero_vel));
        }
    }

    RCLCPP_DEBUG(node->get_logger(), "Reset %zu cones to initial positions", cone_links.size());
}

}  // namespace vehicle_plugins
}  // namespace gazebo_plugins

// Register the plugin
#include <gz/plugin/Register.hh>
GZ_ADD_PLUGIN(
    gazebo_plugins::vehicle_plugins::SimResetPlugin,
    gz::sim::System,
    gazebo_plugins::vehicle_plugins::SimResetPlugin::ISystemConfigure,
    gazebo_plugins::vehicle_plugins::SimResetPlugin::ISystemPreUpdate)
