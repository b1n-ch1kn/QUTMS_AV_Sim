#!/usr/bin/env python3
"""
Minimal test launch - based on working gz_ros2_control_demos
Tests simplest possible Ackermann setup with our dimensions
"""

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, RegisterEventHandler
from launch.event_handlers import OnProcessExit
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command, FindExecutable, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    
    # Get package directory
    pkg_qutms_sim = get_package_share_directory('qutms_sim')
    
    # Paths
    urdf_path = os.path.join(pkg_qutms_sim, 'urdf', 'minimal_test.urdf.xacro')
    controller_config = os.path.join(pkg_qutms_sim, 'config', 'minimal_controllers.yaml')
    
    # Process URDF
    robot_description = ParameterValue(
        Command(['xacro ', urdf_path]),
        value_type=str
    )
    
    # Robot State Publisher
    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='screen',
        parameters=[{
            'robot_description': robot_description,
            'use_sim_time': True
        }]
    )
    
    # Clock bridge - sync Gazebo sim time to ROS 2
    clock_bridge = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        arguments=['/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock'],
        output='screen'
    )

    QUTMS = os.path.expanduser(os.environ.get("QUTMS_WS"))
    DISTRO = os.environ.get("ROS_DISTRO")

    # Set GZ Sim environment variables
    os.environ["GZ_SIM_SYSTEM_PLUGIN_PATH"] = (
        QUTMS + "/install/vehicle_plugins/lib:" + "/opt/ros/" + DISTRO + "/lib"
    )
    os.environ["GZ_SIM_RESOURCE_PATH"] = (
        get_package_share_directory("qutms_sim") + "/models:" + 
        get_package_share_directory("qutms_sim") + "/meshes:" +
        get_package_share_directory("qutms_sim") + "/materials:" +
        os.environ.get("GZ_SIM_RESOURCE_PATH", "")
    )

    world_path = os.path.join(get_package_share_directory("qutms_sim"), "worlds", "small_track.sdf")

    # Gazebo Sim
    gz_sim = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare('ros_gz_sim'),
                'launch',
                'gz_sim.launch.py'
            ])
        ]),
        launch_arguments={
            'gz_args': f'-r {world_path}',
            'on_exit_shutdown': 'true'
        }.items()
    )
    
    # Spawn robot
    spawn_entity = Node(
        package='ros_gz_sim',
        executable='create',
        arguments=[
            '-name', 'qutms_minimal',
            '-topic', 'robot_description',
            '-x', '0.0',
            '-y', '0.0',
            '-z', '0.5'
        ],
        output='screen'
    )
    
    # Joint State Broadcaster
    joint_state_broadcaster = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['joint_state_broadcaster', '--controller-manager', '/controller_manager']
    )
    
    # Ackermann Steering Controller
    ackermann_controller = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['ackermann_steering_controller', '--controller-manager', '/controller_manager']
    )
    
    # Delay ackermann controller after joint state broadcaster
    ackermann_controller_delayed = RegisterEventHandler(
        event_handler=OnProcessExit(
            target_action=joint_state_broadcaster,
            on_exit=[ackermann_controller]
        )
    )
    
    return LaunchDescription([
        clock_bridge,
        gz_sim,
        robot_state_publisher,
        spawn_entity,
        joint_state_broadcaster,
        ackermann_controller_delayed
    ])
