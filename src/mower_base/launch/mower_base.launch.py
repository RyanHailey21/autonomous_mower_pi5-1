from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import RegisterEventHandler
from launch.event_handlers import OnProcessExit
from launch_ros.actions import Node


def generate_launch_description():
    package_share = Path(get_package_share_directory('mower_base'))
    robot_description_path = package_share / 'description' / 'mower_base.urdf'
    controllers_path = package_share / 'config' / 'mower_controllers.yaml'

    robot_description = robot_description_path.read_text()

    control_node = Node(
        package='controller_manager',
        executable='ros2_control_node',
        parameters=[
            {'robot_description': robot_description},
            str(controllers_path),
        ],
        output='screen',
    )

    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description}],
        output='screen',
    )

    joint_state_broadcaster_spawner = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['joint_state_broadcaster'],
        output='screen',
    )

    diff_drive_controller_spawner = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['diff_drive_controller'],
        output='screen',
    )

    delayed_diff_drive_spawner = RegisterEventHandler(
        event_handler=OnProcessExit(
            target_action=joint_state_broadcaster_spawner,
            on_exit=[diff_drive_controller_spawner],
        )
    )

    return LaunchDescription([
        control_node,
        robot_state_publisher,
        joint_state_broadcaster_spawner,
        delayed_diff_drive_spawner,
    ])
