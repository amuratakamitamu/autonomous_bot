#include <chrono>
#include <cmath>
#include <memory>
#include <string>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"
#include "nav2_msgs/action/navigate_to_pose.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "tf2/LinearMath/Quaternion.h"

using namespace std::chrono_literals;

class NavGoalClient : public rclcpp::Node
{
public:
  using NavigateToPose = nav2_msgs::action::NavigateToPose;
  using GoalHandleNavigateToPose = rclcpp_action::ClientGoalHandle<NavigateToPose>;

  NavGoalClient()
  : Node("nav_goal_client")
  {
    publish_initial_pose_ = declare_parameter("publish_initial_pose", true);
    send_goal_ = declare_parameter("send_goal", false);
    initial_x_ = declare_parameter("initial_x", 0.0);
    initial_y_ = declare_parameter("initial_y", 0.0);
    initial_yaw_ = declare_parameter("initial_yaw", 0.0);
    goal_x_ = declare_parameter("goal_x", 1.0);
    goal_y_ = declare_parameter("goal_y", 0.0);
    goal_yaw_ = declare_parameter("goal_yaw", 0.0);
    global_frame_ = declare_parameter<std::string>("global_frame", "map");

    initial_pose_pub_ = create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
      "initialpose", rclcpp::QoS(1).reliable().transient_local());
    nav_client_ = rclcpp_action::create_client<NavigateToPose>(this, "navigate_to_pose");

    timer_ = create_wall_timer(1s, std::bind(&NavGoalClient::on_timer, this));
  }

private:
  static geometry_msgs::msg::Quaternion yaw_to_quaternion(const double yaw)
  {
    tf2::Quaternion quat;
    quat.setRPY(0.0, 0.0, yaw);

    geometry_msgs::msg::Quaternion msg;
    msg.x = quat.x();
    msg.y = quat.y();
    msg.z = quat.z();
    msg.w = quat.w();
    return msg;
  }

  void publish_initial_pose()
  {
    geometry_msgs::msg::PoseWithCovarianceStamped pose;
    pose.header.stamp = now();
    pose.header.frame_id = global_frame_;
    pose.pose.pose.position.x = initial_x_;
    pose.pose.pose.position.y = initial_y_;
    pose.pose.pose.orientation = yaw_to_quaternion(initial_yaw_);

    pose.pose.covariance[0] = 0.25;
    pose.pose.covariance[7] = 0.25;
    pose.pose.covariance[35] = 0.06853891945200942;

    initial_pose_pub_->publish(pose);
    RCLCPP_INFO(
      get_logger(), "Published AMCL initial pose: x=%.3f y=%.3f yaw=%.3f",
      initial_x_, initial_y_, initial_yaw_);
  }

  void send_navigation_goal()
  {
    if (!nav_client_->wait_for_action_server(1s)) {
      RCLCPP_INFO_THROTTLE(
        get_logger(), *get_clock(), 5000,
        "Waiting for Nav2 NavigateToPose action server...");
      return;
    }

    NavigateToPose::Goal goal_msg;
    goal_msg.pose.header.stamp = now();
    goal_msg.pose.header.frame_id = global_frame_;
    goal_msg.pose.pose.position.x = goal_x_;
    goal_msg.pose.pose.position.y = goal_y_;
    goal_msg.pose.pose.orientation = yaw_to_quaternion(goal_yaw_);

    rclcpp_action::Client<NavigateToPose>::SendGoalOptions options;
    options.goal_response_callback =
      [this](const GoalHandleNavigateToPose::SharedPtr & goal_handle) {
        if (!goal_handle) {
          RCLCPP_ERROR(get_logger(), "Navigation goal was rejected.");
          return;
        }
        RCLCPP_INFO(get_logger(), "Navigation goal accepted.");
      };
    options.feedback_callback =
      [this](
        GoalHandleNavigateToPose::SharedPtr,
        const std::shared_ptr<const NavigateToPose::Feedback> feedback) {
        RCLCPP_INFO_THROTTLE(
          get_logger(), *get_clock(), 3000,
          "Distance remaining: %.3f m", feedback->distance_remaining);
      };
    options.result_callback =
      [this](const GoalHandleNavigateToPose::WrappedResult & result) {
        switch (result.code) {
          case rclcpp_action::ResultCode::SUCCEEDED:
            RCLCPP_INFO(get_logger(), "Navigation goal succeeded.");
            break;
          case rclcpp_action::ResultCode::ABORTED:
            RCLCPP_ERROR(get_logger(), "Navigation goal was aborted.");
            break;
          case rclcpp_action::ResultCode::CANCELED:
            RCLCPP_WARN(get_logger(), "Navigation goal was canceled.");
            break;
          default:
            RCLCPP_ERROR(get_logger(), "Navigation goal finished with an unknown result.");
            break;
        }
      };

    nav_client_->async_send_goal(goal_msg, options);
    goal_sent_ = true;
    RCLCPP_INFO(
      get_logger(), "Sent navigation goal: x=%.3f y=%.3f yaw=%.3f",
      goal_x_, goal_y_, goal_yaw_);
  }

  void on_timer()
  {
    if (publish_initial_pose_ && initial_pose_publish_count_ < 5) {
      publish_initial_pose();
      ++initial_pose_publish_count_;
      return;
    }

    if (send_goal_ && !goal_sent_) {
      send_navigation_goal();
    }

    if (!send_goal_ || goal_sent_) {
      timer_->cancel();
    }
  }

  rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr initial_pose_pub_;
  rclcpp_action::Client<NavigateToPose>::SharedPtr nav_client_;
  rclcpp::TimerBase::SharedPtr timer_;

  bool publish_initial_pose_{true};
  bool send_goal_{false};
  bool goal_sent_{false};
  int initial_pose_publish_count_{0};
  double initial_x_{0.0};
  double initial_y_{0.0};
  double initial_yaw_{0.0};
  double goal_x_{1.0};
  double goal_y_{0.0};
  double goal_yaw_{0.0};
  std::string global_frame_{"map"};
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<NavGoalClient>());
  rclcpp::shutdown();
  return 0;
}
