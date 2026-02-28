cmake_minimum_required(VERSION 3.5)

ament_auto_add_library(gazebo_ins_odometry SHARED
  src/ins_odometry.cpp
)

target_include_directories(gazebo_ins_odometry PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  # Include vehicle plugin headers for shared utilities (Noise, utils)
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../gazebo_vehicle_plugin/include>
)

target_link_libraries(gazebo_ins_odometry yaml-cpp)

# Install plugin library for GZ Sim
install(TARGETS gazebo_ins_odometry
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
