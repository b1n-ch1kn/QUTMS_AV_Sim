# QUTMS_AV_Sim

> **⚠️ ROS 2 Jazzy Migration in Progress**  
> This repository is currently being migrated from ROS 2 Humble (Ubuntu 22.04, Gazebo Classic) to ROS 2 Jazzy (Ubuntu 24.04, GZ Sim/Harmonic).  
> 
> - ✅ **Vehicle plugin migrated** to GZ Sim System interface
> - ✅ **Launch files updated** for ros_gz_sim  
> - ✅ **URDF updated** for GZ Sim sensors
> - ⏳ **Cone detection plugin** migration in progress
> - ⏳ **Testing and validation** required
>
> **Documentation:**
> - [Jazzy Migration Status](./JAZZY_MIGRATION_STATUS.md) - Current progress and remaining work
> - [Quick Start Guide (Jazzy)](./QUICKSTART_JAZZY.md) - How to use with ROS 2 Jazzy
> - [Migration Guide](./MIGRATION_GUIDE.md) - Technical details of API changes
> 
> For the stable Humble version, see the `humble` branch (to be created).

## Overview

QUTMS_AV_Sim is designed to facilitate development of autonomous systems with little-to-no prior ROS 2 experience. Primarily, it is intended to be used by the Queensland University of Technology Motorsport (QUTMS) team to develop their autonomous vehicle software in ROS 2. However, it is open source and can be used by anyone.
It makes use of the [Gazebo](http://gazebosim.org/) simulator for lightweight ROS 2 specific vehicle URDFs.

QUTMS has and continues to use in varying capacities, forked versions the Formula Student Driverless Simulator (FSDS) and the Edinburgh University Formula Student Simulator (EUFS Sim). Some Gazebos and ROS 2 plugins have also been forked from eufs_sim for this project.

## Installation

### Prerequisites

**For ROS 2 Jazzy (Ubuntu 24.04):**
- Follow the [Quick Start Guide (Jazzy)](./QUICKSTART_JAZZY.md) for detailed Jazzy setup instructions
- ROS 2 Jazzy Jalisco - [Installation Guide](https://docs.ros.org/en/jazzy/Installation.html)
- GZ Sim (Harmonic) - Installed with `gz-harmonic` package

**For ROS 2 Humble (Ubuntu 22.04) - Legacy:**
- ROS 2 Humble Hawksbill - [Installation Guide](https://docs.ros.org/en/humble/Installation.html)
- Gazebo Classic 11 - Usually installed with `ros-humble-gazebo-ros-pkgs`
- See `humble` branch (to be created) for Humble-specific instructions

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

### Visualising the Simulator

The simulator can be visualised using RViz or Foxglove Studio, these can be configured. With Rviz, you can visualise the vehicle and its sensors. With Foxglove Studio, you can visualise the vehicle, its sensors, and datastreams, in addition to controlling the vehicle and providing a more interactive experience.

With Foxglove Studio, you can download the simulator's `.json` dashboard from github `https://github.com/QUT-Motorsport/QUTMS_AV_Sim/blob/main/qutms_sim/visuals/QUTMS_AV_Sim%20control.json` to load some default visuals.
See the member setup guide for more information on how to use Foxglove Studio and custom dashboards.
