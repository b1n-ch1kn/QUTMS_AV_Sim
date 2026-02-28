# QUTMS_AV_Sim - ROS 2 Jazzy Migration Status

## ✅ Completed Migration Steps

### 1. Package Dependencies Updated
- **vehicle_plugins/package.xml**: Updated to use `gz-sim8`, `gz-plugin2`, `gz-transport13`, `gz-sensors8`, `ros_gz`
- **qutms_sim/package.xml**: Updated to use `ros_gz_sim`, `ros_gz_bridge`

### 2. Build Configuration Updated  
- **vehicle_plugins/CMakeLists.txt**: Updated to find and link against GZ Sim libraries (gz-physics7, gz-transport13, gz-sensors8, gz-plugin2)
- **gazebo_vehicle_plugin/CMakeLists.txt**: Updated dependencies
- **gazebo_cone_detection_plugin/CMakeLists.txt**: Updated dependencies

### 3. Vehicle Plugin Migrated to GZ Sim
- **gazebo_vehicle.hpp**: Completely refactored to use GZ Sim System interface
  - Changed from `gazebo::ModelPlugin` to `gz::sim::System`, `ISystemConfigure`, `ISystemPreUpdate`
  - Updated all Gazebo Classic API calls to GZ Sim ECS (Entity Component System)
  - Replaced `gazebo::physics::ModelPtr` with `gz::sim::Model` and `gz::sim::Entity`
  - Changed time handling from `gazebo::common::Time` to `std::chrono::duration`
  
- **gazebo_vehicle.cpp**: Fully rewritten for GZ Sim
  - `Load()` → `Configure()` + `PreUpdate()`
  - Direct ROS node creation instead of `gazebo_ros::Node::Get()`
  - ECS-based component access for pose, velocity, joint positions
  - Plugin registration using `GZ_ADD_PLUGIN` macro

- **utils.hpp**: Removed Gazebo Classic dependencies, kept only ROS utilities

### 4. URDF Updates
- **robot.urdf.xacro**: Updated plugin tags for GZ Sim
  - Added Physics system plugin requirement
  - Updated plugin filenames and namespacing
  
- **lidar.urdf.xacro**: Migrated to use GZ Sim sensors
  - Changed to use `gz-sim-sensors-system`
  - Added ros_gz_bridge for sensor data publishing

### 5. Launch File Updates
- **sim.launch.py**: Converted from gazebo_ros to ros_gz_sim
  - Updated environment variables (`GAZEBO_*` → `GZ_SIM_*`)
  - Changed from `gazebo.launch.py` to `gz_sim.launch.py`
  - Updated spawn command from `spawn_entity.py` to `create`
  - World files now expected as `.sdf` instead of `.world`

## ⏳ Remaining Work

### High Priority

#### 1. Cone Detection Plugin Migration ✅ COMPLETED
The `gazebo_cone_detection_plugin` has been successfully migrated to GZ Sim:
- ✅ Updated header to use GZ Sim System interface
- ✅ Rewrote source to use ECS for world model access
- ✅ Updated cone detection logic to work with GZ Sim's entity system
- ✅ Updated utils.hpp to use GZ Sim API
- ⏳ Test cone detection functionality (requires Ubuntu 24.04/Jazzy)

**Files updated:**
- `/home/wsl/fsae/QUTMS_AV_Sim/vehicle_plugins/gazebo_cone_detection_plugin/include/gazebo_cone_detection_plugin/gazebo_cone_detection.hpp`
- `/home/wsl/fsae/QUTMS_AV_Sim/vehicle_plugins/gazebo_cone_detection_plugin/src/gazebo_cone_detection.cpp`
- `/home/wsl/fsae/QUTMS_AV_Sim/vehicle_plugins/gazebo_cone_detection_plugin/include/gazebo_cone_detection_plugin/utils.hpp`

**Backup files created:**
- `gazebo_cone_detection_classic.cpp.backup`

#### 2. World Files Conversion ✅ COMPLETED
World files have been renamed from `.world` to `.sdf`:
- ✅ Renamed all `.world` files to `.sdf` using provided script
- ⏳ Consider updating SDF version to 1.8 for better GZ Sim compatibility (optional)
- ⏳ Verify all tracks load in GZ Sim (requires testing)

**Files renamed:** 10 world files in `/home/wsl/fsae/QUTMS_AV_Sim/qutms_sim/worlds/`

#### 3. Test and Validate
- Build the workspace on Ubuntu 24.04 with ROS 2 Jazzy
- Test vehicle plugin functionality:
  - Odometry publishing
  - Vehicle control (Ackermann and Twist commands)
  - Steering visualization
  - Reset service
- Test sensor integration (LIDAR)
- Verify TF publishing

### Medium Priority

#### 4. ROS 2 Control Integration (per TODO.md)
As mentioned in your TODO, implement ROS 2 Control framework:
- Create hardware interface for the simulated vehicle
- Implement controllers using ros2_control
- Replace direct plugin control with controller manager
- Use GZ Sim's JointTrajectoryController or similar

**References:**
- https://control.ros.org/jazzy/index.html
- https://github.com/ros-controls/gz_ros2_control

#### 5. Modular Plugin System (per TODO.md)
Make plugins truly modular:
- Separate sensor plugins from vehicle dynamics
- Allow runtime plugin configuration
- Create plugin configuration files
- Document plugin API

### Low Priority

#### 6. Additional Sensor Plugins
Implement remaining sensors from TODO.md:
- Camera sensor (use GZ Sim camera system)
- IMU sensor (use GZ Sim IMU system)
- GPS sensor (use GZ Sim GPS/GNSS system)
- INS fusion plugin (IMU + GPS)

#### 7. Advanced Vehicle Dynamics
- Ackermann steering model (instead of bicycle)
- Tire slip models
- Suspension dynamics
- Road condition simulation (wet/dry grip)

#### 8. World and Model Updates
- Update cone models if needed for GZ Sim
- Create new test tracks
- Add more realistic lighting and textures

## 🔧 Build and Test Instructions

### Prerequisites (Ubuntu 24.04)
```bash
# Install ROS 2 Jazzy
sudo apt update
sudo apt install ros-jazzy-desktop

# Install GZ Sim (Harmonic)
sudo apt install gz-harmonic

# Install ros_gz packages
sudo apt install ros-jazzy-ros-gz ros-jazzy-ros-gz-sim ros-jazzy-ros-gz-bridge
```

### Building
```bash
cd ~/fsae
source /opt/ros/jazzy/setup.bash
colcon build --packages-select vehicle_plugins qutms_sim
```

### Testing
```bash
source ~/fsae/install/setup.bash
ros2 launch qutms_sim sim.launch.py track:=small_track
```

## 📝 Migration Notes

### Key API Changes
1. **Time Handling**: GZ Sim uses `std::chrono::duration` instead of `gazebo::common::Time`
2. **Entity Access**: Use Entity Component Manager (ECM) instead of direct model pointers
3. **Component System**: All data access through components (Pose, LinearVelocity, etc.)
4. **Plugin Registration**: Use `GZ_ADD_PLUGIN` with all implemented interfaces listed
5. **ROS Integration**: Create ROS nodes directly instead of using gazebo_ros wrappers

### Breaking Changes
- Old Gazebo Classic plugins are **incompatible** with GZ Sim
- Launch files need complete rewrite for ros_gz_sim
- Environment variables have changed
- Plugin loading mechanism is different

### Backward Compatibility
To maintain support for Humble/Gazebo Classic:
- Keep backup files (`gazebo_vehicle_classic.cpp.backup`)
- Consider using build flags to switch between versions
- Maintain separate branches for Humble and Jazzy

## 🎯 Next Steps

1. **Immediate**: Complete cone detection plugin migration
2. **Short-term**: Test vehicle plugin on actual Jazzy system
3. **Medium-term**: Implement ROS 2 Control framework
4. **Long-term**: Add advanced features from TODO.md

## 📚 Documentation References
- [GZ Sim Documentation](https://gazebosim.org/docs/harmonic)
- [Migration Guide](./MIGRATION_GUIDE.md)
- [TODO List](./TODO.md)
- [ROS 2 Jazzy Docs](https://docs.ros.org/en/jazzy/)
