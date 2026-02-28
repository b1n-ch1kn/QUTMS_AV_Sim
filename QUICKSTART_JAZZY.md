# QUTMS_AV_Sim - Quick Start Guide (ROS 2 Jazzy)

## System Requirements
- Ubuntu 24.04 LTS (Noble Numbat)
- ROS 2 Jazzy Jalisco
- GZ Sim (Harmonic)
- QUTMS_Driverless stack

## Initial Setup

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

### 2. Set Environment Variables
Add to your `~/.bashrc`:
```bash
# ROS 2 Jazzy
source /opt/ros/jazzy/setup.bash

# QUTMS Workspace
export QUTMS_WS=~/fsae
source $QUTMS_WS/install/setup.bash

# GZ Sim
export GZ_VERSION=harmonic
```

### 3. Build Workspace
```bash
cd ~/fsae

# Build simulator packages
colcon build --packages-select vehicle_plugins qutms_sim

# Source the workspace
source install/setup.bash
```

## Running the Simulator

### Basic Launch
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
- `/sim/control/ackermann_cmd` - Ackermann drive commands
- `/sim/control/twist_cmd` - Twist (velocity) commands

### Published Topics (Sensors & State)
- `/sim/odometry` - Noisy odometry (simulated sensor)
- `/sim/odometry/ground_truth` - Perfect odometry (for debugging)
- `/sim/joint_states/steering` - Steering joint angles
- `/sim/scan` - LIDAR scan data
- `/sim/cones` - Detected cones (from cone detection plugin)

### Services
- `/sim/reset_vehicle` - Reset vehicle to starting position

## Sending Commands

### Using Command Line
```bash
# Publish Ackermann command
ros2 topic pub /sim/control/ackermann_cmd ackermann_msgs/msg/AckermannDriveStamped "
drive:
  speed: 5.0
  steering_angle: 0.2
  acceleration: 1.0
"

# Publish Twist command
ros2 topic pub /sim/control/twist_cmd geometry_msgs/msg/Twist "
linear:
  x: 2.0
angular:
  z: 0.1
"
```

### Reset Vehicle
```bash
ros2 service call /sim/reset_vehicle std_srvs/srv/Trigger
```

## Visualization

### RViz2
```bash
# Launch with RViz (or add rviz:=true to launch command)
rviz2 -d ~/fsae/install/qutms_sim/share/qutms_sim/visuals/default.rviz
```

### Foxglove Studio
```bash
# Launch with foxglove:=true, then connect Foxglove Studio to ws://localhost:8765
```

## Configuration

### Vehicle Parameters
Edit: `qutms_sim/config/vehicle_params.yaml`
- Wheelbase, track width
- Steering limits
- Velocity/acceleration limits
- Inertia parameters

### Sensor Noise
Edit: `qutms_sim/config/motion_noise.yaml`
- Odometry noise parameters
- Sensor standard deviations

### Simulation Config
Edit: `qutms_sim/config/config.yaml`
- Default track selection
- Namespace settings
- Visualization options
- Update rates

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
1. Add sensor to URDF in `qutms_sim/urdf/`
2. Use GZ Sim sensor systems
3. Bridge to ROS topics using ros_gz_bridge

## Additional Resources

- [Migration Status](./JAZZY_MIGRATION_STATUS.md) - Current migration progress
- [Migration Guide](./MIGRATION_GUIDE.md) - Detailed API changes
- [TODO List](./TODO.md) - Planned features
- [GZ Sim Tutorials](https://gazebosim.org/docs/harmonic/tutorials)

## Support

For issues specific to QUTMS_AV_Sim:
- Check [JAZZY_MIGRATION_STATUS.md](./JAZZY_MIGRATION_STATUS.md) for known issues
- Review [MIGRATION_GUIDE.md](./MIGRATION_GUIDE.md) for API changes

For GZ Sim issues:
- [GZ Community](https://community.gazebosim.org/)
- [GZ Sim GitHub](https://github.com/gazebosim/gz-sim)
