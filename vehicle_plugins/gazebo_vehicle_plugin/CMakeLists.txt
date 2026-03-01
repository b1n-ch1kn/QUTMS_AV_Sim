cmake_minimum_required(VERSION 3.5)

ament_auto_add_library(gazebo_vehicle SHARED
  src/gazebo_vehicle.cpp
)

target_include_directories(gazebo_vehicle PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../gazebo_vehicle_control_plugin/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(gazebo_vehicle yaml-cpp)

# Install plugin library for GZ Sim
install(TARGETS gazebo_vehicle
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
