cmake_minimum_required(VERSION 3.5)

ament_auto_add_library(gazebo_tf_broadcaster SHARED
  src/tf_broadcaster.cpp
)

target_include_directories(gazebo_tf_broadcaster PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Install plugin library for GZ Sim
install(TARGETS gazebo_tf_broadcaster
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
