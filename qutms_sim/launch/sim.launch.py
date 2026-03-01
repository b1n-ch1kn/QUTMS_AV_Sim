import os
from os.path import isfile, join
import yaml

import xacro
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (DeclareLaunchArgument, IncludeLaunchDescription,
                            OpaqueFunction, TimerAction)
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

sim_pkg = get_package_share_directory("qutms_sim")

def get_argument(context, arg):
    return LaunchConfiguration(arg).perform(context)


def gen_world(context, *args, **kwargs):
    track = str(get_argument(context, "track") + ".sdf")

    QUTMS = os.path.expanduser(os.environ.get("QUTMS_WS"))
    DISTRO = os.environ.get("ROS_DISTRO")

    # Set GZ Sim environment variables
    os.environ["GZ_SIM_SYSTEM_PLUGIN_PATH"] = (
        QUTMS + "/install/vehicle_plugins/lib:" + "/opt/ros/" + DISTRO + "/lib"
    )
    os.environ["GZ_SIM_RESOURCE_PATH"] = (
        sim_pkg + "/models:" + 
        sim_pkg + "/meshes:" +
        sim_pkg + "/materials:" +
        os.environ.get("GZ_SIM_RESOURCE_PATH", "")
    )

    world_path = join(sim_pkg, "worlds", track)

    gz_sim_launch = join(get_package_share_directory("ros_gz_sim"), "launch", "gz_sim.launch.py")

    return [
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(gz_sim_launch),
            launch_arguments=[
                ("gz_args", f"-r {world_path} -s"),
                ("on_exit_shutdown", "true"),
            ],
        ),
    ]


def spawn_car(context, *args, **kwargs):
    # get x,y,z,roll,pitch,yaw from track csv file
    track = get_argument(context, "track")

    with open(join(sim_pkg, "worlds", track + ".csv"), "r") as f:
        # car position is last line of csv file
        for line in f:
            pass
        car_pos = line.split(",")
        x = car_pos[1]
        y = car_pos[2]
        yaw = car_pos[3]

    vehicle_config = get_argument(context, "vehicle_config")
    base_frame = get_argument(context, "base_frame")
    display_car = get_argument(context, "display_car")
    namespace = get_argument(context, "namespace")

    # Prepare bridge configuration with world name replacement
    bridge_config_in = join(sim_pkg, "config", "bridge.yaml")
    bridge_config_out = join(sim_pkg, "config", f"bridge_{track}.yaml")
    
    with open(bridge_config_in, "r") as f:
        bridge_yaml = f.read()
    
    # Replace world name placeholder with actual track name
    bridge_yaml = bridge_yaml.replace("WORLD_NAME", track)
    
    with open(bridge_config_out, "w") as f:
        f.write(bridge_yaml)

    xacro_path = join(sim_pkg, "urdf", "robot.urdf.xacro")
    urdf_path = join(sim_pkg, "urdf", "robot.urdf")
    controller_config = join(sim_pkg, "config", "ros2_controllers.yaml")

    if not isfile(urdf_path):
        os.mknod(urdf_path)

    doc = xacro.process_file(
        xacro_path,
        mappings={
            "vehicle_config": vehicle_config,
            "base_frame": base_frame,
            "display_car": display_car,
            "namespace": namespace,
        },
    )
    out = xacro.open_output(urdf_path)
    out.write(doc.toprettyxml(indent="  "))

    with open(urdf_path, "r") as urdf_file:
        robot_description = urdf_file.read()

    return [
        TimerAction(
            period=3.0,
            actions=[
                Node(
                    name="spawn_robot",
                    package="ros_gz_sim",
                    executable="create",
                    output="screen",
                    arguments=[
                        "-name",
                        "QEV-3D",
                        "-file",
                        urdf_path,
                        "-x",
                        x,
                        "-y",
                        y,
                        "-z",
                        "0.5",
                        "-Y",
                        yaw,
                        "--ros-args",
                        "--log-level",
                        "warn",
                    ],
                ),
            ],
        ),
        # robot_state_publisher subscribes to joint_states and publishes TF transforms
        # It automatically uses the namespace, so it listens to /sim/joint_states
        Node(
            name="robot_state_publisher",
            package="robot_state_publisher",
            executable="robot_state_publisher",
            output="screen",
            namespace=namespace,
            parameters=[
                {
                    "robot_description": robot_description,
                    "rate": 200,
                    "use_sim_time": LaunchConfiguration("use_sim_time"),
                }
            ],
            arguments=["--ros-args", "--log-level", "warn"],
        ),
        # Controller spawner - loads and activates joint_state_broadcaster
        TimerAction(period=3.0, actions=[
            Node(
                package="controller_manager",
                executable="spawner",
                namespace=namespace,
                output="screen",
                arguments=[
                    "joint_state_broadcaster",
                    "--param-file",
                    controller_config,
                ],
            ),
        ]),
        Node(
            package="ros_gz_bridge",
            executable="parameter_bridge",
            name="gz_bridge",
            namespace=namespace,
            output="screen",
            parameters=[
                {
                    "use_sim_time": LaunchConfiguration("use_sim_time"),
                    "config_file": bridge_config_out,
                }
            ],
            arguments=["--ros-args", "--log-level", "warn"],
        ),
    ]


def load_visuals(context, *args, **kwargs):
    rviz_config_file = join(sim_pkg, "visuals", "default.rviz")

    return [
        TimerAction(
            period=3.0,
            actions=[
                Node(
                    package="rviz2",
                    executable="rviz2",
                    arguments=["-d", rviz_config_file],
                    condition=IfCondition(LaunchConfiguration("rviz")),
                    parameters=[
                        {"use_sim_time": LaunchConfiguration("use_sim_time")}
                    ],
                ),
            ],
        ),
        Node(
            package="foxglove_bridge",
            executable="foxglove_bridge",
            condition=IfCondition(LaunchConfiguration("foxglove")),
            parameters=[
                {"use_sim_time": LaunchConfiguration("use_sim_time")}
            ],
        ),
    ]

def generate_launch_description():
    default_plugin_yaml = join(sim_pkg, "config", "config.yaml")

    with open(default_plugin_yaml, "r") as f:
        data = yaml.safe_load(f)

    vehicle_config = join(sim_pkg, "config", "vehicle_params.yaml")

    track = data["/**"]["ros__parameters"]["track"]
    sim_time = str(data["/**"]["ros__parameters"]["use_sim_time"])
    rviz = str(data["/**"]["ros__parameters"]["rviz"])
    foxglove = str(data["/**"]["ros__parameters"]["foxglove"])
    display_car = data["/**"]["ros__parameters"]["display_car"]
    namespace = data["/**"]["ros__parameters"]["namespace"]
    base_frame = data["/**"]["ros__parameters"]["base_frame"]

    # load yaml file
    return LaunchDescription(
        [
            DeclareLaunchArgument(
                name="use_sim_time",
                default_value=sim_time,
                description="Use simulation (Gazebo) clock if true",
            ),
            DeclareLaunchArgument(
                name="track",
                default_value=track,
                description="Determines which track is launched",
            ),
            DeclareLaunchArgument(
                name="vehicle_config",
                default_value=vehicle_config,
                description="Determines the file from which the vehicle model parameters are read",
            ),
            DeclareLaunchArgument(
                name="rviz",
                default_value=rviz,
                description="Condition to launch the Rviz GUI",
            ),
            DeclareLaunchArgument(
                name="foxglove",
                default_value=foxglove,
                description="Condition to launch the Foxglove GUI",
            ),
            DeclareLaunchArgument(
                name="base_frame",
                default_value=base_frame,
                description="ROS transform frame for the vehicle base",
            ),
            DeclareLaunchArgument(
                name="namespace",
                default_value=namespace,
                description="ROS namespace for the vehicle",
            ),
            DeclareLaunchArgument(
                name="display_car",
                default_value=display_car,
                description="Determines if the car is displayed in Rviz",
            ),
            OpaqueFunction(function=load_visuals),
            # launch the gazebo world
            OpaqueFunction(function=gen_world),
            # launch the car
            OpaqueFunction(function=spawn_car),
        ]
    )

if __name__ == "__main__":
    generate_launch_description()