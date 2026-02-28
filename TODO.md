# TODO List for QUTMS_AV_Sim

ROS 2 simulation for Formula Student vehicles.

## ROS 2 Upgrades

Convert to ROS 2 Jazzy
Implement ROS 2 control and controllers with GZ sim
Update vehicle plugin to modern GZ sim model connection and GZ bridge methods


## Plugin System Feature

Configure plugins to be modular
Plugins should be able to be added/removed without affecting the core functionality of the simulator.

### Plugins that should be available via base Gazebo or custom plugins:

- LIDAR pointcloud sensor
- LIDAR laserscan sensor
- Camera sensor
- IMU sensor
- GPS sensor
- INS plugin (odometry from IMU+GPS fusion)
- Cone Detection camera plugin (by passes camera processing algorithm, directly outputs cone positions in camera frame)
- Cone Detection LIDAR plugin (by passes LIDAR processing algorithm, directly outputs cone positions in lidar frame)
- Teleop plugin (for manual control of the vehicle)
- Reset plugin (to reset the simulation to a known state)
- More complex dynamic models that can replace default bicycle model (e.g., ackerman steering, full 3D dynamics, slip, tire models)
- Road conditions (wet or dry)

## Cmake ament auto

Convert cmakelists to use ament auto for ROS 2.