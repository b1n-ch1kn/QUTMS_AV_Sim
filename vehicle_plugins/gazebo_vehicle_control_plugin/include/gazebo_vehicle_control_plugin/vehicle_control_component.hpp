#ifndef VEHICLE_PLUGINS_GAZEBO_VEHICLE_CONTROL_PLUGIN_INCLUDE_GAZEBO_VEHICLE_CONTROL_PLUGIN_VEHICLE_CONTROL_COMPONENT_HPP_
#define VEHICLE_PLUGINS_GAZEBO_VEHICLE_CONTROL_PLUGIN_INCLUDE_GAZEBO_VEHICLE_CONTROL_PLUGIN_VEHICLE_CONTROL_COMPONENT_HPP_

#include <gz/sim/components/Component.hh>
#include <gz/sim/components/Factory.hh>
#include <gz/sim/components/Serialization.hh>
#include <gz/sim/config.hh>

namespace gazebo_plugins {
namespace vehicle_plugins {

/// \brief Data structure for vehicle control inputs
struct VehicleControlData {
    /// Desired velocity (m/s)
    double velocity{0.0};
    
    /// Desired acceleration (m/s²)
    double acceleration{0.0};
    
    /// Desired steering angle (radians)
    double steering{0.0};
    
    /// Timestamp of last command (for timeout detection)
    double timestamp{0.0};
    
    bool operator==(const VehicleControlData &_data) const {
        return this->velocity == _data.velocity &&
               this->acceleration == _data.acceleration &&
               this->steering == _data.steering &&
               this->timestamp == _data.timestamp;
    }
};

}  // namespace vehicle_plugins
}  // namespace gazebo_plugins

namespace gz {
namespace sim {
namespace components {

/// \brief Custom ECM component for vehicle control inputs
/// This component stores the desired control inputs from ROS command topics
/// and makes them available to the vehicle dynamics plugin via ECM.
using VehicleControlInput = Component<gazebo_plugins::vehicle_plugins::VehicleControlData, 
                                      class VehicleControlInputTag>;

GZ_SIM_REGISTER_COMPONENT("gazebo_plugins.vehicle_plugins.VehicleControlInput", 
                           VehicleControlInput)

}  // namespace components
}  // namespace sim
}  // namespace gz

#endif  // VEHICLE_PLUGINS_GAZEBO_VEHICLE_CONTROL_PLUGIN_INCLUDE_GAZEBO_VEHICLE_CONTROL_PLUGIN_VEHICLE_CONTROL_COMPONENT_HPP_
