# Initial Pose Preset Panel

RViz2の **Panels → Add New Panel** から
`initial_pose_preset_panel/InitialPosePresetPanel` を追加します。

1. RViz2の **2D Pose Estimate** で初期位置を指定します。
2. 名前を入力して **Add Current** を押します。
3. プリセットを選び **Apply** を押すと、標準の
   `geometry_msgs/msg/PoseWithCovarianceStamped` を `/initialpose` にpublishします。

プリセットはOSのユーザー設定ディレクトリにある
`initial_pose_presets.yaml` に自動保存されます。
