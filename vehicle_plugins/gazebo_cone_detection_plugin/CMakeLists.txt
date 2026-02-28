cmake_minimum_required(VERSION 3.5)

# ground truth cones plugin
add_library(gazebo_cone_detection SHARED
  src/gazebo_cone_detection.cpp)

ament_target_dependencies(gazebo_cone_detection
  "ros_gz"
  "rclcpp"
  "std_srvs"
  "visualization_msgs"
  "driverless_msgs"
  "gz-physics7"
  "gz-transport13"
  "gz-plugin2"
)

target_include_directories(gazebo_cone_detection PUBLIC include)
target_include_directories(gazebo_cone_detection PUBLIC ${PROJECT_SOURCE_DIR}/include)
ament_export_libraries(gazebo_cone_detection)

install(TARGETS gazebo_cone_detection
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib
)
