#ifndef GZ_SIM_COMPONENTS_VEHICLE_STATE_HPP
#define GZ_SIM_COMPONENTS_VEHICLE_STATE_HPP

#include <gz/sim/components/Component.hh>
#include <gz/sim/components/Factory.hh>
#include <gz/sim/components/Serialization.hh>
#include <gz/sim/config.hh>

#include "gazebo_vehicle_plugin/vehicle_state.hpp"

namespace gz
{
namespace sim
{
// Inline bracket to help doxygen filtering
inline namespace GZ_SIM_VERSION_NAMESPACE {
namespace components
{
  /// \brief Component that stores the vehicle dynamics state
  /// Contains position, orientation, velocities, and accelerations
  /// This allows any plugin to read or modify vehicle state through ECM
  using VehicleState = Component<gazebo_plugins::vehicle_plugins::State, 
                                  class VehicleStateTag>;
  GZ_SIM_REGISTER_COMPONENT("gz_sim_components.VehicleState", VehicleState)
}
}
}
}

#endif  // GZ_SIM_COMPONENTS_VEHICLE_STATE_HPP
