cmake_minimum_required(VERSION 3.5)

ament_auto_add_library(gazebo_vehicle_control SHARED
  src/vehicle_control.cpp
)

target_include_directories(gazebo_vehicle_control PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Install plugin library for GZ Sim
install(TARGETS gazebo_vehicle_control
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
