# QUTMS_AV_Sim

> **📍 ROS 2 Jazzy (Ubuntu 24.04)**  
> This simulator uses ROS 2 Jazzy Jalisco with GZ Sim (Harmonic).  
>
> **Documentation:**
> - [Quick Start Guide](./QUICKSTART_JAZZY.md) - Setup and installation instructions
> - [Migration Guide](./MIGRATION_GUIDE.md) - If upgrading from Gazebo Classic
> - [Migration Status](./JAZZY_MIGRATION_STATUS.md) - Current development status

## Overview

QUTMS_AV_Sim is designed to facilitate development of autonomous systems with little-to-no prior ROS 2 experience. Primarily, it is intended to be used by the Queensland University of Technology Motorsport (QUTMS) team to develop their autonomous vehicle software in ROS 2. However, it is open source and can be used by anyone, as is aligned with QUTMS's vision. QUTMS_AV_Sim makes use of the [Gazebo](http://gazebosim.org/) simulator for lightweight ROS 2 specific vehicle URDFs.

QUTMS has and continues to use in varying capacities, forked versions the Formula Student Driverless Simulator (FSDS) and the Edinburgh University Formula Student Simulator (EUFS Sim). Some Gazebo and ROS 2 source code has also been forked from eufs_sim for this project.

## Installation

### Prerequisites

- Ubuntu 24.04
- ROS 2 Jazzy Jalisco - [Installation Guide](https://docs.ros.org/en/jazzy/Installation.html)
- GZ Sim (Harmonic) - Installed with `gz-harmonic` package
- ros_gz packages - `ros-jazzy-ros-gz`, `ros-jazzy-ros-gz-sim`, `ros-jazzy-ros-gz-bridge`

For detailed setup instructions, see the [Quick Start Guide](./QUICKSTART_JAZZY.md).

```
cd <YOUR ROS 2 WORKSPACE<> # eg. QUTMS/
git clone https://github.com/QUT-Motorsport/QUTMS_AV_Sim.git
```

If not already installed, install the QUTMS_Driverless repo for our ROS 2 msgs.
```
git clone https://github.com/QUT-Motorsport/QUTMS_Driverless.git
```

### Dependencies

Source existing ROS 2 workspace

```
source install/setup.bash 
# alternatively, if workspace was installed with scripts
a
```

Install ROS 2 dependencies with `rosdep`

```
sudo apt-get update && apt-get upgrade -y
rosdep update
rosdep install -y \
    --rosdistro=${ROS_DISTRO} \
    --ignore-src \
    --from-paths QUTMS_AV_Sim
```

> If this fails due to missing dependency `driverless_msgs`, try building the `driverless_msgs` package first and re-sourcing the workspace.

### Building

Use `colcon build` or build shortcut scripts
```
colcon build --symlink-install --packages-up-to qutms_sim
# alternatively, if workspace was installed with scripts
./build.sh -u qutms_sim
```

## Usage

### Launching the Simulator

```
source install/setup.bash 
# alternatively, if workspace was installed with scripts
a
ros2 launch qutms_sim sim.launch.py
```

### Configuring the Simulator

The simulator can be configured using the config file. This file is located at `QUTMS_AV_Sim/qutms_sim/config/config.yaml`. The file contains a number of parameters, whose effects are documented within.

#### Plugin Configuration

GZ Sim plugins (vehicle dynamics, cone detection, etc.) are configured directly in the URDF files using SDF parameters. Plugin parameters are embedded as XML elements within the `<plugin>` tags.

To modify plugin behavior, edit the corresponding URDF/xacro files:
- **Vehicle Plugin**: `qutms_sim/urdf/robot.urdf.xacro` - Configure vehicle dynamics, update rates, frame IDs, and control delays
- **Cone Detection Plugin**: `qutms_sim/urdf/robot.urdf.xacro` - Configure LIDAR parameters, detection ranges, and noise settings
- **Sensor Plugins**: `qutms_sim/urdf/lidar.urdf.xacro` - Configure sensor-specific parameters

Example plugin configuration in URDF:
```xml
<plugin filename="libgazebo_vehicle_plugin" name="vehicle_plugin">
  <vehicle_params>$(find qutms_sim)/config/vehicle.yaml</vehicle_params>
  <update_rate>1000.0</update_rate>
  <publish_rate>200.0</publish_rate>
  <map_frame>track</map_frame>
  <odom_frame>track</odom_frame>
  <base_frame>base_footprint</base_frame>
</plugin>
```

#### Bridge Configuration

Topic bridging between GZ Sim and ROS 2 is configured in `qutms_sim/config/bridge.yaml`. This allows sensor data and other topics to pass between the simulator and ROS 2.

To add new topics to the bridge:
1. Open `qutms_sim/config/bridge.yaml`
2. Add a new topic entry to the `topics` list following this format:

```yaml
- topic_name: your_ros_topic_name
  gz_topic_name: /world/WORLD_NAME/model/QEV-3D/link/<link_name>/sensor/<sensor_name>/<topic>
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

### Visualising the Simulator

The simulator can be visualised using RViz or Foxglove Studio, these can be configured. With Rviz, you can visualise the vehicle and its sensors. With Foxglove Studio, you can visualise the vehicle, its sensors, and datastreams, in addition to controlling the vehicle and providing a more interactive experience.

With Foxglove Studio, you can download the simulator's `.json` dashboard from github `https://github.com/QUT-Motorsport/QUTMS_AV_Sim/blob/main/qutms_sim/visuals/QUTMS_AV_Sim%20control.json` to load some default visuals.
See the member setup guide for more information on how to use Foxglove Studio and custom dashboards.
