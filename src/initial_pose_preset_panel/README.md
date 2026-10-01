# Initial Pose Preset Panel

RViz2の **Panels → Add New Panel** から
`initial_pose_preset_panel/InitialPosePresetPanel` を追加します。

1. RViz2の **2D Pose Estimate** で初期位置を指定します。
2. 名前を入力して **Add Current** を押します。
3. 現在の自己位置を保存する場合は名前を入力して **Add Robot Pose** を押します。
   最新の `map -> base_link` TFを使うため、AMCL・EMCL2のどちらでも利用できます。
4. プリセットを選び **Apply** を押すと、標準の
   `geometry_msgs/msg/PoseWithCovarianceStamped` を `/initialpose` にpublishします。

**Add Robot Pose** で保存した位置には、TFが不確かさを含まないため、初期位置として
標準的なXY 0.5 m、yaw 約15度の対角covarianceを設定します。

初期状態ではプリセットはOSのユーザー設定ディレクトリ内の
`initial_pose_preset_panel/initialpose/initial_pose_presets.yaml` に自動保存されます。
**Open YAML** で既存のYAMLを読み込むと、表示中のプリセットがその内容に切り替わり、
以後の追加・削除もそのファイルへ保存されます。**Save As...** は現在のプリセットを
指定したYAMLへ保存し、以後の保存先もそのファイルに切り替えます（既存ファイルは上書き確認あり）。
選択したファイルのパスはRVizの設定ファイルに保存されるため、次回も使う場合はRVizの設定を保存してください。
