# QUTMS_AV_Sim

> **📍 ROS 2 Jazzy (Ubuntu 24.04)**  
> This simulator uses ROS 2 Jazzy Jalisco with GZ Sim (Harmonic).  
>
> **Documentation:**
> - [TODO](./TODO.md) - Complete migration history and future roadmap

## Overview

QUTMS_AV_Sim is designed to facilitate development of autonomous systems with little-to-no prior ROS 2 experience. It was intended to be used by the Queensland University of Technology Motorsport (QUTMS) team to develop their autonomous vehicle software in ROS 2. However, aligning with the open source philosophy and QUTMS's vision, it can be used by anyone. QUTMS_AV_Sim makes use of the [Gazebo](http://gazebosim.org/) simulator for lightweight ROS 2 specific vehicle URDFs with custom vehicle model plugins.

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

## ROS 2 Topics

### Subscribed Topics (Control)
- `/control/ackermann_cmd` - Ackermann drive commands
- `/control/twist_cmd` - Twist (velocity) commands

### Published Topics (Sensors & State)
- `/odometry` - Noisy odometry (simulated INS sensor)
- `/odometry/ground_truth` - Perfect odometry (for debugging)
- `/joint_states` - All joint states (steering + wheels)
- `/scan` - LIDAR scan data
- `/track/ground_truth` - Ground truth track (all cones)
- `/detections/ground_truth` - Simulated cone detections

### Services
- `/reset_simulation` - Reset vehicle and cones to starting positions

## Sending Commands

### Using Command Line

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

- [TODO](./COMPREHENSIVE_TODO.md) - Complete migration history and roadmap
- [GZ Sim Tutorials](https://gazebosim.org/docs/harmonic/tutorials)
- [ROS 2 Jazzy Documentation](https://docs.ros.org/en/jazzy/)

## Support

For GZ Sim issues:
- [GZ Community](https://community.gazebosim.org/)
- [GZ Sim GitHub](https://github.com/gazebosim/gz-sim)

For ROS 2 issues:
- [ROS 2 Discourse](https://discourse.ros.org/)
- [ROS 2 GitHub](https://github.com/ros2)
