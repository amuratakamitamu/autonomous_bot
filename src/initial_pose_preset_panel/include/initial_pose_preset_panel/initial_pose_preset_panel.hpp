#ifndef INITIAL_POSE_PRESET_PANEL__INITIAL_POSE_PRESET_PANEL_HPP_
#define INITIAL_POSE_PRESET_PANEL__INITIAL_POSE_PRESET_PANEL_HPP_

#include <optional>
#include <string>
#include <vector>

#include <QWidget>

#include <geometry_msgs/msg/pose_with_covariance_stamped.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rviz_common/panel.hpp>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>

class QComboBox;
class QLineEdit;

namespace initial_pose_preset_panel
{

class InitialPosePresetPanel : public rviz_common::Panel
{
  Q_OBJECT

public:
  explicit InitialPosePresetPanel(QWidget * parent = nullptr);
  void onInitialize() override;
  void load(const rviz_common::Config & config) override;
  void save(rviz_common::Config config) const override;

private Q_SLOTS:
  void applySelected();
  void addCurrent();
  void addRobotPose();
  void deleteSelected();
  void openPresetFile();
  void savePresetFileAs();

private:
  struct Preset
  {
    std::string name;
    geometry_msgs::msg::PoseWithCovarianceStamped pose;
  };

  void buildUi();
  void addPose(const geometry_msgs::msg::PoseWithCovarianceStamped & pose);
  void refreshPresetCombo();
  void setStatus(const QString & status);
  bool loadPresets(const QString & filename, QString * error);
  bool savePresets(const QString & filename, QString * error) const;

  rclcpp::Node::SharedPtr node_;
  rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr initial_pose_publisher_;
  rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr initial_pose_subscription_;
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
  std::optional<geometry_msgs::msg::PoseWithCovarianceStamped> current_pose_;
  std::vector<Preset> presets_;
  QString preset_file_path_;

  QComboBox * preset_combo_{nullptr};
  QLineEdit * name_edit_{nullptr};
  QLineEdit * path_edit_{nullptr};
};

}  // namespace initial_pose_preset_panel

#endif  // INITIAL_POSE_PRESET_PANEL__INITIAL_POSE_PRESET_PANEL_HPP_
