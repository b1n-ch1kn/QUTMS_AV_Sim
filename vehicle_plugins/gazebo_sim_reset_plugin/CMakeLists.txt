cmake_minimum_required(VERSION 3.8)

# Build the plugin as a shared library
ament_auto_add_library(gazebo_sim_reset SHARED
  src/sim_reset.cpp
)

target_include_directories(gazebo_sim_reset PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../gazebo_vehicle_plugin/include>
  $<INSTALL_INTERFACE:include>
)

# Install the plugin library
install(TARGETS gazebo_sim_reset
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
