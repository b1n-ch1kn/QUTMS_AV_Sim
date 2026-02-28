# Migration Summary: QUTMS_AV_Sim ROS 2 Jazzy Upgrade

## What Was Done

### Core Migration Completed ✅

I've successfully migrated the majority of QUTMS_AV_Sim from ROS 2 Humble/Gazebo Classic to ROS 2 Jazzy/GZ Sim (Harmonic). Here's what was accomplished:

#### 1. **Package Manifests Updated**
- Updated `package.xml` files to use GZ Sim (`gz-sim8`, `ros_gz`) instead of Gazebo Classic
- All ROS dependencies remain compatible with Jazzy

#### 2. **Build System Migrated**
- CMakeLists.txt files updated to find and link GZ Sim libraries:
  - `gz-physics7`, `gz-transport13`, `gz-sensors8`, `gz-plugin2`
- Removed Gazebo Classic dependencies (`gazebo`, `gazebo_dev`, `gazebo_ros`)

#### 3. **Vehicle Plugin Completely Rewritten**
The vehicle dynamics plugin has been fully migrated to GZ Sim's modern architecture:

**Before (Gazebo Classic):**
```cpp
class VehiclePlugin : public gazebo::ModelPlugin {
    void Load(gazebo::physics::ModelPtr model, sdf::ElementPtr sdf);
};
```

**After (GZ Sim):**
```cpp
class VehiclePlugin : public gz::sim::System,
                      public gz::sim::ISystemConfigure,
                      public gz::sim::ISystemPreUpdate {
    void Configure(...);
    void PreUpdate(...);
};
```

**Key changes:**
- Switched from inheritance-based `ModelPlugin` to interface-based `System`
- Replaced event-driven `Load()` with `Configure()` + `PreUpdate()`
- Migrated from `gazebo::physics::ModelPtr` to `gz::sim::Entity` + ECS
- Updated from `gazebo::common::Time` to `std::chrono::duration`
- Changed from `gazebo_ros::Node::Get()` to direct `rclcpp::Node` creation
- All model access now uses Entity Component Manager (ECM)

#### 4. **URDF/Xacro Files Updated**
- Robot description now includes GZ Sim Physics system
- Plugin tags updated with correct GZ Sim format
- LIDAR sensor migrated to use `gz-sim-sensors-system`
- Added `ros_gz_bridge` for sensor data publishing

#### 5. **Launch Files Converted**
- Changed from `gazebo_ros.launch.py` to `ros_gz_sim.launch.py`
- Updated environment variables (`GAZEBO_*` → `GZ_SIM_*`)
- Modified spawn command from `spawn_entity.py` to `create`
- World files now referenced as `.sdf` instead of `.world`

#### 6. **Documentation Created**
I've created comprehensive documentation:
- **JAZZY_MIGRATION_STATUS.md** - Detailed progress tracking
- **MIGRATION_GUIDE.md** - API changes and technical reference
- **QUICKSTART_JAZZY.md** - User-friendly setup and usage guide
- **scripts/rename_worlds.sh** - Helper script for file renaming
- Updated **README.md** with migration notice

### What Remains To Do ⏳

#### Critical (Required for Basic Functionality)

1. **Cone Detection Plugin Migration**
   - The `gazebo_cone_detection_plugin` still uses Gazebo Classic APIs
   - Needs similar rewrite as vehicle plugin
   - Files to update:
     - `vehicle_plugins/gazebo_cone_detection_plugin/include/gazebo_cone_detection_plugin/gazebo_cone_detection.hpp`
     - `vehicle_plugins/gazebo_cone_detection_plugin/src/gazebo_cone_detection.cpp`
     - `vehicle_plugins/gazebo_cone_detection_plugin/include/gazebo_cone_detection_plugin/utils.hpp`

2. **World Files**
   - Rename `.world` → `.sdf` (use provided script)
   - Test all tracks load correctly in GZ Sim
   - Optional: Update SDF version 1.6 → 1.8

3. **Build and Test on Ubuntu 24.04**
   - Install ROS 2 Jazzy and GZ Sim Harmonic
   - Build the workspace
   - Test vehicle dynamics and control
   - Verify sensor data publishing
   - Check TF tree and visualization

#### Important (For Full Feature Parity)

4. **ROS 2 Control Integration** (from TODO.md)
   - Implement hardware interface for simulated vehicle
   - Use `gz_ros2_control` package
   - Create controller configuration
   - Replace direct plugin control with controller manager

5. **Additional Sensors** (from TODO.md)
   - Camera (use GZ Sim camera system)
   - IMU (use GZ Sim IMU system)
   - GPS (use GZ Sim GNSS system)
   - INS fusion plugin

6. **Advanced Features** (from TODO.md)
   - Modular plugin system
   - Ackermann steering model
   - Tire dynamics and slip
   - Road conditions (wet/dry)

## How to Proceed

### Step 1: Environment Setup (Ubuntu 24.04)
```bash
# Install ROS 2 Jazzy
sudo apt update
sudo apt install ros-jazzy-desktop-full

# Install GZ Sim Harmonic
sudo apt install gz-harmonic

# Install ROS-GZ packages
sudo apt install ros-jazzy-ros-gz \
                 ros-jazzy-ros-gz-sim \
                 ros-jazzy-ros-gz-bridge
```

### Step 2: Verify the Migration
```bash
cd ~/fsae
source /opt/ros/jazzy/setup.bash

# Try building
colcon build --packages-select vehicle_plugins qutms_sim

# Check for errors
# Common issues: missing dependencies, API changes
```

### Step 3: Complete Cone Detection Plugin
Use the vehicle plugin migration as a reference:
1. Update header to use GZ Sim System interface
2. Rewrite `Load()` as `Configure()` + `PreUpdate()`
3. Replace world/model access with ECM queries
4. Update cone detection logic for ECS
5. Test cone publishing

### Step 4: Rename World Files
```bash
cd ~/fsae/QUTMS_AV_Sim
chmod +x scripts/rename_worlds.sh
./scripts/rename_worlds.sh
```

### Step 5: Test the Simulator
```bash
source ~/fsae/install/setup.bash
ros2 launch qutms_sim sim.launch.py track:=small_track

# In another terminal, send commands
ros2 topic pub /sim/control/ackermann_cmd ackermann_msgs/msg/AckermannDriveStamped \
  "drive: {speed: 5.0, steering_angle: 0.2, acceleration: 1.0}"
```

## Files Modified

### Created
- `QUTMS_AV_Sim/JAZZY_MIGRATION_STATUS.md`
- `QUTMS_AV_Sim/MIGRATION_GUIDE.md`
- `QUTMS_AV_Sim/QUICKSTART_JAZZY.md`
- `QUTMS_AV_Sim/scripts/rename_worlds.sh`
- `vehicle_plugins/gazebo_vehicle_plugin/src/gazebo_vehicle.cpp` (new version)

### Modified
- `QUTMS_AV_Sim/README.md` (added migration notice)
- `vehicle_plugins/package.xml`
- `vehicle_plugins/CMakeLists.txt`
- `vehicle_plugins/gazebo_vehicle_plugin/CMakeLists.txt`
- `vehicle_plugins/gazebo_cone_detection_plugin/CMakeLists.txt`
- `vehicle_plugins/gazebo_vehicle_plugin/include/gazebo_vehicle_plugin/gazebo_vehicle.hpp`
- `vehicle_plugins/gazebo_vehicle_plugin/include/gazebo_vehicle_plugin/utils.hpp`
- `qutms_sim/package.xml`
- `qutms_sim/urdf/robot.urdf.xacro`
- `qutms_sim/urdf/lidar.urdf.xacro`
- `qutms_sim/launch/sim.launch.py`

### Backed Up
- `vehicle_plugins/gazebo_vehicle_plugin/src/gazebo_vehicle_classic.cpp.backup` (original)

## Quick Reference

### Key API Mappings

| Gazebo Classic | GZ Sim (Harmonic) |
|----------------|-------------------|
| `gazebo::ModelPlugin` | `gz::sim::System + ISystemConfigure + ISystemPreUpdate` |
| `Load(ModelPtr, sdf)` | `Configure(Entity, sdf, ECM, EventMgr)` + `PreUpdate(UpdateInfo, ECM)` |
| `model->WorldPose()` | `ecm.Component<gz::sim::components::Pose>(entity)` |
| `model->SetWorldPose()` | Create/update Pose component in ECM |
| `gazebo::common::Time` | `std::chrono::duration` |
| `gazebo_ros::Node::Get()` | `std::make_shared<rclcpp::Node>()` |
| `GZ_REGISTER_MODEL_PLUGIN` | `GZ_ADD_PLUGIN(MyPlugin, gz::sim::System, ...)` |

### Important Environment Variables
```bash
# GZ Sim
export GZ_VERSION=harmonic
export GZ_SIM_SYSTEM_PLUGIN_PATH=$QUTMS_WS/install/vehicle_plugins/lib:/opt/ros/jazzy/lib
export GZ_SIM_RESOURCE_PATH=$QUTMS_WS/install/qutms_sim/share/qutms_sim/models
```

## Support Resources

1. **Generated Documentation**
   - Start with [QUICKSTART_JAZZY.md](./QUICKSTART_JAZZY.md)
   - Technical details in [MIGRATION_GUIDE.md](./MIGRATION_GUIDE.md)
   - Track progress in [JAZZY_MIGRATION_STATUS.md](./JAZZY_MIGRATION_STATUS.md)

2. **External Resources**
   - [GZ Sim Documentation](https://gazebosim.org/docs/harmonic)
   - [ros_gz packages](https://github.com/gazebosim/ros_gz)
   - [ROS 2 Jazzy Docs](https://docs.ros.org/en/jazzy/)
   - [GZ Community Forum](https://community.gazebosim.org/)

## Notes for Team

### Testing Checklist
- [ ] Builds without errors on Ubuntu 24.04 + ROS 2 Jazzy
- [ ] Vehicle spawns in simulator
- [ ] Odometry published correctly
- [ ] Ackermann commands control vehicle
- [ ] Steering visualization works
- [ ] LIDAR data published
- [ ] Cone detection works (after plugin migration)
- [ ] Reset service functions
- [ ] TF tree is correct
- [ ] RViz visualization works
- [ ] Foxglove connection successful

### Known Limitations
- Cone detection plugin not yet migrated
- World files need renaming (.world → .sdf)
- Lidar bridge may need tuning
- Joint position control might need adjustments
- No ROS 2 Control integration yet

### Future Enhancements (from TODO.md)
See [TODO.md](./TODO.md) for the full feature roadmap. Priority items:
1. ROS 2 Control integration
2. Modular plugin architecture
3. Advanced vehicle dynamics
4. Additional sensor plugins

---

**Questions or Issues?**
- Check [JAZZY_MIGRATION_STATUS.md](./JAZZY_MIGRATION_STATUS.md) for known issues
- Review [MIGRATION_GUIDE.md](./MIGRATION_GUIDE.md) for API details
- Refer to original eufs_sim repository for examples

Good luck with testing and completing the migration! 🚗💨
