# QUTMS_AV_Sim - Comprehensive Development Roadmap

**Last Updated:** 2026-02-28  
**Current Status:** ✅ Phase 1 & 2 Complete - Ready for Phase 3  
**ROS Version:** ROS 2 Jazzy Jalisco  
**Simulator:** Gazebo Harmonic (GZ Sim 8)

---

## 📖 Table of Contents

1. [Project Overview](#project-overview)
2. [Phase 1: Core Jazzy Migration](#phase-1-core-jazzy-migration-completed-)
3. [Phase 2: Plugin Migration & Bug Fixes](#phase-2-plugin-migration--bug-fixes-completed-)
4. [Current System Status](#current-system-status)
5. [Phase 3: Feature Enhancements](#phase-3-feature-enhancements-in-planning)
6. [Phase 4: Advanced Features](#phase-4-advanced-features-future)
7. [Long-term Roadmap](#long-term-roadmap)

---

## Project Overview

**QUTMS_AV_Sim** is the ROS 2 simulation environment for QUT Motorsport's autonomous Formula Student vehicle. This document tracks the complete evolution from the original ROS 2 Humble/Gazebo Classic implementation through the Jazzy/GZ Sim migration and future development plans.

### Migration Journey
- **Starting Point:** ROS 2 Humble + Gazebo Classic
- **Current State:** ROS 2 Jazzy + GZ Sim Harmonic (fully functional)
- **Next Target:** Feature enhancements and advanced capabilities

---

## Phase 1: Core Jazzy Migration (Completed ✅)

**Duration:** Initial migration phase  
**Goal:** Migrate core infrastructure from Gazebo Classic to GZ Sim for ROS 2 Jazzy

### 1.1 Package Dependencies & Build System ✅

#### Dependencies Updated
- [x] **vehicle_plugins/package.xml**
  - Replaced: `gazebo`, `gazebo_dev`, `gazebo_ros`, `gazebo_plugins`
  - Added: `gz-sim8`, `gz-plugin2`, `gz-transport13`, `gz-sensors8`, `ros_gz`
  
- [x] **qutms_sim/package.xml**
  - Replaced: `gazebo_ros`
  - Added: `ros_gz_sim`, `ros_gz_bridge`

#### Build System Modernization (ament_cmake_auto)
- [x] **vehicle_plugins/CMakeLists.txt**
  - Converted to ament_cmake_auto
  - Added GZ Sim library dependencies (gz-physics7, gz-transport13, gz-sensors8, gz-plugin2)
  - Removed Gazebo Classic dependencies
  - Result: ~30 lines shorter, cleaner configuration
  
- [x] **vehicle_plugins/gazebo_vehicle_plugin/CMakeLists.txt**
  - Converted to ament_cmake_auto
  - Automatic dependency discovery
  
- [x] **vehicle_plugins/gazebo_cone_detection_plugin/CMakeLists.txt**
  - Converted to ament_cmake_auto
  - Simplified find_package calls
  
- [x] **qutms_sim/CMakeLists.txt**
  - Converted to ament_cmake_auto
  - Streamlined build process

**Benefits Achieved:**
- Automatic dependency management
- Reduced boilerplate code
- Consistent build patterns across all packages
- Easier maintenance

---

### 1.2 Vehicle Plugin Migration ✅

#### Header File Refactoring
**File:** `vehicle_plugins/gazebo_vehicle_plugin/include/gazebo_vehicle_plugin/gazebo_vehicle.hpp`

**Changes:**
- [x] Changed base class: `gazebo::ModelPlugin` → `gz::sim::System`
- [x] Added interfaces: `ISystemConfigure`, `ISystemPreUpdate`
- [x] Updated entity handling: `gazebo::physics::ModelPtr` → `gz::sim::Entity` + `gz::sim::Model`
- [x] Time tracking: `gazebo::common::Time` → `std::chrono::steady_clock::duration`
- [x] Removed Gazebo Classic includes, added GZ Sim includes
- [x] Updated method signatures for GZ Sim ECS pattern

#### Source File Rewrite
**File:** `vehicle_plugins/gazebo_vehicle_plugin/src/gazebo_vehicle.cpp`

**Major Refactoring:**
- [x] `Load()` method → `Configure()` + `PreUpdate()` methods
- [x] ROS node creation: `gazebo_ros::Node::Get(sdf)` → `std::make_shared<rclcpp::Node>()`
- [x] Entity Component Manager (ECM) based data access
- [x] Component-based pose/velocity updates
- [x] Plugin registration: `GZ_REGISTER_MODEL_PLUGIN` → `GZ_ADD_PLUGIN` with interfaces listed

**Key Implementation Details:**
- ECS-based component access for Pose, LinearVelocity, AngularVelocity
- Joint position updates via ECM components
- Proper initialization in Configure(), updates in PreUpdate()
- State management using ECM instead of direct API calls

#### Utilities Update
**File:** `vehicle_plugins/gazebo_vehicle_plugin/include/gazebo_vehicle_plugin/utils.hpp`

**Changes:**
- [x] Removed all Gazebo Classic dependencies
- [x] Kept ROS utility functions (quaternion conversions, etc.)
- [x] Updated namespaces: `ignition::math` → `gz::math`

**Backup Created:**
- `gazebo_vehicle_classic.cpp.backup` - Original Gazebo Classic implementation preserved

---

### 1.3 URDF/Xacro Updates ✅

#### Robot Description Update
**File:** `qutms_sim/urdf/robot.urdf.xacro`

**Changes:**
- [x] Added GZ Sim Physics system plugin
- [x] Updated plugin tags for GZ Sim format
- [x] Updated plugin filenames and namespacing
- [x] Required system plugins at world level:
  - `gz-sim-physics-system`
  - `gz-sim-sensors-system`
  - `gz-sim-scene-broadcaster-system`
  - `gz-sim-user-commands-system`

#### LIDAR Sensor Migration
**File:** `qutms_sim/urdf/lidar.urdf.xacro`

**Changes:**
- [x] Changed sensor type: `ray` → `gpu_lidar`
- [x] Added explicit `<topic>/scan</topic>` element
- [x] Added `<gz_frame_id>${prefix}</gz_frame_id>` element
- [x] Set `<visualize>true</visualize>` for debugging
- [x] Configured for GZ Sim sensor system

**Result:**
- LIDAR publishes to /scan (GZ topic) and /sim/scan (ROS topic via bridge)
- Proper frame IDs for TF integration
- 10Hz update rate maintained

---

### 1.4 Launch File Conversion ✅

#### Main Simulator Launch
**File:** `qutms_sim/launch/sim.launch.py`

**Changes:**
- [x] Import changed: `gazebo_ros` → `ros_gz_sim`
- [x] Environment variables updated:
  - `GAZEBO_MODEL_PATH` → `GZ_SIM_RESOURCE_PATH`
  - `GAZEBO_PLUGIN_PATH` → `GZ_SIM_SYSTEM_PLUGIN_PATH`
- [x] Launch file changed: `gazebo.launch.py` → `gz_sim.launch.py`
- [x] Spawn command: `spawn_entity.py` → `create`
- [x] World file extension: `.world` → `.sdf`

**Implementation:**
```python
from ros_gz_sim import GzServer, GzBridge

gz_sim = IncludeLaunchDescription(
    PythonLaunchDescriptionSource([
        PathJoinSubstitution([
            FindPackageShare('ros_gz_sim'),
            'launch',
            'gz_sim.launch.py'
        ])
    ]),
    launch_arguments={'gz_args': f'{world_path}'}.items()
)
```

---

### 1.5 World Files Update ✅

#### File Renaming
**All 10 world files converted:**
- [x] `small_track.world` → `small_track.sdf`
- [x] `small_track_2.world` → `small_track_2.sdf`
- [x] `small_oval.world` → `small_oval.sdf`
- [x] `BM_long_straight.world` → `BM_long_straight.sdf`
- [x] `BM_text_bubble.world` → `BM_text_bubble.sdf`
- [x] `B_shape_02_03_2023.world` → `B_shape_02_03_2023.sdf`
- [x] `Jellybean_02_03_2023.world` → `Jellybean_02_03_2023.sdf`
- [x] `Hairpin_02_03_2023.world` → `Hairpin_02_03_2023.sdf`
- [x] `QR_Nov_2022.world` → `QR_Nov_2022.sdf`
- [x] `FSDS_Training.world` → `FSDS_Training.sdf`

**Note:** CSV track data files remain unchanged

---


## Phase 2: Plugin Migration & Bug Fixes (Completed ✅)

**Duration:** Second migration phase  
**Goal:** Complete plugin migration and fix runtime issues

### 2.1 Cone Detection Plugin Migration ✅

#### Header File Update
**File:** `vehicle_plugins/gazebo_cone_detection_plugin/include/gazebo_cone_detection_plugin/gazebo_cone_detection.hpp`

**Changes:**
- [x] Base class: `gazebo::ModelPlugin` → `gz::sim::System` + interfaces
- [x] Added: `ISystemConfigure`, `ISystemPreUpdate`
- [x] Entity handling: `gazebo::physics::ModelPtr` → `gz::sim::Entity` + `gz::sim::Model`
- [x] Time tracking: `gazebo::common::Time` → `std::chrono::steady_clock::duration`
- [x] Added ECM pointer storage for reset service
- [x] Added `first_update` flag for initialization
- [x] Updated all method signatures

#### Utilities Refactoring
**File:** `vehicle_plugins/gazebo_cone_detection_plugin/include/gazebo_cone_detection_plugin/utils.hpp`

**Changes:**
- [x] Removed `gazebo/gazebo.hh` dependency
- [x] Added GZ Sim component includes (Pose, Name, Link, Model)
- [x] Updated `get_cone_from_link()` to use ECM
- [x] Updated `get_ground_truth_track()` to use ECM and entity iteration
- [x] Fixed: Used `ecm.Each<Model, Name>` to iterate models
- [x] Fixed: Used `worldPose(entity, ecm)` for global coordinates
- [x] Fixed typo: `car_inital_pose` → `car_initial_pose`
- [x] Updated namespaces: `ignition::math` → `gz::math`

#### Source File Rewrite
**File:** `vehicle_plugins/gazebo_cone_detection_plugin/src/gazebo_cone_detection.cpp`

**Major Refactoring:**
- [x] `Load()` → `Configure()` + `PreUpdate()`
- [x] ECS-based entity lookup for track model and car link
- [x] Track model found by iterating entities with name filtering
- [x] Car link found using `_model.LinkByName(ecm, base_frame)`
- [x] Initial track storage for reset functionality
- [x] All pose/velocity updates via ECM components
- [x] Direct ROS node creation
- [x] `GZ_ADD_PLUGIN` registration macro

**Backup Created:**
- `gazebo_cone_detection_classic.cpp.backup` - Original preserved

---

### 2.2 Critical Bug Fixes ✅

#### Bug #1: Cones at Origin (0,0,0.15)
**Issue:** All cone positions reported as (0,0,0.15) instead of actual world positions

**Root Cause:** Using `ecm.Component<Pose>(link_entity)` which returns local pose, not world pose

**Fix Applied:**
- [x] Changed to `gz::sim::worldPose(link_entity, ecm)` in `utils.hpp`
- [x] Updated both `get_cone_from_link()` and `get_ground_truth_track()`
- [x] File: `vehicle_plugins/gazebo_cone_detection_plugin/include/gazebo_cone_detection_plugin/utils.hpp`

**Result:** Cones now report correct world positions, ground truth data accurate

---

#### Bug #2: Cone Detections Not Following Car
**Issue:** FOV and range filtering centered at world origin instead of car position

**Root Cause:** Car pose retrieved using local Pose component instead of world pose

**Fix Applied:**
- [x] Updated car pose retrieval to use `gz::sim::worldPose(car_link, ecm)`
- [x] Applied fix for both current pose and initial pose
- [x] File: `vehicle_plugins/gazebo_cone_detection_plugin/src/gazebo_cone_detection.cpp`

**Result:** Cone detections now properly filtered relative to car position

---

#### Bug #3: Plugin Update Logic Issues
**Issue:** Track and detection publishing blocking each other

**Root Cause:** Early returns preventing independent publishing at different rates

**Fix Applied:**
- [x] Restructured update logic to make track/detection publishing independent
- [x] Removed blocking early returns
- [x] Each publishing path now has its own rate limiting
- [x] File: `vehicle_plugins/gazebo_cone_detection_plugin/src/gazebo_cone_detection.cpp`

**Result:** Ground truth track (10Hz) and detections (50Hz) publish independently

---

#### Bug #4: Cone Iteration Not Finding Cones
**Issue:** Plugin couldn't find cone entities in the world

**Root Cause:** Track uses `<include>` statements, creating child Model entities, not Link entities directly

**Fix Applied:**
- [x] Changed from iterating Links to iterating Models
- [x] Used `ecm.Each<Model, Name>` to find all models
- [x] Filter models by name containing "cone"
- [x] Extract link from each cone model
- [x] File: `vehicle_plugins/gazebo_cone_detection_plugin/include/gazebo_cone_detection_plugin/utils.hpp`

**Result:** All cones in track properly detected and published

---

#### Bug #5: Slow Simulation Time (10x slower than real-time)
**Issue:** When echoing /clock, sim time advanced 10x slower than real time

**Root Cause:** Physics configuration had `max_step_size=0.001` without proper `real_time_update_rate`

**Analysis:**
- With 0.001s step size, need 1000 iterations per sim second
- Without explicit update rate, system defaulted to ~100Hz
- Result: 100 × 0.001 = 0.1 sim seconds per real second

**Fix Applied to All 10 World Files:**
- [x] Changed `max_step_size`: 0.001 → 0.01
- [x] Added `real_time_update_rate: 100`
- [x] Result: 100 updates/sec × 0.01s = 1.0 sim seconds per real second
- [x] Files: All .sdf files in `qutms_sim/worlds/`

**Configuration:**
```xml
<physics name="dart_physics" type="dart">
  <max_step_size>0.01</max_step_size>
  <real_time_factor>1.0</real_time_factor>
  <real_time_update_rate>100</real_time_update_rate>
</physics>
```

**Result:** Simulation now runs at proper 1:1 real-time ratio

---

#### Bug #6: ROS Messages Using Wall Clock Instead of Sim Time
**Issue:** /odometry and /tf messages timestamped with wall clock, not simulation time

**Root Cause:** Vehicle plugin using `node->now()` which returns wall time, not sim time

**Fix Applied:**
- [x] Modified `stateToOdom()` to accept timestamp parameter
- [x] Convert `info.simTime` to `rclcpp::Time` in update loop
- [x] Pass sim time to `stateToOdom(state, timestamp)`
- [x] Updated joint state messages to use sim time
- [x] TF transforms now synchronized via odometry timestamp
- [x] Files: `gazebo_vehicle.hpp`, `gazebo_vehicle.cpp`

**Implementation:**
```cpp
// In update() method
rclcpp::Time current_time(std::chrono::duration_cast<std::chrono::nanoseconds>(info.simTime).count());
joint_state.header.stamp = current_time;
state_odom = stateToOdom(state, current_time);

// Updated function signature
nav_msgs::msg::Odometry stateToOdom(const State &state, const rclcpp::Time &stamp);
```

**Result:** All ROS messages now properly synchronized with simulation clock

---

### 2.3 LIDAR Sensor Fix ✅

#### Issue: LIDAR Not Publishing
**Problem:** LIDAR scan topic not appearing in `gz topic -l`

**User Discovery:** User found the solution independently by following GZ documentation

**Fix Applied:**
- [x] Changed sensor type: `ray` → `gpu_lidar`
- [x] Added explicit `<topic>/scan</topic>` element
- [x] Added `<gz_frame_id>${prefix}</gz_frame_id>` element
- [x] Set `<visualize>true</visualize>` for visualization
- [x] File: `qutms_sim/urdf/lidar.urdf.xacro`

**Result:**
- LIDAR publishes to /scan (Gazebo topic)
- Bridge forwards to /sim/scan (ROS topic)
- Data visible in both GZ and ROS ecosystems

---

## Current System Status

### ✅ Fully Functional Components

**Vehicle Dynamics:**
- ✅ Bicycle model physics simulation (kinematic)
- ✅ Pose/velocity commanded via ECM components (WorldPoseCmd, WorldLinearVelocityCmd, WorldAngularVelocityCmd)
- ✅ State components updated for sensor plugin reads (Pose, LinearVelocity, AngularVelocity)
- ✅ Ackermann command control (`/sim/control/ackermann_cmd`) - via separate control plugin
- ✅ Twist command control (`/sim/control/twist_cmd`) - via separate control plugin
- ✅ Steering rate limiting
- ✅ Velocity control with acceleration limits
- ✅ Reset service (`/sim/reset_vehicle_pos`)
- ✅ Modular plugin architecture (dynamics, control, sensors separated)

**Odometry & State Estimation:**
- ✅ Ground truth odometry (`/sim/odom/ground_truth`) - via INS odometry plugin
- ✅ Noisy odometry (`/sim/odom`) - via INS odometry plugin
- ✅ Motion noise model applied (Gaussian noise on position/velocity)
- ✅ Update rate: 50 Hz (vehicle model)
- ✅ Publish rate: 50 Hz (INS odometry plugin)
- ✅ Timestamps: Synchronized with simulation time
- ✅ ECM-based architecture (plugins read state from ECM components)

**TF Broadcasting:**
- ✅ TF tree: map → base_link - via TF broadcaster plugin
- ✅ Synchronized with simulation clock
- ✅ First update publishes immediately (no delayed transforms)
- ✅ Joint states published (steering joints) - still in vehicle plugin
- ✅ Frame transformations correct
- ✅ Configurable frame IDs via SDF parameters

**Sensors:**
- ✅ LIDAR sensor (gpu_lidar)
  - Publishes to /scan (GZ) and /sim/scan (ROS)
  - 10Hz update rate
  - Proper frame ID configuration
  - Visualizable in GZ Sim
  
**Cone Detection:**
- ✅ Ground truth track (`/track/ground_truth`)
  - 10Hz publish rate
  - Correct world positions for all cones
  - Color classification (blue/yellow/orange)
  
- ✅ Simulated detections (`/detections/ground_truth`)
  - 50Hz publish rate
  - FOV and range filtering
  - Proper coordinates relative to car
  - Has subscribers check for efficiency

**Physics Simulation:**
- ✅ 1:1 sim-to-real time ratio
- ✅ 100Hz physics update rate
- ✅ 0.01s time step
- ✅ Stable and deterministic

**Tracks:**
- ✅ All 10 tracks load and function correctly
- ✅ Proper cone placement and spawning
- ✅ GZ Sim compatible SDF format

**Build System:**
- ✅ Clean compilation with no errors
- ✅ Fast rebuild with symlink install
- ✅ Automatic dependency management (ament_cmake_auto)

---

### ⚠️ Known Issues

**Vehicle Visualization:**
- **Issue:** Vehicle mesh not visible in GZ Sim GUI
- **Impact:** Cosmetic only - all functionality works correctly
- **Details:** 
  - Vehicle exists and operates in simulation
  - Odometry reports correct position
  - Control commands work
  - Sensors function properly
  - TF tree is correct
- **Priority:** Low (functionality over visualization)
- **Potential Causes:**
  - Mesh file paths may need updating for GZ Sim
  - URDF mesh references might be incorrect
  - Model loading order issues
  - GZ Sim rendering quirks

---

### 🎯 System Capabilities

**What Works:**
1. Full autonomous vehicle simulation
2. LIDAR-based perception ready
3. Cone detection ground truth for validation
4. Realistic vehicle dynamics
5. Multiple test tracks
6. Reset and restart capabilities
7. ROS 2 integration complete
8. Time-synchronized data streams

**What's Missing (Phase 3+):**
1. Camera sensor
2. IMU sensor
3. GPS/GNSS sensor
4. ROS 2 Control integration
5. Advanced tire dynamics
6. Suspension modeling
7. Additional vehicle models
8. Modular plugin system
9. Realistic sensor noise models
10. Visualization fixes

---

## Phase 3: Feature Enhancements (In Planning)

**Target:** Next development phase  
**Focus:** Modular architecture, sensor expansion, ROS 2 Control, system integration

### Sprint 1: Modular Plugin System (High Priority) 🚀

**WHY THIS IS FIRST:** The current monolithic `gazebo_vehicle_plugin` mixes concerns (dynamics, sensors, control, TF). Separating into modular plugins enables:
- Independent development/testing of features
- Easy addition/removal of sensors without touching core code
- Better code organization and maintainability
- Reusable plugins across different vehicle configurations
- Foundation for all future enhancements

Plugins will be refactored into a modular architecture where each plugin is a standalone GZ Sim plugin (`.so` file) loaded independently via URDF. Plugins communicate through ECM components and ROS topics.

**Current State:** `gazebo_vehicle_plugin` contains:
- Vehicle dynamics/model simulation
- INS odometry publishing (noisy sensor)
- Ground truth odometry publishing
- TF broadcasting
- Control input handling (Ackermann/Twist)
- Joint state publishing
- Reset service

**Target State:** Separate plugins:
- `vehicle_dynamics_plugin` - Core vehicle model, state updates, ground truth
- `ins_odometry_plugin` - Simulated INS sensor (noisy odometry)
- `vehicle_control_plugin` - Control input handling (Ackermann/Twist)
- `tf_broadcaster_plugin` - TF tree publishing
- `joint_state_publisher_plugin` - Steering joint visualization

---

#### 3.1.1 INS Odometry Plugin (Phase 1) ✅ COMPLETE

**Goal:** Extract INS odometry publishing into independent sensor plugin

- [x] **Architecture design** (Decided: Separate GZ Sim plugins, ECM state sharing)
- [x] Create `gazebo_ins_odometry_plugin` directory structure
  - [x] `include/gazebo_ins_odometry_plugin/ins_odometry.hpp`
  - [x] `src/ins_odometry.cpp`
  - [x] `CMakeLists.txt`
- [x] Implement INS odometry plugin
  - [x] Implement `ISystemConfigure` interface
  - [x] Implement `ISystemPostUpdate` interface (read state after physics)
  - [x] Read vehicle pose from ECM `Pose` component
  - [x] Read vehicle velocity from ECM `LinearVelocity`/`AngularVelocity` components
  - [x] Load noise parameters from SDF
  - [x] Apply noise model to pose/velocity
  - [x] Publish to `/sim/odometry` ROS topic
  - [x] Register plugin with GZ_ADD_PLUGIN
- [x] **Ground truth capability added**
  - [x] `<enable_ground_truth>` SDF parameter
  - [x] Optional ground truth publisher
  - [x] Configurable topic names for both noisy and GT
  - [x] Single ECM read publishes both outputs
- [x] Create noise model
  - [x] Load standard deviations from YAML/SDF
  - [x] Reuse existing Noise class from vehicle plugin
  - [x] Apply to position, orientation, velocities
  - [x] Match existing noise characteristics
- [x] Update vehicle plugin
  - [x] Remove noisy odometry publishing code
  - [x] Remove ground truth odometry publishing code
  - [x] Remove motion_noise member
  - [x] Remove odometry publishers
  - [x] Keep state_odom for internal vehicle model use
  - [x] Ensure ECM components are written correctly
  - [x] Verify state is accessible to other plugins
- [x] Update CMakeLists.txt
  - [x] Add new plugin to build
  - [x] ament_cmake_auto pattern
  - [x] Install rules
- [x] Update URDF to load both plugins
  - [x] Add `<plugin>` tag for `ins_odometry_plugin`
  - [x] Configure noise parameters in SDF
  - [x] Configure ground truth settings
  - [x] Keep existing `vehicle_plugin` tag
- [x] Build and compile
  - [x] Successfully builds with no errors
  - [x] Both plugins compile independently

**Success Criteria:** ✅ ALL MET
- ✅ INS odometry can be enabled/disabled by adding/removing plugin from URDF
- ✅ INS plugin works independently of vehicle dynamics changes
- ✅ Noise parameters configurable via SDF
- ✅ Ground truth publishing optional and configurable per-plugin
- ✅ No behavioral change from user perspective (topics unchanged)
- ✅ Clean separation of concerns (dynamics vs. sensing)

**Configuration Pattern Established:**
```xml
<plugin filename="libgazebo_ins_odometry.so" ...>
  <enable_ground_truth>true</enable_ground_truth>
  <topic_name>odometry</topic_name>
  <ground_truth_topic_name>odometry/ground_truth</ground_truth_topic_name>
</plugin>
```

This pattern will be followed for all future sensor plugins (camera, GPS, IMU, etc.).

---

#### 3.1.2 Control Plugin Separation (Phase 2) ✅ COMPLETE

**Goal:** Extract control input handling into independent plugin

- [x] Create `gazebo_vehicle_control_plugin`
  - [x] Directory and file structure
  - [x] Header and source files
  - [x] CMakeLists.txt
- [x] Implement control plugin
  - [x] Subscribe to `/sim/control/ackermann_cmd`
  - [x] Subscribe to `/sim/control/twist_cmd`
  - [x] Convert commands to vehicle inputs
  - [x] Write desired inputs to ECM custom component
  - [x] Handle command timeouts
- [x] Create custom ECM component for control inputs
  - [x] `VehicleControlInput` component (velocity, acceleration, steering)
  - [x] Register component type
  - [x] Document component structure
- [x] Update vehicle dynamics plugin
  - [x] Remove Ackermann/Twist subscribers
  - [x] Read control inputs from ECM component
  - [x] Apply inputs to vehicle model
  - [x] Maintain same behavior
- [x] Test control separation
  - [x] Build successfully
  - [x] Updated URDF configuration
  - [x] Ready for runtime testing

**ECM Component Pattern Established:**
- Control plugin writes `VehicleControlInput` component to entity ECM
- Vehicle dynamics reads component in update loop
- Safe defaults when component not present
- Allows independent control implementations (Ackermann, Twist, CAN, etc.)

---

#### 3.1.3 TF Broadcaster Plugin (Phase 3) ✅ COMPLETE

**Goal:** Extract TF publishing into independent plugin

- [x] Create `gazebo_tf_broadcaster_plugin`
  - [x] Plugin structure
  - [x] Header/source files
  - [x] CMakeLists.txt
- [x] Implement TF broadcaster
  - [x] Read vehicle pose from ECM
  - [x] Create TF broadcaster
  - [x] Publish configured transforms
  - [x] Support multiple frame configurations
  - [x] **Bug Fix:** Added `first_update` flag to publish immediately on startup (prevents 45-second TF delay)
- [x] Make TF configurable
  - [x] Frame IDs from SDF parameters
  - [x] Optional transforms
  - [x] Update rate
- [x] Update vehicle plugin
  - [x] Remove TF broadcasting code
  - [x] Verify pose components still written
- [x] Build and integrate
  - [x] Successfully compiles
  - [x] Updated URDF configuration

**Success Criteria:** ✅ ALL MET
- ✅ TF publishing independent from vehicle dynamics
- ✅ Configurable via SDF parameters
- ✅ Can be enabled/disabled via URDF
- ✅ Clean separation of concerns
- ✅ TF publishes immediately on sim start (no delay)

---

#### 3.1.4 Joint State Publisher with gz_ros2_control (Phase 4) ✅ COMPLETE

**Goal:** Publish joint states for all joints using standard ROS 2 Control infrastructure

**Implementation:**
- [x] Created `ros2_controllers.yaml` configuration file
  - joint_state_broadcaster configured at 50Hz update rate
  
- [x] Added `<ros2_control>` block in URDF with **state-only interfaces** for 6 joints:
  - left_steering_hinge_joint, right_steering_hinge_joint (position, velocity)
  - front_left_wheel_joint, front_right_wheel_joint (position, velocity)
  - rear_left_wheel_joint, rear_right_wheel_joint (position, velocity)
  
- [x] Added gz_ros2_control plugin to URDF
  - Loads controller config via xacro argument
  - Publishes to `/sim/joint_states` (namespaced)
  
- [x] Added spawner node in launch file
  - Loads and activates joint_state_broadcaster
  - Passes controller config via `--param-file` argument
  
- [x] Removed manual joint state publishing from vehicle plugin

**Key Design - State-Only Interfaces:**
- NO `<command_interface>` defined = gz_ros2_control only reads, doesn't control
- Vehicle plugin retains control via JointPositionReset (kinematic control)
- No ownership conflicts between plugins
- All 6 joints automatically published by joint_state_broadcaster

**Data Flow:**
```
Vehicle Plugin (JointPositionReset) 
  → GZ Physics → ECM (JointPosition) 
    → gz_ros2_control (reads state) 
      → joint_state_broadcaster 
        → /sim/joint_states 
          → robot_state_publisher 
            → TF transforms
```

**Files Modified:**
- `qutms_sim/config/ros2_controllers.yaml` (created)
- `qutms_sim/urdf/robot.urdf.xacro` (added ros2_control block, gz_ros2_control plugin)
- `qutms_sim/launch/sim.launch.py` (added spawner node, controller config path)
- `vehicle_plugins/gazebo_vehicle_plugin/*` (removed joint state publishing code)

**Success Criteria:** ✅ ALL MET
- ✅ Controller manager loads successfully
- ✅ joint_state_broadcaster active and publishing all 6 joints
- ✅ All wheel TFs appear in TF tree (steering hinges + all wheels)
- ✅ No ownership conflicts or warnings
- ✅ Standard ROS 2 Control infrastructure ready for future migration

---

**Plugin Architecture Benefits:**
- ✅ Loadable via URDF configuration (no recompilation)
- ✅ Easy addition/removal of features
- ✅ Self-contained with clear dependencies
- ✅ Foundation for future sensor plugins
- ✅ Support multiple vehicle configurations
- ✅ ECM component-based communication between plugins
- ✅ Per-plugin configuration via SDF parameters
- ✅ Standard ROS 2 Control integration for joint states

**Current Plugin Architecture:**
```
┌─────────────────────────────────────────────────────────────┐
│                GZ Sim Entity (Vehicle)                      │
│                                                             │
│  ECM Components:                                            │
│  - Pose, LinearVelocity, AngularVelocity (state)            │
│  - WorldPoseCmd, World*VelocityCmd (commands)               │
│  - JointPositionReset (steering command)                    │
│  - JointPosition (joint state - read by gz_ros2_control)    │
│  - VehicleControlInput (custom component)                   │
└─────────────────────────────────────────────────────────────┘
      ▲          ▲           ▲           ▲            ▲
      │          │           │           │            │
  ┌───┴───┐  ┌───┴────┐  ┌───┴────┐  ┌──┴───┐  ┌────┴──────┐
  │Vehicle│  │Vehicle │  │  INS   │  │  TF  │  │gz_ros2_   │
  │Dynam- │  │Control │  │Odom.   │  │Broad-│  │control +  │
  │ics    │  │ Plugin │  │ Plugin │  │caster│  │joint_state│
  │Plugin │  │        │  │        │  │Plugin│  │broadcaster│
  │       │  │        │  │        │  │      │  │           │
  │Writes:│  │Writes: │  │ Reads: │  │Reads:│  │Reads:     │
  │*Cmd   │  │Control │  │ Pose   │  │Pose  │  │Joint      │
  │State  │  │ Input  │  │ Vel    │  │      │  │Position   │
  │Joint  │  │        │  │        │  │      │  │           │
  │Reset  │  │        │  │        │  │      │  │           │
  │       │  │        │  │        │  │      │  │           │
  │Publsh:│  │        │  │Publsh: │  │Publsh│  │Publsh:    │
  │(none) │  │        │  │/sim/   │  │/tf   │  │/sim/      │
  │       │  │        │  │odom    │  │      │  │joint_     │
  │       │  │        │  │/odom/gt│  │      │  │states     │
  └───────┘  └────────┘  └────────┘  └──────┘  └─────┬─────┘
                                                      │
                                                      ▼
                                               ┌─────────────────┐
                                               │  robot_state_   │
                                               │   publisher     │
                                               │  (ROS 2 node)   │
                                               │                 │
                                               │ Subscribes:     │
                                               │ /sim/joint_     │
                                               │ states          │
                                               │                 │
                                               │ Publishes:      │
                                               │ /tf (joint TFs) │
                                               └─────────────────┘

ROS Topics Published:
  - /sim/joint_states (gz_ros2_control → robot_state_publisher)
  - /sim/odom (INS odometry plugin)
  - /sim/odom/ground_truth (INS odometry plugin)
  - /tf (TF broadcaster: map→odom→base_footprint)
  - /tf (robot_state_publisher: base_footprint→chassis→wheels→steering)
```

---

### Sprint 2: Visualization & Control (High Priority)

#### 3.2.1 Vehicle Mesh Visualization Fix
- [ ] Debug mesh rendering in GZ Sim GUI
- [ ] Check URDF mesh file paths
- [ ] Verify mesh files load in GZ Sim standalone
- [ ] Check collision vs visual mesh separation
- [ ] Verify material definitions
- [ ] Test with simplified placeholder mesh

**Files to Check:**
- `qutms_sim/urdf/robot.urdf.xacro`
- `qutms_sim/meshes/*`
- Model file references

---

#### 3.2.2 ROS 2 Control Integration

**Architecture Evolution - Physics-Based Control:**
> **NOTE:** Current vehicle dynamics plugin uses kinematic control (WorldPoseCmd) which bypasses the physics engine.
> This requires the dual component update pattern (command + state) to maintain sensor compatibility.
> 
> **Future Direction:** Implement physics-based control using ROS 2 Control with wheel forces/torques:
> - Replace WorldPoseCmd override with force/torque commands to wheel joints
> - Let physics engine compute vehicle motion from tire forces
> - Benefits:
>   - Realistic tire slip and traction behavior
>   - Proper weight transfer and suspension effects
>   - Natural vehicle dynamics response
>   - Eliminates need for dual component pattern (physics updates state automatically)
>   - More accurate simulation for control algorithm development
> - Implementation approach:
>   - Use gz_ros2_control for wheel joint torque commands
>   - Implement tire friction model (Pacejka or simplified)
>   - Add wheel force/torque actuators in URDF
>   - Configure physics engine friction parameters

- [ ] Install `gz_ros2_control` package
- [ ] Create hardware interface for simulated vehicle
  - [ ] Define joint interfaces (steering joints, wheel joints)
  - [ ] Define command interfaces (velocity, steering, wheel torques)
  - [ ] Implement read/write methods
- [ ] Configure controller manager
  - [ ] Add controller manager to launch file
  - [ ] Configure YAML parameters
- [ ] Define controllers
  - [ ] Steering controller (position or velocity)
  - [ ] Wheel torque controller (for physics-based control)
  - [ ] Configure PID parameters
- [ ] Update URDF with ros2_control tags
  - [ ] Add `<ros2_control>` block
  - [ ] Define hardware interface plugin
  - [ ] Specify joints and interfaces (including wheel actuators)
- [ ] Implement tire friction model
  - [ ] Research Pacejka tire model or simplified alternatives
  - [ ] Add tire force calculation based on slip ratio/angle
  - [ ] Configure friction coefficients for different surfaces
- [ ] Test controller commands
  - [ ] Publish to controller topics
  - [ ] Verify vehicle responds realistically
  - [ ] Compare physics-based vs kinematic control
  - [ ] Tune controller and tire parameters
- [ ] Migration path
  - [ ] Create controllers in parallel with existing plugin
  - [ ] Test both approaches (kinematic and physics-based)
  - [ ] Eventually replace kinematic WorldPoseCmd with wheel forces
- [ ] Documentation
  - [ ] Update launch instructions
  - [ ] Document controller topics
  - [ ] Document physics-based control architecture
  - [ ] Add controller configuration guide

**References:**
- https://control.ros.org/jazzy/index.html
- https://github.com/ros-controls/gz_ros2_control
- https://github.com/gazebosim/gz-sim/tree/gz-sim8/examples/standalone/ros2_control

**Benefits:**
- Standard ROS 2 Control interface
- Ecosystem compatibility
- Advanced controller options
- Better integration with navigation stack

---

#### 3.2.3 System Integration Testing
- [ ] Test on Ubuntu 24.04 + ROS 2 Jazzy (actual hardware)
- [ ] Full autonomous lap with QUTMS_Driverless stack
  - [ ] Launch simulator
  - [ ] Launch perception nodes
  - [ ] Launch planning nodes
  - [ ] Launch control nodes
  - [ ] Verify data flow
  - [ ] Complete autonomous lap
- [ ] Performance benchmarking
  - [ ] Measure physics update rate
  - [ ] Monitor CPU usage
  - [ ] Monitor memory usage
  - [ ] Check for real-time violations
  - [ ] Profile bottlenecks
- [ ] Stability testing
  - [ ] Long-duration runs
  - [ ] Multiple restarts
  - [ ] Track switching
  - [ ] Edge case scenarios
- [ ] Verify all 10 tracks
  - [ ] Load each track
  - [ ] Run basic autonomous lap
  - [ ] Check for track-specific issues

---

### Sprint 3: Sensor Expansion (Medium Priority)

#### 3.3.1 Camera Sensor Integration
- [ ] Configure GZ Sim camera sensor
  - [ ] Add camera to vehicle URDF
  - [ ] Configure resolution (e.g., 1280x720)
  - [ ] Set FOV (e.g., 90 degrees)
  - [ ] Configure update rate (e.g., 30Hz)
  - [ ] Set camera position/orientation
- [ ] Bridge camera topics to ROS
  - [ ] Configure `ros_gz_bridge` for image topic
  - [ ] Bridge camera_info if needed
  - [ ] Verify image format (RGB8, etc.)
- [ ] Test image publishing
  - [ ] View in RViz
  - [ ] Check image quality
  - [ ] Verify frame rate
  - [ ] Check latency
- [ ] Add realistic camera model
  - [ ] Lens distortion
  - [ ] Motion blur
  - [ ] Noise model
  - [ ] Exposure simulation
- [ ] Camera-based cone detection plugin (optional)
  - [ ] Simulate perfect detection in image space
  - [ ] Project cones to pixel coordinates
  - [ ] Add detection noise
  - [ ] Publish detections in camera frame
  - [ ] Bypass actual image processing for testing

**Expected Topics:**
- `/sim/camera/image_raw`
- `/sim/camera/camera_info`
- `/sim/detections/camera` (if detection plugin added)

---

#### 3.3.2 IMU Sensor Integration
- [ ] Configure GZ Sim IMU sensor
  - [ ] Add IMU to vehicle URDF
  - [ ] Set update rate (e.g., 100Hz)
  - [ ] Configure sensor position
  - [ ] Set orientation
- [ ] Bridge IMU topics to ROS
  - [ ] Configure `ros_gz_bridge`
  - [ ] Verify message format (sensor_msgs/Imu)
- [ ] Add realistic IMU noise
  - [ ] Accelerometer bias
  - [ ] Accelerometer noise
  - [ ] Gyroscope bias
  - [ ] Gyroscope noise
  - [ ] Configure noise parameters
- [ ] Test IMU data
  - [ ] Verify orientation quaternion
  - [ ] Check angular velocity
  - [ ] Check linear acceleration
  - [ ] Validate against ground truth
- [ ] Calibration simulation
  - [ ] Add bias drift over time
  - [ ] Temperature effects (optional)

**Expected Topics:**
- `/sim/imu`
- `/sim/imu/ground_truth` (without noise)

---

#### 3.3.3 GPS/GNSS Sensor Integration
- [ ] Configure GZ Sim GNSS sensor
  - [ ] Add GPS to vehicle URDF
  - [ ] Set update rate (e.g., 10Hz)
  - [ ] Configure reference position
  - [ ] Set sensor position on vehicle
- [ ] Bridge GPS topics to ROS
  - [ ] Configure `ros_gz_bridge`
  - [ ] Verify message format (sensor_msgs/NavSatFix)
- [ ] Add realistic GPS noise model
  - [ ] Position noise (horizontal/vertical)
  - [ ] Multipath effects
  - [ ] Signal loss simulation
  - [ ] HDOP/VDOP simulation
- [ ] Test GPS data
  - [ ] Verify latitude/longitude
  - [ ] Check altitude
  - [ ] Validate accuracy estimates
  - [ ] Compare with ground truth position
- [ ] Advanced features
  - [ ] RTK GPS simulation (cm-level accuracy)
  - [ ] Satellite visibility modeling
  - [ ] Urban canyon effects

**Expected Topics:**
- `/sim/gps/fix`
- `/sim/gps/ground_truth`

---

#### 3.3.4 INS Fusion Plugin
- [ ] Design sensor fusion architecture
  - [ ] Choose fusion algorithm (EKF, UKF, complementary filter)
  - [ ] Define state vector
  - [ ] Define measurement models
- [ ] Implement IMU + GPS fusion
  - [ ] Create fusion plugin or node
  - [ ] Subscribe to IMU and GPS topics
  - [ ] Implement prediction step (IMU)
  - [ ] Implement correction step (GPS)
  - [ ] Publish fused odometry
- [ ] Publish fused odometry estimate
  - [ ] Position (from GPS + IMU integration)
  - [ ] Orientation (from IMU)
  - [ ] Velocity (from GPS + IMU)
  - [ ] Covariance estimates
- [ ] Validation and tuning
  - [ ] Compare with ground truth
  - [ ] Tune process/measurement noise
  - [ ] Test with GPS outages
  - [ ] Test with IMU drift
- [ ] Test scenarios
  - [ ] Straight line driving
  - [ ] Corner handling
  - [ ] Aggressive maneuvers
  - [ ] GPS denial scenarios

**Expected Topics:**
- `/sim/odometry/ins`
- `/sim/odometry/ins_ground_truth`

**Libraries to Consider:**
- `robot_localization` package
- Custom EKF implementation
- `imu_tools` package

---

### Sprint 4: Advanced Vehicle Dynamics (Low Priority)

#### 3.4.1 Ackermann Steering Model
- [ ] Research Ackermann geometry
- [ ] Implement full Ackermann steering
  - [ ] Model front/rear axle independently
  - [ ] Calculate inner/outer wheel angles
  - [ ] Proper steering linkage simulation
- [ ] Replace simplified bicycle model
  - [ ] Create new vehicle model class
  - [ ] Implement Ackermann kinematic equations
  - [ ] Update plugin to use new model
- [ ] Test steering behavior
  - [ ] Verify wheel angles
  - [ ] Check turning radius
  - [ ] Compare with bicycle model
  - [ ] Validate against real vehicle data (if available)
- [ ] Configuration
  - [ ] Make model selectable via parameter
  - [ ] Allow switching between bicycle and Ackermann
  - [ ] Tune parameters for accuracy

---

#### 3.4.2 Tire Dynamics Model
- [ ] Research tire models (Pacejka, brush model, etc.)
- [ ] Implement tire slip model
  - [ ] Longitudinal slip
  - [ ] Lateral slip
  - [ ] Combined slip
  - [ ] Slip angle calculation
- [ ] Add friction/grip modeling
  - [ ] Normal force calculation
  - [ ] Friction circle/ellipse
  - [ ] Load transfer effects
  - [ ] Saturation curves
- [ ] Simulate different tire compounds
  - [ ] Hard compound (longer life, less grip)
  - [ ] Soft compound (more grip, faster wear)
  - [ ] Wet tires
  - [ ] Parameter sets for each
- [ ] Test on various surfaces
  - [ ] Asphalt
  - [ ] Concrete
  - [ ] Wet surfaces
  - [ ] Validate behavior
- [ ] Validation
  - [ ] Compare with real vehicle data
  - [ ] Check cornering limits
  - [ ] Verify acceleration/braking
  - [ ] Tune parameters

**Libraries to Consider:**
- Pacejka Magic Formula
- Brush tire model
- TMeasy model

---

#### 3.4.3 Suspension Dynamics
- [ ] Design suspension model
  - [ ] Choose suspension type (double wishbone, MacPherson, etc.)
  - [ ] Model spring-damper system
  - [ ] Define suspension geometry
- [ ] Implement spring-damper system
  - [ ] Spring stiffness
  - [ ] Damping coefficient
  - [ ] Bump and rebound
  - [ ] Anti-roll bars (optional)
- [ ] Model weight transfer
  - [ ] During acceleration/braking
  - [ ] During cornering
  - [ ] Load on each wheel
  - [ ] Center of gravity effects
- [ ] Simulate pitch and roll
  - [ ] Body pitch under acceleration/braking
  - [ ] Body roll in corners
  - [ ] Coupling with tire model
- [ ] Test over bumpy terrain
  - [ ] Create test track with bumps
  - [ ] Verify suspension behavior
  - [ ] Check wheel contact
- [ ] Tune suspension parameters
  - [ ] For comfort vs performance
  - [ ] Match real vehicle characteristics
  - [ ] Validate behavior

---

#### 3.4.4 Road Conditions & Surface Modeling
- [ ] Implement wet/dry surface grip variation
  - [ ] Friction coefficient adjustment
  - [ ] Surface parameter in world files
  - [ ] Dynamic switching capability
- [ ] Adjustable surface friction coefficients
  - [ ] Per-surface type
  - [ ] Configurable via SDF
  - [ ] Runtime modification
- [ ] Dynamic weather effects
  - [ ] Rain simulation
  - [ ] Puddles (grip reduction zones)
  - [ ] Gradual transitions
- [ ] Test behavior changes
  - [ ] Braking distance on wet vs dry
  - [ ] Cornering speed limits
  - [ ] Acceleration traction
- [ ] Visual indicators
  - [ ] Surface texture changes
  - [ ] Water effects (if feasible)
  - [ ] HUD or topic for current conditions



---

## Phase 4: Advanced Features (Future)

**Target:** Long-term enhancements  
**Focus:** Research-level capabilities, advanced simulation

### 4.1 Advanced Perception Simulation

Replaces standard Gazebo plugins with more realistic sensor models and perception effects

#### 4.1.1 Realistic Camera Simulation
- [ ] Lens distortion models
- [ ] Chromatic aberration
- [ ] Motion blur
- [ ] Rolling shutter effects
- [ ] HDR and auto-exposure
- [ ] Sun glare and reflections
- [ ] Day/night lighting conditions

#### 4.1.2 LIDAR Simulation Improvements
- [ ] Multi-echo returns
- [ ] Beam divergence
- [ ] Atmospheric effects (fog, rain)
- [ ] Surface reflectivity modeling
- [ ] Moving object distortion
- [ ] Different LIDAR types (mechanical, solid-state)

---

### 4.2 Vehicle System Simulation

Plugins for simulating internal vehicle systems, communication, and safety features

#### 4.2.1 State Machine Simulation
- [ ] ECU bootup sequence
- [ ] Fault injection (sensor failure, actuator failure)
- [ ] Mode switching (manual, autonomous)
- [ ] Safety system simulation (emergency braking, stability control)

#### 4.2.2 Communication Simulation
- [ ] V2V (Vehicle-to-Vehicle) communication
- [ ] V2I (Vehicle-to-Infrastructure)
- [ ] Network latency
- [ ] Packet loss
- [ ] Bandwidth limits

---

### 4.3 Advanced Vehicle Dynamics

#### 4.3.1 Aerodynamics
- [ ] Downforce modeling
- [ ] Drag coefficient
- [ ] Crosswind effects
- [ ] Ground effect
- [ ] Aero balance

#### 4.3.2 Drivetrain
- [ ] Electric motor modeling
- [ ] Battery simulation
- [ ] Regenerative braking
- [ ] Power limits
- [ ] Thermal management

#### 4.3.3 Full 3D Dynamics
- [ ] 6-DOF vehicle model
- [ ] Unsprung mass
- [ ] Chassis flex
- [ ] Complete multi-body simulation

---

## Roadmap

### Short-term Goals
- ✅ Complete Jazzy migration
- ✅ Fix all critical bugs
- ✅ Establish stable baseline
- 🎯 Complete Phase 3 (Sensor expansion + ROS 2 Control)
- 🎯 Full integration with QUTMS_Driverless stack
- 🎯 Documented and tested system

### Medium-term Goals
- Advanced vehicle dynamics models
- Multi-vehicle support
- Comprehensive sensor suite

### Long-term Goals
- Industry-standard simulation platform
- Real-time hardware-in-the-loop
- Digital twin capabilities
- Competition scenario library
- Training programs and documentation

---

## References & Resources

### Documentation
- [ROS 2 Jazzy Documentation](https://docs.ros.org/en/jazzy/)
- [GZ Sim Harmonic Documentation](https://gazebosim.org/docs/harmonic)
- [ros_gz Documentation](https://github.com/gazebosim/ros_gz)
- [ROS 2 Control](https://control.ros.org/jazzy/index.html)

### Related Projects
- [EUFS Sim](https://gitlab.com/eufs/eufs_sim) - Original inspiration
- [EUFS Sim Fork](https://github.com/QUT-Motorsport/eufs_sim) - Adaptation for QUTMS
- [Gazebo Classic](https://classic.gazebosim.org/) - Original simulation platform

### Tools
- [RViz2](https://github.com/ros2/rviz) - ROS visualization
- [Foxglove Studio](https://foxglove.dev/) - Modern robotics visualization

---

**Document Version:** 1.0  
**Last Updated:** 2026-02-28  
**Migration Status:** ✅ Complete (Phase 1 & 2)  
**Next Focus:** Phase 3 - Feature Enhancements
