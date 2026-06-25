cmake_minimum_required(VERSION 3.8)
project(autonomous_bot_nav)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(tf2 REQUIRED)

add_executable(nav_goal_client src/nav_goal_client.cpp)
ament_target_dependencies(nav_goal_client
  geometry_msgs
  nav2_msgs
  rclcpp
  rclcpp_action
  tf2
)

install(TARGETS nav_goal_client
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY config launch maps rviz
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
