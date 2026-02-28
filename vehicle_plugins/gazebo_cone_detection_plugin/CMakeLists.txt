cmake_minimum_required(VERSION 3.5)

# ground truth cones plugin
ament_auto_add_library(gazebo_cone_detection SHARED
  src/gazebo_cone_detection.cpp
)

target_include_directories(gazebo_cone_detection PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Install plugin library for GZ Sim
install(TARGETS gazebo_cone_detection
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
