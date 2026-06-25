# 地図

Gazeboワールド用に生成した地図のYAMLと画像をここに置くか、launchファイルに地図YAMLの絶対パスを渡してください。

## 地図を作成する

先にロボットまたはGazeboワールドを起動し、その後SLAMを実行します。

```bash
ros2 launch autonomous_bot_nav mapping.launch.py
```

RViz上の地図が必要な範囲を覆うまでロボットを走らせます。その後、地図を保存します。

```bash
ros2 run nav2_map_server map_saver_cli -f maps/map
```

これにより、`maps/map.yaml` と `maps/map.pgm` が作成されます。

`/scan`、`/odom`、またはTFフレーム名が異なる場合は、`config/slam_toolbox_params.yaml` を更新してください。

## 保存済みの地図を使う

例:

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/maps/map.yaml
```

RVizが開いたら、**2D Pose Estimate** で地図上のロボット姿勢を設定します。
ロボットがいる位置をクリックし、ロボットの前方方向へドラッグしてから離してください。
ナビゲーション目標を送る前にこの操作を行ってください。AMCLは、レーザースキャンを地図へ正しく変換するために初期姿勢を必要とします。
姿勢を設定するまでは、`AMCL cannot publish a pose` や `map frame does not exist` のような警告が出ることがあります。

姿勢を正確に設定しにくい場合は、AMCLのglobal localizationを使います。

```bash
ros2 service call /reinitialize_global_localization std_srvs/srv/Empty {}
```

その後、レーザースキャンが地図と合うまで、ロボットをその場でゆっくり回転させるか、少し動かしてください。
このサービス呼び出しの直後は、表示されるロボット姿勢が地図の中央付近に見えることがあります。これはAMCLのパーティクルが地図全体に広がっている間は正常な挙動です。

`config/nav2_params.yaml` のAMCLパラメータは、以下のように調整されています。

- global search用にパーティクル数を増やす
- マッチング用にレーザービーム数を増やす
- 小さな回転や移動でもAMCLが更新されるように移動しきい値を小さくする
- 保存済み地図に存在しない障害物の影響を減らすためにbeam skippingを使う

RVizではなくlaunchファイルから初期姿勢をpublishする場合は、姿勢を明示的に渡します。

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/maps/map.yaml \
  start_goal_client:=true publish_initial_pose:=true \
  initial_x:=0.0 initial_y:=0.0 initial_yaw:=0.0
```

`/clock` トピックがない実機ロボットでは、`use_sim_time:=false` を指定して起動してください。

`/absolute/path/to/map.yaml` はプレースホルダーです。実際のYAMLファイルパスに置き換えてください。
YAMLファイルでは、たとえば次のように地図画像を参照します。

```yaml
image: map.pgm
mode: trinary
resolution: 0.05
origin: [0.0, 0.0, 0.0]
negate: 0
occupied_thresh: 0.65
free_thresh: 0.25
```

`navigation.launch.py` ではデフォルトでRVizが起動します。RVizなしで起動する場合:

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/maps/map.yaml use_rviz:=false
```

RVizのレーザースキャン表示では、地図とスキャンの位置合わせを見やすくするために四角いマーカーを使っています。
