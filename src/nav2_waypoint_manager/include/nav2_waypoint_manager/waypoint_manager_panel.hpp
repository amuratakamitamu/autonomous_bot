#ifndef NAV2_WAYPOINT_MANAGER__WAYPOINT_MANAGER_PANEL_HPP_
#define NAV2_WAYPOINT_MANAGER__WAYPOINT_MANAGER_PANEL_HPP_

#include <memory>
#include <string>
#include <vector>

#include <QWidget>

#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/point_stamped.hpp>
#include <nav2_msgs/action/navigate_through_poses.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <rviz_common/panel.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

class QLabel;
class QListWidget;
class QPushButton;

namespace nav2_waypoint_manager
{

class WaypointManagerPanel : public rviz_common::Panel
{
  Q_OBJECT

public:
  explicit WaypointManagerPanel(QWidget * parent = nullptr);
  ~WaypointManagerPanel() override;

  void onInitialize() override;

private Q_SLOTS:
  void deleteSelected();
  void clearWaypoints();
  void moveUp();
  void moveDown();
  void saveYaml();
  void loadYaml();
  void startFollowing();
  void cancelFollowing();
  void toggleStop();
  void resumeFollowing();
  void setStatus(const QString & status);
  void handleResult(int result_code);

private:
  using NavigateThroughPoses = nav2_msgs::action::NavigateThroughPoses;
  using GoalHandleNavigateThroughPoses = rclcpp_action::ClientGoalHandle<NavigateThroughPoses>;

  void buildUi();
  void refreshList();
  void publishMarkers();
  void updateButtons();
  void addClickedPoint(const geometry_msgs::msg::PointStamped & point);
  void addClickedPose(const geometry_msgs::msg::PoseStamped & pose);
  void sendSegment();
  bool readYamlFile(const QString & filename, std::vector<geometry_msgs::msg::PoseStamped> * poses,
                    std::vector<bool> * stops, std::string * frame_id, QString * error) const;
  bool writeYamlFile(const QString & filename, QString * error) const;
  QString formatWaypoint(int index, const geometry_msgs::msg::PoseStamped & pose) const;

  std::string frame_id_{"map"};
  std::vector<geometry_msgs::msg::PoseStamped> waypoints_;
  std::vector<bool> stop_flags_;  // parallel to waypoints_
  size_t next_index_{0};  // first waypoint of the next segment; >0 while paused at a stop
  size_t segment_end_{0};
  rclcpp::Node::SharedPtr node_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr marker_publisher_;
  rclcpp::Subscription<geometry_msgs::msg::PointStamped>::SharedPtr clicked_point_subscription_;
  rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr clicked_pose_subscription_;
  rclcpp_action::Client<NavigateThroughPoses>::SharedPtr action_client_;
  GoalHandleNavigateThroughPoses::SharedPtr active_goal_;

  QListWidget * waypoint_list_{nullptr};
  QLabel * status_label_{nullptr};
  QPushButton * start_button_{nullptr};
  QPushButton * cancel_button_{nullptr};
  QPushButton * resume_button_{nullptr};
};

}  // namespace nav2_waypoint_manager

#endif  // NAV2_WAYPOINT_MANAGER__WAYPOINT_MANAGER_PANEL_HPP_
