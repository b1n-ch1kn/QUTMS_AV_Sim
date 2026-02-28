# Migration Guide: Gazebo Classic to GZ Sim (ROS 2 Humble → Jazzy)

## Overview
This document describes the migration from Ubuntu 22.04/ROS 2 Humble/Gazebo Classic to Ubuntu 24.04/ROS 2 Jazzy/GZ Sim (Harmonic).

## Key Changes

### 1. Dependencies
**Before (Gazebo Classic):**
- `gazebo`, `gazebo_dev`, `gazebo_ros`, `gazebo_plugins`

**After (GZ Sim):**
- `gz-sim8` (Gazebo Harmonic)
- `gz-plugin2`, `gz-transport13`, `gz-sensors8`, `gz-physics7`
- `ros_gz`, `ros_gz_sim`, `ros_gz_bridge`

### 2. Plugin Architecture

#### Gazebo Classic (Humble)
```cpp
class MyPlugin : public gazebo::ModelPlugin {
    void Load(gazebo::physics::ModelPtr model, sdf::ElementPtr sdf) override;
    gazebo::event::ConnectionPtr update_connection;
    gazebo::physics::WorldPtr world;
    gazebo::physics::ModelPtr model;
};
```

#### GZ Sim (Jazzy)
```cpp
class MyPlugin : public gz::sim::System,
                 public gz::sim::ISystemConfigure,
                 public gz::sim::ISystemPreUpdate {
    void Configure(const gz::sim::Entity &entity,
                   const std::shared_ptr<const sdf::Element> &sdf,
                   gz::sim::EntityComponentManager &ecm,
                   gz::sim::EventManager &eventMgr) override;
    
    void PreUpdate(const gz::sim::UpdateInfo &info,
                   gz::sim::EntityComponentManager &ecm) override;
    
    gz::sim::Entity _entity;
    gz::sim::Model _model;
};
```

### 3. API Changes

| Gazebo Classic | GZ Sim |
|----------------|--------|
| `gazebo::physics::ModelPtr` | `gz::sim::Model` (wraps `gz::sim::Entity`) |
| `gazebo::physics::WorldPtr` | `gz::sim::World` |
| `gazebo::physics::JointPtr` | `gz::sim::Joint` |
| `model->WorldPose()` | Use ECM components |
| `ignition::math` namespace | `gz::math` namespace |
| `gazebo_ros::Node::Get(sdf)` | `rclcpp::Node` (create directly) |
| Event-based updates | `PreUpdate()`/`PostUpdate()` methods |

### 4. ROS Node Creation

#### Before
```cpp
node = gazebo_ros::Node::Get(sdf);
```

#### After
```cpp
if (!rclcpp::ok()) {
    rclcpp::init(0, nullptr);
}
node = std::make_shared<rclcpp::Node>("plugin_node");
```

### 5. Plugin Registration

#### Before
```cpp
GZ_REGISTER_MODEL_PLUGIN(MyPlugin)
```

#### After  
```cpp
#include <gz/plugin/Register.hh>
GZ_ADD_PLUGIN(MyPlugin,
              gz::sim::System,
              MyPlugin::ISystemConfigure,
              MyPlugin::ISystemPreUpdate)
```

### 6. Launch File Changes

#### Before (Gazebo Classic)
```python
from launch_ros.actions import Node
from gazebo_ros.gazebo_launcher import GazeboLauncher

gazebo = IncludeLaunchDescription(
    PythonLaunchDescriptionSource(
        os.path.join(get_package_share_directory('gazebo_ros'), 'launch', 'gazebo.launch.py')
    ),
    launch_arguments={'world': world_path}.items()
)

spawn = Node(
    package='gazebo_ros',
    executable='spawn_entity.py',
    arguments=['-entity', 'robot', '-file', urdf_path]
)
```

#### After (GZ Sim)
```python
from launch_ros.actions import Node

gz_sim = IncludeLaunchDescription(
    PythonLaunchDescriptionSource(
        os.path.join(get_package_share_directory('ros_gz_sim'), 'launch', 'gz_sim.launch.py')
    ),
    launch_arguments={'gz_args': f'{world_path}'}.items()
)

spawn = Node(
    package='ros_gz_sim',
    executable='create',
    arguments=['-name', 'robot', '-file', urdf_path, '-x', '0', '-y', '0', '-z', '0.5']
)
```

### 7. URDF/SDF Plugin Tags

#### Before
```xml
<gazebo>
    <plugin name="my_plugin" filename="libmy_plugin.so">
        <ros>
            <namespace>/robot</namespace>
        </ros>
    </plugin>
</gazebo>
```

#### After
```xml
<gazebo>
    <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics"/>
    <plugin filename="libmy_plugin.so" name="my_namespace::MyPlugin">
        <ros>
            <namespace>/robot</namespace>
        </ros>
    </plugin>
</gazebo>
```

### 8. Environment Variables

#### Before
- `GAZEBO_MODEL_PATH`
- `GAZEBO_PLUGIN_PATH`
- `GAZEBO_RESOURCE_PATH`

#### After
- `GZ_SIM_RESOURCE_PATH`
- `GZ_SIM_SYSTEM_PLUGIN_PATH`

## Migration Steps

1. ✅ Update `package.xml` dependencies
2. ✅ Update `CMakeLists.txt` with GZ libraries
3. 🔄 Refactor plugin headers (System interface)
4. 🔄 Refactor plugin source code (Configure/PreUpdate)
5. ⏳ Update URDF plugin references
6. ⏳ Update launch files
7. ⏳ Update world files to SDF 1.8+
8. ⏳ Test and validate

## Resources
- [GZ Sim Documentation](https://gazebosim.org/docs/harmonic)
- [ROS 2 Jazzy](https://docs.ros.org/en/jazzy/)
- [ros_gz packages](https://github.com/gazebosim/ros_gz)
