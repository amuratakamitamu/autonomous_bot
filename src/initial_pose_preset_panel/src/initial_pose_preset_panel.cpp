#include "initial_pose_preset_panel/initial_pose_preset_panel.hpp"

#include <algorithm>
#include <cmath>

#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMetaObject>
#include <QPushButton>
#include <QSaveFile>
#include <QStandardPaths>
#include <QVBoxLayout>

#include <pluginlib/class_list_macros.hpp>
#include <rviz_common/display_context.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction.hpp>
#include <yaml-cpp/yaml.h>

namespace initial_pose_preset_panel
{

namespace
{
constexpr char kInitialPoseTopic[] = "/initialpose";

bool isFinitePose(const geometry_msgs::msg::PoseWithCovarianceStamped & pose)
{
  const auto & p = pose.pose.pose.position;
  const auto & q = pose.pose.pose.orientation;
  const double q_squared = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
  return !pose.header.frame_id.empty() && std::isfinite(p.x) && std::isfinite(p.y) &&
         std::isfinite(p.z) && std::isfinite(q_squared) && q_squared > 1e-12;
}
}  // namespace

InitialPosePresetPanel::InitialPosePresetPanel(QWidget * parent)
: rviz_common::Panel(parent)
{
  buildUi();
}

void InitialPosePresetPanel::onInitialize()
{
  auto ros_node_abstraction = getDisplayContext()->getRosNodeAbstraction().lock();
  if (!ros_node_abstraction) {
    setStatus("Error: RViz ROS node is unavailable");
    return;
  }
  node_ = ros_node_abstraction->get_raw_node();
  initial_pose_publisher_ = node_->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
    kInitialPoseTopic, rclcpp::QoS(10));
  initial_pose_subscription_ = node_->create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
    kInitialPoseTopic, rclcpp::QoS(10),
    [this](geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr pose) {
      QMetaObject::invokeMethod(this, [this, pose]() {current_pose_ = *pose;}, Qt::QueuedConnection);
    });

  QString error;
  if (!loadPresets(&error)) {
    setStatus("Error: " + error);
    return;
  }
  refreshPresetCombo();
  setStatus("Use 2D Pose Estimate, then add the current pose");
}

void InitialPosePresetPanel::buildUi()
{
  auto * layout = new QVBoxLayout(this);
  layout->setContentsMargins(4, 4, 4, 4);

  preset_combo_ = new QComboBox(this);
  layout->addWidget(preset_combo_);

  auto * apply_button = new QPushButton("Apply", this);
  auto * delete_button = new QPushButton("Delete", this);
  connect(apply_button, &QPushButton::clicked, this, &InitialPosePresetPanel::applySelected);
  connect(delete_button, &QPushButton::clicked, this, &InitialPosePresetPanel::deleteSelected);
  auto * action_row = new QHBoxLayout();
  action_row->addWidget(apply_button);
  action_row->addWidget(delete_button);
  layout->addLayout(action_row);

  auto * add_row = new QHBoxLayout();
  name_edit_ = new QLineEdit(this);
  name_edit_->setPlaceholderText("Preset name");
  auto * add_button = new QPushButton("Add Current", this);
  connect(add_button, &QPushButton::clicked, this, &InitialPosePresetPanel::addCurrent);
  add_row->addWidget(name_edit_);
  add_row->addWidget(add_button);
  layout->addLayout(add_row);
}

void InitialPosePresetPanel::applySelected()
{
  const int index = preset_combo_->currentIndex();
  if (index < 0 || index >= static_cast<int>(presets_.size()) || !initial_pose_publisher_) {
    setStatus("Error: select a preset");
    return;
  }
  auto pose = presets_[index].pose;
  pose.header.stamp = node_->now();
  initial_pose_publisher_->publish(pose);
  setStatus("Applied " + QString::fromStdString(presets_[index].name));
}

void InitialPosePresetPanel::addCurrent()
{
  const QString name = name_edit_->text().trimmed();
  if (!current_pose_ || !isFinitePose(*current_pose_)) {
    setStatus("Error: set an initial pose with 2D Pose Estimate first");
    return;
  }
  if (name.isEmpty()) {
    setStatus("Error: enter a preset name");
    return;
  }
  const std::string name_text = name.toStdString();
  if (std::any_of(presets_.begin(), presets_.end(), [&name_text](const Preset & preset) {
      return preset.name == name_text;
    }))
  {
    setStatus("Error: a preset with that name already exists");
    return;
  }
  presets_.push_back({name_text, *current_pose_});
  QString error;
  if (!savePresets(&error)) {
    presets_.pop_back();
    setStatus("Error: " + error);
    return;
  }
  refreshPresetCombo();
  preset_combo_->setCurrentIndex(preset_combo_->count() - 1);
  name_edit_->clear();
  setStatus("Saved " + name);
}

void InitialPosePresetPanel::deleteSelected()
{
  const int index = preset_combo_->currentIndex();
  if (index < 0 || index >= static_cast<int>(presets_.size())) {
    setStatus("Error: select a preset");
    return;
  }
  const Preset removed = presets_[index];
  presets_.erase(presets_.begin() + index);
  QString error;
  if (!savePresets(&error)) {
    presets_.insert(presets_.begin() + index, removed);
    setStatus("Error: " + error);
    return;
  }
  refreshPresetCombo();
  setStatus("Deleted " + QString::fromStdString(removed.name));
}

void InitialPosePresetPanel::refreshPresetCombo()
{
  const int selected = preset_combo_->currentIndex();
  preset_combo_->clear();
  for (const auto & preset : presets_) {
    preset_combo_->addItem(QString::fromStdString(preset.name));
  }
  if (selected >= 0 && selected < preset_combo_->count()) {
    preset_combo_->setCurrentIndex(selected);
  }
}

QString InitialPosePresetPanel::presetFilePath() const
{
  const QString config_dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  return config_dir + "/initial_pose_presets.yaml";
}

bool InitialPosePresetPanel::loadPresets(QString * error)
{
  const QString filename = presetFilePath();
  if (!QFile::exists(filename)) {
    return true;
  }
  try {
    const YAML::Node entries = YAML::LoadFile(filename.toStdString())["presets"];
    if (!entries || !entries.IsSequence()) {
      *error = "invalid presets YAML";
      return false;
    }
    std::vector<Preset> loaded;
    for (const auto & entry : entries) {
      Preset preset;
      preset.name = entry["name"].as<std::string>();
      auto & pose = preset.pose;
      pose.header.frame_id = entry["frame_id"].as<std::string>();
      const YAML::Node position = entry["position"];
      const YAML::Node orientation = entry["orientation"];
      pose.pose.pose.position.x = position["x"].as<double>();
      pose.pose.pose.position.y = position["y"].as<double>();
      pose.pose.pose.position.z = position["z"].as<double>();
      pose.pose.pose.orientation.x = orientation["x"].as<double>();
      pose.pose.pose.orientation.y = orientation["y"].as<double>();
      pose.pose.pose.orientation.z = orientation["z"].as<double>();
      pose.pose.pose.orientation.w = orientation["w"].as<double>();
      const YAML::Node covariance = entry["covariance"];
      if (!covariance || !covariance.IsSequence() || covariance.size() != pose.pose.covariance.size()) {
        *error = "each preset requires 36 covariance values";
        return false;
      }
      for (size_t i = 0; i < pose.pose.covariance.size(); ++i) {
        pose.pose.covariance[i] = covariance[i].as<double>();
      }
      if (preset.name.empty() || !isFinitePose(pose)) {
        *error = "preset has an invalid name or pose";
        return false;
      }
      loaded.push_back(std::move(preset));
    }
    presets_ = std::move(loaded);
  } catch (const YAML::Exception & exception) {
    *error = QString("invalid YAML: %1").arg(exception.what());
    return false;
  }
  return true;
}

bool InitialPosePresetPanel::savePresets(QString * error) const
{
  const QString filename = presetFilePath();
  if (!QDir().mkpath(QFileInfo(filename).absolutePath())) {
    *error = "cannot create configuration directory";
    return false;
  }
  YAML::Emitter out;
  out << YAML::BeginMap << YAML::Key << "presets" << YAML::Value << YAML::BeginSeq;
  for (const auto & preset : presets_) {
    const auto & pose = preset.pose.pose.pose;
    out << YAML::BeginMap << YAML::Key << "name" << YAML::Value << preset.name;
    out << YAML::Key << "frame_id" << YAML::Value << preset.pose.header.frame_id;
    out << YAML::Key << "position" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "x" << YAML::Value << pose.position.x << YAML::Key << "y" << YAML::Value << pose.position.y;
    out << YAML::Key << "z" << YAML::Value << pose.position.z << YAML::EndMap;
    out << YAML::Key << "orientation" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "x" << YAML::Value << pose.orientation.x << YAML::Key << "y" << YAML::Value << pose.orientation.y;
    out << YAML::Key << "z" << YAML::Value << pose.orientation.z << YAML::Key << "w" << YAML::Value << pose.orientation.w << YAML::EndMap;
    out << YAML::Key << "covariance" << YAML::Value << YAML::Flow << YAML::BeginSeq;
    for (const double value : preset.pose.pose.covariance) {out << value;}
    out << YAML::EndSeq << YAML::EndMap;
  }
  out << YAML::EndSeq << YAML::EndMap;
  QSaveFile file(filename);
  if (!out.good() || !file.open(QIODevice::WriteOnly | QIODevice::Text) ||
    file.write(out.c_str(), static_cast<qint64>(out.size())) < 0 || !file.commit())
  {
    *error = "cannot save presets YAML";
    return false;
  }
  return true;
}

void InitialPosePresetPanel::setStatus(const QString & status)
{
  setToolTip(status);
}

}  // namespace initial_pose_preset_panel

PLUGINLIB_EXPORT_CLASS(initial_pose_preset_panel::InitialPosePresetPanel, rviz_common::Panel)
