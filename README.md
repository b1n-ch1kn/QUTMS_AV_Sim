# QUTMS_AV_Sim

> **📍 ROS 2 Jazzy (Ubuntu 24.04)**  
> This simulator uses ROS 2 Jazzy Jalisco with GZ Sim (Harmonic).  
> **Current Mode:** Physics-Based Control (ROS 2 Control)  
> **Status:** ✅ Fully operational - Ackermann steering controller working with realistic vehicle dynamics
>
> **Documentation:**
> - [TODO](./TODO.md) - Complete migration history and future roadmap

## Overview

QUTMS_AV_Sim is designed to facilitate development of autonomous systems with little-to-no prior ROS 2 experience. It was intended to be used by the Queensland University of Technology Motorsport (QUTMS) team to develop their autonomous vehicle software in ROS 2. However, aligning with the open source philosophy and QUTMS's vision, it can be used by anyone. QUTMS_AV_Sim makes use of the [Gazebo](http://gazebosim.org/) simulator for lightweight ROS 2 specific vehicle URDFs with custom vehicle model plugins.

The simulator supports **two control modes**:
- **Physics-Based Control** (Active): ROS 2 Control with Ackermann steering controller - realistic vehicle dynamics via wheel velocities
- **Kinematic Control** (Disabled): Custom plugins with direct pose/velocity commands - faster, simplified control

QUTMS has and continues to use in varying capacities, forked versions the Formula Student Driverless Simulator (FSDS) and the Edinburgh University Formula Student Simulator (EUFS Sim). Some Gazebo and ROS 2 source code has also been forked from eufs_sim for this project.

## System Requirements

- **OS**: Ubuntu 24.04 LTS (Noble Numbat)
- **ROS 2**: Jazzy Jalisco
- **Simulator**: GZ Sim (Harmonic)
- **Additional**: QUTMS_Driverless stack

## Installation

### 1. Install Dependencies

```bash
# ROS 2 Jazzy
sudo apt install ros-jazzy-desktop-full

# GZ Sim Harmonic  
sudo apt install gz-harmonic

# ROS-GZ Bridge
sudo apt install ros-jazzy-ros-gz \
                 ros-jazzy-ros-gz-sim \
                 ros-jazzy-ros-gz-bridge \
                 ros-jazzy-ros-gz-image

# Additional dependencies
sudo apt install ros-jazzy-xacro \
                 ros-jazzy-joint-state-publisher \
                 ros-jazzy-robot-state-publisher \
                 ros-jazzy-rviz2 \
                 ros-jazzy-foxglove-bridge

# ROS 2 Control dependencies (for physics-based control)
sudo apt install ros-jazzy-ros2-control \
                 ros-jazzy-ros2-controllers \
                 ros-jazzy-gz-ros2-control \
                 ros-jazzy-controller-manager \
                 ros-jazzy-joint-state-broadcaster \
                 ros-jazzy-ackermann-steering-controller \
                 ros-jazzy-effort-controllers
```

### 2. Clone Repositories

```bash
cd <YOUR ROS 2 WORKSPACE> # eg. ~/fsae or ~/QUTMS/
git clone https://github.com/QUT-Motorsport/QUTMS_AV_Sim.git
```

If not already installed, install the QUTMS_Driverless repo for our ROS 2 msgs:
```bash
git clone https://github.com/QUT-Motorsport/QUTMS_Driverless.git
```

### 3. Set Environment Variables

Add to your `~/.bashrc`:
```bash
# ROS 2 Jazzy
source /opt/ros/jazzy/setup.bash

# QUTMS Workspace
export QUTMS_WS=~/fsae  # or your workspace path
source $QUTMS_WS/install/setup.bash

# GZ Sim
export GZ_VERSION=harmonic
```

Apply changes:
```bash
source ~/.bashrc
```

### 4. Install ROS Dependencies

Source existing ROS 2 workspace:
```bash
source install/setup.bash 
# alternatively, if workspace was installed with scripts
a
```

Install ROS 2 dependencies with `rosdep`:
```bash
sudo apt-get update && apt-get upgrade -y
rosdep update
rosdep install -y \
    --rosdistro=${ROS_DISTRO} \
    --ignore-src \
    --from-paths QUTMS_AV_Sim
```

> **Note:** If this fails due to missing dependency `driverless_msgs`, try building the `driverless_msgs` package first and re-sourcing the workspace.

### 5. Build Workspace

```bash
cd ~/fsae  # or your workspace path

# Build simulator packages
colcon build --symlink-install --packages-up-to qutms_sim

# alternatively, if workspace was installed with scripts
./build.sh -u qutms_sim

# Source the workspace
source install/setup.bash
```

## Usage

### Launch Options

```bash
# Launch with default track
ros2 launch qutms_sim sim.launch.py

# Launch with specific track
ros2 launch qutms_sim sim.launch.py track:=small_track

# Launch with RViz
ros2 launch qutms_sim sim.launch.py track:=small_track rviz:=true

# Launch with Foxglove
ros2 launch qutms_sim sim.launch.py track:=small_track foxglove:=true
```

### Available Tracks

- `small_track`
- `small_track_2`
- `small_oval`
- `acceleration`
- `BM_long_straight`
- `BM_text_bubble`
- `B_shape_02_03_2023`
- `Jellybean_02_03_2023`
- `Hairpin_02_03_2023`
- `QR_Nov_2022`
- `FSDS_Training`

## Control Modes

The simulator supports two distinct control architectures. **Currently using: Physics-Based Control**

### Physics-Based Control (Active)

**Architecture:** ROS 2 Control + Ackermann Steering Controller  
**Method:** Wheel velocity commands → Physics engine computes vehicle motion  
**Status:** ✅ Active (default) - **Fully operational and tested**

**Features:**
- Realistic vehicle dynamics from tire-ground contact
- Natural weight transfer and suspension behavior
- Proper Ackermann steering geometry
- Odometry feedback from controller
- Supports velocity and effort (torque) interfaces

**Controllers:**
- `ackermann_steering_controller` - Bicycle model control
- `joint_state_broadcaster` - Joint state publishing
- Individual wheel effort controllers - Direct torque control

**Use Case:** Advanced control algorithm development, realistic simulation

### Kinematic Control (Disabled)

**Architecture:** Custom Gazebo Plugins  
**Method:** Direct pose/velocity commands via WorldPoseCmd  
**Status:** ⏸️ Commented out (to enable, edit `qutms_sim/urdf/gz_plugins.urdf.xacro`)

**Features:**
- Fast, deterministic control
- No physics engine overhead
- Instant response to commands
- Simpler for basic testing

**Plugins:**
- `VehiclePlugin` - Bicycle model kinematics
- `VehicleControlPlugin` - Command input handling

**Use Case:** Fast testing, simple scenarios, sensor algorithm development

**To Switch Modes:**
Edit [qutms_sim/urdf/gz_plugins.urdf.xacro](qutms_sim/urdf/gz_plugins.urdf.xacro):
- Enable kinematic: Uncomment Vehicle & VehicleControl plugins
- Enable physics: Uncomment those plugins (they conflict with ROS 2 Control)

## ROS 2 Topics

### Subscribed Topics (Control)

**Custom Vehicle Control (via plugins):**
- `/control/ackermann_cmd` - Ackermann drive commands (high-level)
- `/control/twist_cmd` - Twist (velocity) commands (high-level)

**ROS 2 Control (physics-based):**
- `/ackermann_steering_controller/reference` - Bicycle model control via TwistStamped
- `/front_left_wheel_controller/commands` - Direct torque control (Nm)
- `/front_right_wheel_controller/commands` - Direct torque control (Nm)
- `/rear_left_wheel_controller/commands` - Direct torque control (Nm)
- `/rear_right_wheel_controller/commands` - Direct torque control (Nm)

### Published Topics (Sensors & State)
- `/odometry` - Noisy odometry (simulated INS sensor)
- `/odometry/ground_truth` - Perfect odometry (for debugging)
- `/joint_states` - All joint states (steering + wheels)
- `/scan` - LIDAR scan data
- `/track/ground_truth` - Ground truth track (all cones)
- `/detections/ground_truth` - Simulated cone detections
- `/ackermann_steering_controller/odometry` - Controller odometry feedback
- `/ackermann_steering_controller/controller_state` - Controller status
- `/ackermann_steering_controller/tf_odometry` - TF-based odometry

### Services
- `/reset_simulation` - Reset vehicle to starting position (⚠️ cone reset currently not working, see [Known Issues](#known-issues))

## Sending Commands

### Using Custom Vehicle Control Plugin

The custom vehicle control plugin provides high-level Ackermann and Twist interfaces:

```bash
# Publish Ackermann command
ros2 topic pub /control/ackermann_cmd ackermann_msgs/msg/AckermannDriveStamped "
drive:
  speed: 5.0
  steering_angle: 0.2
  acceleration: 1.0
"

# Publish Twist command
ros2 topic pub /control/twist_cmd geometry_msgs/msg/Twist "
linear:
  x: 2.0
angular:
  z: 0.1
"
```

### Using ROS 2 Control (Physics-Based)

ROS 2 Control provides physics-based control using the Ackermann steering controller:

```bash
# Send TwistStamped to Ackermann steering controller (bicycle model)
# Command topic is /ackermann_steering_controller/reference
ros2 topic pub /ackermann_steering_controller/reference geometry_msgs/msg/TwistStamped "
header:
  stamp:
    sec: 0
    nanosec: 0
  frame_id: ''
twist:
  linear:
    x: 5.0
    y: 0.0
    z: 0.0
  angular:
    x: 0.0
    y: 0.0
    z: 0.3
"

# Or use a simpler one-liner (auto-fills header)
ros2 topic pub /ackermann_steering_controller/reference geometry_msgs/msg/TwistStamped "{twist: {linear: {x: 5.0}, angular: {z: 0.3}}}"

# Or control individual wheels directly with torques (effort in Nm)
ros2 topic pub /front_left_wheel_controller/commands std_msgs/msg/Float64MultiArray "
data: [50.0]
"
```

**Note:** The Ackermann steering controller automatically:
- Converts TwistStamped reference to steering angles and wheel velocities
- Handles Ackermann geometry (wheelbase, track width)
- Applies velocity/acceleration limits
- Computes proper left/right steering angles
- Publishes odometry feedback on `/ackermann_steering_controller/odometry`

**Key Configuration Details:**
- Front wheels have **state-only interfaces** (they rotate passively from ground friction)
- Rear wheels have **velocity command interfaces** (actively driven)
- Steering hinges have **position command interfaces** (actively steered)
- Update rate: 100 Hz for responsive control

### List Active Controllers

```bash
# List all controllers
ros2 control list_controllers

# Expected output:
# joint_state_broadcaster[joint_state_broadcaster/JointStateBroadcaster] active
# ackermann_steering_controller[ackermann_steering_controller/AckermannSteeringController] active
# front_left_wheel_controller[effort_controllers/JointGroupEffortController] active
# front_right_wheel_controller[effort_controllers/JointGroupEffortController] active
# rear_left_wheel_controller[effort_controllers/JointGroupEffortController] active
# rear_right_wheel_controller[effort_controllers/JointGroupEffortController] active
```

### Reset Simulation

```bash
ros2 service call /reset_simulation std_srvs/srv/Trigger
```

## Configuration

### Simulation Config

Edit `qutms_sim/config/config.yaml` for general simulator settings:
- Default track selection
- Namespace settings
- Visualization options
- Update rates

### Vehicle Parameters

Edit `qutms_sim/config/vehicle_params.yaml` to configure:
- Wheelbase, track width
- Steering limits
- Velocity/acceleration limits
- Inertia parameters

### Sensor Noise

Edit `qutms_sim/config/motion_noise.yaml` to configure:
- Odometry noise parameters
- Sensor standard deviations

### ROS 2 Controllers

Edit `qutms_sim/config/ros2_controllers.yaml` to configure physics-based controllers:

**Important:** The configuration has been optimized for realistic physics simulation:
- `enable_odom_tf: false` - Odometry TF to be handled by autonomous system
- `open_loop: false` - Uses velocity feedback for accurate control
- `position_feedback: false` - Relies on commanded positions (standard for Ackermann)
- `velocity_rolling_window_size: 10` - Smooths velocity measurements
- **Critical:** Front wheel joints must NOT have command interfaces (passive rotation only)

**Ackermann Steering Controller:**
- Vehicle geometry (wheelbase, track width, wheel radius)
- Steering limits and interface type (position/velocity)
- Wheel control interface type (effort/velocity)
- Velocity and acceleration limits

**Individual Wheel Effort Controllers:**
- Direct torque control for each wheel
- Useful for traction control, differential braking, etc.

The configuration includes:
- `joint_state_broadcaster` - Publishes joint states to `/joint_states`
- `ackermann_steering_controller` - Bicycle model control
- Individual wheel effort controllers - Direct torque commands

Controllers are automatically spawned at launch and can be managed using `ros2 control` CLI tools.

### Plugin Configuration

GZ Sim plugins (vehicle dynamics, cone detection, etc.) are configured directly in the URDF files using SDF parameters. Plugin parameters are embedded as XML elements within the `<plugin>` tags.

To modify plugin behavior, edit the corresponding URDF/xacro files:
- **Custom Plugins**: `qutms_sim/urdf/gz_plugins.urdf.xacro` - All 6 custom Gazebo plugins (vehicle dynamics, control, sensors, reset)
- **ROS 2 Control**: `qutms_sim/urdf/ros2_control.urdf.xacro` - Joint state interfaces and gz_ros2_control configuration
- **Vehicle Structure**: `qutms_sim/urdf/robot.urdf.xacro` - Chassis, inertia, and component assembly
- **Components**: `qutms_sim/urdf/components/` - Wheels, LIDAR, and other vehicle components

Example plugin configuration in URDF:
```xml
<!-- Vehicle dynamics plugin -->
<plugin filename="libgazebo_vehicle.so" name="gazebo_plugins::vehicle_plugins::VehiclePlugin">
  <ros>
    <namespace>/</namespace>  <!-- No namespace by default -->
  </ros>
  <vehicle_params>$(find qutms_sim)/config/vehicle_params.yaml</vehicle_params>
  <update_rate>50.0</update_rate>
  <steering_lock_time>1.5</steering_lock_time>
</plugin>
```

### Bridge Configuration

Topic bridging between GZ Sim and ROS 2 is configured in `qutms_sim/config/bridge.yaml`. This allows sensor data and other topics to pass between the simulator and ROS 2.

To add new topics to the bridge:
1. Open `qutms_sim/config/bridge.yaml`
2. Add a new topic entry to the `topics` list following this format:

```yaml
- ros_topic_name: your_ros_topic_name
  gz_topic_name: your_gz_topic_name
  ros_type_name: package_name/msg/MessageType
  gz_type_name: gz.msgs.MessageType
  direction: GZ_TO_ROS  # or ROS_TO_GZ or BIDIRECTIONAL
```

**Note:** Use `WORLD_NAME` as a placeholder - it will be automatically replaced with the actual track name at launch time.

Common sensor bridges:
- **LIDAR**: `sensor_msgs/msg/LaserScan` ↔ `gz.msgs.LaserScan`
- **Camera**: `sensor_msgs/msg/Image` ↔ `gz.msgs.Image`
- **IMU**: `sensor_msgs/msg/Imu` ↔ `gz.msgs.IMU`
- **Odometry**: `nav_msgs/msg/Odometry` ↔ `gz.msgs.Odometry`

See the commented examples in `bridge.yaml` for more details.

## Visualization

### RViz2

```bash
# Launch with RViz (or add rviz:=true to launch command)
ros2 launch qutms_sim sim.launch.py rviz:=true

# Or launch RViz manually
rviz2 -d ~/fsae/install/qutms_sim/share/qutms_sim/visuals/default.rviz
```

With RViz, you can visualize the vehicle and its sensors including LIDAR scans, odometry, and TF frames.

### Foxglove Studio

```bash
# Launch with Foxglove bridge
ros2 launch qutms_sim sim.launch.py foxglove:=true

# Then connect Foxglove Studio to ws://localhost:8765
```

With Foxglove Studio, you can visualize the vehicle, its sensors, and datastreams, in addition to controlling the vehicle and providing a more interactive experience.

You can download the simulator's `.json` dashboard from GitHub: `https://github.com/QUT-Motorsport/QUTMS_AV_Sim/blob/main/qutms_sim/visuals/QUTMS_AV_Sim%20control.json` to load some default visuals.

See the member setup guide for more information on how to use Foxglove Studio and custom dashboards.

## Quick Reference

### Physics-Based Control (Active)

**Test vehicle control:**
```bash
# Drive forward with slight left turn
ros2 topic pub /ackermann_steering_controller/reference geometry_msgs/msg/TwistStamped "{twist: {linear: {x: 5.0}, angular: {z: 0.3}}}"

# Stop
ros2 topic pub /ackermann_steering_controller/reference geometry_msgs/msg/TwistStamped "{twist: {linear: {x: 0.0}, angular: {z: 0.0}}}"
```

**Monitor controller:**
```bash
# List active controllers
ros2 control list_controllers

# View controller odometry
ros2 topic echo /ackermann_steering_controller/odometry

# View controller state
ros2 topic echo /ackermann_steering_controller/controller_state
```

**Direct wheel torque control:**
```bash
# Apply 50 Nm to rear left wheel
ros2 topic pub /rear_left_wheel_controller/commands std_msgs/msg/Float64MultiArray "{data: [50.0]}"
```

### Kinematic Control (Disabled)

To switch to kinematic control mode:
1. Edit [qutms_sim/urdf/gz_plugins.urdf.xacro](qutms_sim/urdf/gz_plugins.urdf.xacro)
2. Uncomment the `VehiclePlugin` and `VehicleControlPlugin` sections
3. Rebuild: `colcon build --symlink-install --packages-select qutms_sim`
4. Launch normally

**Commands (when kinematic mode is active):**
```bash
# Ackermann command
ros2 topic pub /control/ackermann_cmd ackermann_msgs/msg/AckermannDriveStamped "{drive: {speed: 5.0, steering_angle: 0.2}}"

# Twist command
ros2 topic pub /control/twist_cmd geometry_msgs/msg/Twist "{linear: {x: 5.0}, angular: {z: 0.3}}"
```

### Key Topics Summary

| Topic | Type | Mode | Description |
|-------|------|------|-------------|
| `/ackermann_steering_controller/reference` | TwistStamped | Physics | Command input |
| `/ackermann_steering_controller/odometry` | Odometry | Physics | Controller feedback |
| `/control/ackermann_cmd` | AckermannDriveStamped | Kinematic | Command input |
| `/control/twist_cmd` | Twist | Kinematic | Command input |
| `/odometry` | Odometry | Both | INS sensor (noisy) |
| `/odometry/ground_truth` | Odometry | Both | Perfect odometry |
| `/joint_states` | JointState | Both | All joint states |
| `/scan` | LaserScan | Both | LIDAR data |
| `/track/ground_truth` | ConeArray | Both | All track cones |
| `/detections/ground_truth` | ConeArray | Both | Detected cones |

### Configuration Files

| File | Purpose |
|------|---------|
| `qutms_sim/config/ros2_controllers.yaml` | ROS 2 Control parameters |
| `qutms_sim/config/vehicle_params.yaml` | Vehicle geometry, limits |
| `qutms_sim/config/motion_noise.yaml` | Sensor noise parameters |
| `qutms_sim/config/config.yaml` | General simulation settings |
| `qutms_sim/urdf/ros2_control.urdf.xacro` | Joint interface definitions |
| `qutms_sim/urdf/gz_plugins.urdf.xacro` | Plugin configuration |
| `qutms_sim/urdf/robot.urdf.xacro` | Main vehicle structure |

## Troubleshooting

### Plugin Not Loading

```bash
# Check environment variables
echo $GZ_SIM_SYSTEM_PLUGIN_PATH
echo $GZ_SIM_RESOURCE_PATH

# Should include paths to install/vehicle_plugins/lib
```

### Model Not Found

```bash
# Ensure models directory is in GZ resource path
export GZ_SIM_RESOURCE_PATH=$GZ_SIM_RESOURCE_PATH:~/fsae/install/qutms_sim/share/qutms_sim/models
```

### Build Errors

```bash
# Clean and rebuild
cd ~/fsae
rm -rf build/ install/ log/
colcon build --packages-select vehicle_plugins qutms_sim
```

### GZ Sim Won't Start

```bash
# Check GZ installation
gz sim --version

# Should show Harmonic version (8.x.x)
```

## Development Workflow

### Modify Vehicle Plugin

1. Edit code in `vehicle_plugins/gazebo_vehicle_plugin/`
2. Rebuild: `colcon build --packages-select vehicle_plugins`
3. Source: `source install/setup.bash`
4. Test: `ros2 launch qutms_sim sim.launch.py`

### Create New Track

1. Create SDF file in `qutms_sim/worlds/<name>.sdf`
2. Create CSV file in `qutms_sim/worlds/<name>.csv` with cone positions
3. Launch: `ros2 launch qutms_sim sim.launch.py track:=<name>`

### Add New Sensor

1. Create sensor xacro in `qutms_sim/urdf/components/`
2. Include in `qutms_sim/urdf/robot.urdf.xacro`
3. Use GZ Sim sensor systems
4. Bridge to ROS topics in `qutms_sim/config/bridge.yaml`

## Additional Resources

- [TODO](./TODO.md) - Complete migration history and roadmap
- [GZ Sim Tutorials](https://gazebosim.org/docs/harmonic/tutorials)
- [ROS 2 Jazzy Documentation](https://docs.ros.org/en/jazzy/)
- [ROS 2 Control Documentation](https://control.ros.org/jazzy/index.html)
- [Ackermann Steering Controller](https://control.ros.org/jazzy/doc/ros2_controllers/ackermann_steering_controller/doc/userdoc.html)

## Development Status

**ROS 2 Control Integration Complete:**
- ✅ Ackermann steering controller fully operational
- ✅ Front wheels now rotate correctly (passive rotation via ground friction)
- ✅ Resolved interface configuration: front wheels use state-only, rear wheels use velocity commands
- ✅ Fixed wheel joint axis configuration to match gz_ros2_control_demos standard
- ✅ Controller update rate increased to 100 Hz for responsive control
- ✅ All wheels and steering hinges properly integrated with physics engine

### Completed ✅
- ✅ ROS 2 Jazzy + GZ Sim Harmonic migration
- ✅ 6 modular Gazebo plugins (ECM-based)
- ✅ Custom ECM components (VehicleControlInput, VehicleState)
- ✅ ROS 2 Control integration with command interfaces
- ✅ Ackermann steering controller (bicycle model)
- ✅ Physics-based vehicle dynamics
- ✅ Dual control modes (physics/kinematic)
- ✅ Modular URDF architecture
- ✅ Joint state broadcasting
- ✅ TF tree publishing
- ✅ Simulation reset service
- ✅ Ground truth sensors (odometry, cone detection)
- ✅ Configurable noise models

### In Progress 🚧
- 🚧 Advanced tire friction models
- 🚧 Traction control algorithms
- 🚧 Performance optimization

### Future Plans 📋
- 📋 Additional sensor plugins (cameras, IMU)
- 📋 Multi-vehicle support
- 📋 Advanced weather/lighting conditions
- 📋 Trajectory visualization tools
- 📋 Automated testing framework

## Known Issues

### Cone Reset Not Working

**Issue:** The `/reset_simulation` service successfully resets the vehicle to its initial position, but cones do not return to their starting positions.

**Status:** Under investigation

**Current Behavior:**
- ✅ Vehicle resets correctly (pose and velocities)
- ✅ Service detects all 77 cones
- ❌ Cone positions do not update in simulation

**Technical Details:**
The cone reset attempts to use `WorldPoseCmd` components on nested model links, but GZ Sim Harmonic's physics system appears to ignore these commands for included/nested models. The issue may require:
- Using a different component type for nested models
- Direct physics engine API calls
- Restructuring how cones are included in the world

**Workaround:** Restart the simulation to reset cone positions:
```bash
# Stop current sim (Ctrl+C)
ros2 launch qutms_sim sim.launch.py
```

**Tracking:** See [TODO.md](./TODO.md#known-issues) for updates

---

## Support

For GZ Sim issues:
- [GZ Community](https://community.gazebosim.org/)
- [GZ Sim GitHub](https://github.com/gazebosim/gz-sim)

For ROS 2 issues:
- [ROS 2 Discourse](https://discourse.ros.org/)
- [ROS 2 GitHub](https://github.com/ros2)
