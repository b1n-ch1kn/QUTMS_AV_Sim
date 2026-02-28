cmake_minimum_required(VERSION 3.8)
project(qutms_sim)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake_auto REQUIRED)
ament_auto_find_build_dependencies()

# launch folder
install(DIRECTORY
  launch
  config
  visuals
  meshes
  urdf
  models
  worlds
  DESTINATION share/${PROJECT_NAME}
)

ament_auto_package()
