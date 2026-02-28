cmake_minimum_required(VERSION 3.5)
project(vehicle_plugins)

# Default to C++17 for GZ Sim
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # we dont use add_compile_options with pedantic in message packages
  # because the Python C extensions dont comply with it
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wno-inconsistent-missing-override")
endif()

find_package(gz-physics7 REQUIRED)
find_package(gz-transport13 REQUIRED)
find_package(gz-sensors8 REQUIRED)
find_package(gz-plugin2 REQUIRED)

# Find ROS packages automatically from package.xml
find_package(ament_cmake_auto REQUIRED)
ament_auto_find_build_dependencies()

# Manual yaml-cpp find (vendor package pattern)
find_package(yaml-cpp REQUIRED)

include_directories(/usr/include/gz/sim8/)
include_directories(/usr/include/gz/plugin2/)
include_directories(/usr/include/gz/sensors8/)
include_directories(/usr/include/gz/rendering8/)
include_directories(/usr/include/gz/common5/)
include_directories(/usr/include/gz/sdformat14/)
include_directories(/usr/include/gz/msgs10/)
include_directories(/usr/include/gz/math7/)
include_directories(/usr/include/gz/transport13/)
include_directories(/usr/include/gz/sim8/gz/sim/components/)

option(ENABLE_PROFILER "Enable Ignition Profiler" FALSE)
if(ENABLE_PROFILER)
  find_package(ignition-common3 QUIET COMPONENTS profiler)
  if(ignition-common3_FOUND)
    add_definitions("-DIGN_PROFILER_ENABLE=1" "-DIGN_PROFILER_REMOTERY=1")
    message(STATUS "Profiler is active")
  else()
    message(WARNING "Can't find Ignition common3. Profiler will not be actived")
    add_definitions("-DIGN_PROFILER_ENABLE=0" "-DIGN_PROFILER_REMOTERY=0")
  endif()
endif()

add_subdirectory(gazebo_vehicle_plugin)
add_subdirectory(gazebo_cone_detection_plugin)

# Export GZ dependencies manually (not in package.xml)
ament_export_dependencies(gz-physics7)
ament_export_dependencies(gz-transport13)
ament_export_dependencies(gz-sensors8)
ament_export_dependencies(gz-plugin2)

ament_auto_package()
