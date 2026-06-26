import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    pkg_share = get_package_share_directory("autonomous_bot_nav")
    nav2_share = get_package_share_directory("nav2_bringup")

    default_params = os.path.join(pkg_share, "config", "nav2_params.yaml")
    default_rviz_config = os.path.join(pkg_share, "rviz", "nav2_default.rviz")
    bringup_launch = os.path.join(nav2_share, "launch", "bringup_launch.py")

    namespace = LaunchConfiguration("namespace")
    map_yaml = LaunchConfiguration("map")
    params_file = LaunchConfiguration("params_file")
    rviz_config = LaunchConfiguration("rviz_config")
    use_sim_time = LaunchConfiguration("use_sim_time")
    autostart = LaunchConfiguration("autostart")
    use_composition = LaunchConfiguration("use_composition")
    use_rviz = LaunchConfiguration("use_rviz")
    start_goal_client = LaunchConfiguration("start_goal_client")
    publish_initial_pose = LaunchConfiguration("publish_initial_pose")
    send_goal = LaunchConfiguration("send_goal")
    initial_x = LaunchConfiguration("initial_x")
    initial_y = LaunchConfiguration("initial_y")
    initial_yaw = LaunchConfiguration("initial_yaw")
    goal_x = LaunchConfiguration("goal_x")
    goal_y = LaunchConfiguration("goal_y")
    goal_yaw = LaunchConfiguration("goal_yaw")

    def validate_map_file(context, *args, **kwargs):
        map_path = map_yaml.perform(context)
        if not os.path.isfile(map_path):
            raise RuntimeError(
                f"Map YAML file does not exist: {map_path}\n"
                "Pass a real map path, for example: "
                "map:=$PWD/src/autonomous_bot_nav/maps/map.yaml"
            )
        return []

    return LaunchDescription([
        DeclareLaunchArgument("namespace", default_value=""),
        DeclareLaunchArgument("map", description="Full path to the map yaml file."),
        DeclareLaunchArgument("params_file", default_value=default_params),
        DeclareLaunchArgument("rviz_config", default_value=default_rviz_config),
        DeclareLaunchArgument("use_sim_time", default_value="false"),
        DeclareLaunchArgument("autostart", default_value="true"),
        DeclareLaunchArgument("use_composition", default_value="True"),
        DeclareLaunchArgument("use_rviz", default_value="true"),
        DeclareLaunchArgument("start_goal_client", default_value="false"),
        DeclareLaunchArgument("publish_initial_pose", default_value="false"),
        DeclareLaunchArgument("send_goal", default_value="false"),
        DeclareLaunchArgument("initial_x", default_value="0.0"),
        DeclareLaunchArgument("initial_y", default_value="0.0"),
        DeclareLaunchArgument("initial_yaw", default_value="0.0"),
        DeclareLaunchArgument("goal_x", default_value="1.0"),
        DeclareLaunchArgument("goal_y", default_value="0.0"),
        DeclareLaunchArgument("goal_yaw", default_value="0.0"),

        OpaqueFunction(function=validate_map_file),

        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(bringup_launch),
            launch_arguments={
                "namespace": namespace,
                "use_namespace": "False",
                "slam": "False",
                "map": map_yaml,
                "use_sim_time": use_sim_time,
                "params_file": params_file,
                "autostart": autostart,
                "use_composition": use_composition,
            }.items(),
        ),

        Node(
            condition=IfCondition(start_goal_client),
            package="autonomous_bot_nav",
            executable="nav_goal_client",
            name="nav_goal_client",
            namespace=namespace,
            output="screen",
            parameters=[{
                "use_sim_time": use_sim_time,
                "publish_initial_pose": publish_initial_pose,
                "send_goal": send_goal,
                "initial_x": initial_x,
                "initial_y": initial_y,
                "initial_yaw": initial_yaw,
                "goal_x": goal_x,
                "goal_y": goal_y,
                "goal_yaw": goal_yaw,
            }],
        ),

        Node(
            condition=IfCondition(use_rviz),
            package="rviz2",
            executable="rviz2",
            name="rviz2",
            namespace=namespace,
            output="screen",
            arguments=["-d", rviz_config],
            parameters=[{"use_sim_time": use_sim_time}],
        ),
    ])
