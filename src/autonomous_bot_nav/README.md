# autonomous_bot_nav

`autonomous_bot_nav` は、Gazebo上のTurtleBot系ロボット、または同等の差動二輪ロボットをNav2で自律走行させるためのROS 2パッケージです。

SLAM Toolboxによる地図作成、保存済み地図を使ったAMCL自己位置推定、Nav2の経路計画と追従、RVizでの可視化、初期姿勢とゴールを送る簡易クライアントを含みます。

## 主な機能

- `slam_toolbox` を使った地図作成
- `nav2_bringup` を使ったNav2起動
- AMCLによる保存済み地図上での自己位置推定
- RViz設定ファイル付きの地図作成・ナビゲーション表示
- `nav_goal_client` による初期姿勢publishと `NavigateToPose` ゴール送信
- 実機向けの `use_sim_time:=false` をデフォルトにし、Gazeboでは `use_sim_time:=true` を指定して切り替え

## 前提

このパッケージは、次のトピックとTF構成を前提にしています。

- LaserScan: `/scan`
- Odometry: `/odom`
- TF: `map -> odom -> base_footprint`

名前が異なる場合は、`config/nav2_params.yaml` と `config/slam_toolbox_params.yaml` を環境に合わせて変更してください。

## 依存パッケージ

主な依存は以下です。

- `nav2_bringup`
- `nav2_amcl`
- `nav2_map_server`
- `nav2_planner`
- `nav2_controller`
- `nav2_bt_navigator`
- `slam_toolbox`
- `rviz2`

依存関係は `package.xml` に定義されています。ROS 2環境をsourceしたうえで、必要に応じて `rosdep` を使ってインストールしてください。

```bash
rosdep install --from-paths src --ignore-src -r -y
```

## ビルド

ワークスペースのルートでビルドします。

```bash
colcon build --packages-select autonomous_bot_nav
source install/setup.bash
```

以降の相対パスを含むコマンドは、ワークスペースのルートで実行する前提です。

## 地図を作成する

先にロボットまたはGazeboワールドを起動し、その後SLAMを起動します。

```bash
ros2 launch autonomous_bot_nav mapping.launch.py
```

RViz上の地図が必要な範囲を覆うまでロボットを走らせます。完了したら、ワークスペースのルートで地図を保存します。

```bash
ros2 run nav2_map_server map_saver_cli -f src/autonomous_bot_nav/maps/map
```

これにより、以下のファイルが作成されます。

- `src/autonomous_bot_nav/maps/map.yaml`
- `src/autonomous_bot_nav/maps/map.pgm`

RVizなしで地図作成を実行する場合は、次のように起動します。

```bash
ros2 launch autonomous_bot_nav mapping.launch.py use_rviz:=false
```

Gazeboなど `/clock` トピックを使う場合は、`use_sim_time:=true` を指定します。

```bash
ros2 launch autonomous_bot_nav mapping.launch.py use_sim_time:=true
```

## 保存済み地図でナビゲーションする

保存済み地図を指定してNav2を起動します。

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/src/autonomous_bot_nav/maps/map.yaml
```

`src` ディレクトリ内から実行する場合は、地図パスを次のように指定します。

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/autonomous_bot_nav/maps/map.yaml
```

RVizが開いたら、**2D Pose Estimate** で地図上のロボット姿勢を設定します。
ロボットがいる位置をクリックし、ロボットの前方方向へドラッグしてから離してください。

この初期姿勢は、AMCLがレーザースキャンを地図へ正しく合わせるために必要です。姿勢を設定するまでは、`AMCL cannot publish a pose` や `map frame does not exist` のような警告が出ることがあります。
レーザースキャンが地図の壁と重なるまで数秒待ち、ずれている場合はもう一度 **2D Pose Estimate** を入れ直してからゴールを送ってください。

RVizなしで起動する場合は、次のように指定します。

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/src/autonomous_bot_nav/maps/map.yaml use_rviz:=false
```

Gazeboなど `/clock` トピックを使う場合は、`use_sim_time:=true` を指定します。

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/src/autonomous_bot_nav/maps/map.yaml use_sim_time:=true
```

## 初期姿勢とゴールをlaunchから送る

`nav_goal_client` を起動すると、RVizを使わずに初期姿勢をpublishできます。

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/src/autonomous_bot_nav/maps/map.yaml \
  start_goal_client:=true publish_initial_pose:=true \
  initial_x:=0.0 initial_y:=0.0 initial_yaw:=0.0
```

ナビゲーションゴールも送る場合は、`send_goal:=true` とゴール座標を指定します。

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/src/autonomous_bot_nav/maps/map.yaml \
  start_goal_client:=true publish_initial_pose:=true send_goal:=true \
  initial_x:=0.0 initial_y:=0.0 initial_yaw:=0.0 \
  goal_x:=1.0 goal_y:=0.0 goal_yaw:=0.0
```

角度はラジアンで指定します。

## AMCLのglobal localizationを使う

初期姿勢を正確に設定しにくい場合は、AMCLのglobal localizationを使えます。

```bash
ros2 service call /reinitialize_global_localization std_srvs/srv/Empty {}
```

サービス呼び出し後、レーザースキャンが地図と合うまで、ロボットをその場でゆっくり回転させるか少し動かしてください。直後にロボット姿勢が地図中央付近に見えることがありますが、AMCLのパーティクルが地図全体に広がっている間は正常な挙動です。

## 地図ファイル

`navigation.launch.py` の `map` 引数には、実際に存在する地図YAMLファイルを指定する必要があります。

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=/absolute/path/to/map.yaml
```

地図YAMLは、たとえば次のように地図画像を参照します。

```yaml
image: map.pgm
mode: trinary
resolution: 0.05
origin: [0.0, 0.0, 0.0]
negate: 0
occupied_thresh: 0.65
free_thresh: 0.25
```

## 主要ファイル

- `launch/mapping.launch.py`: SLAM ToolboxとRVizを起動するlaunchファイル
- `launch/navigation.launch.py`: Nav2、RViz、任意で `nav_goal_client` を起動するlaunchファイル
- `config/slam_toolbox_params.yaml`: 地図作成用のSLAM Toolbox設定
- `config/nav2_params.yaml`: AMCL、Planner、Controller、CostmapなどのNav2設定
- `src/nav_goal_client.cpp`: 初期姿勢とナビゲーションゴールを送るC++ノード
- `rviz/mapping.rviz`: 地図作成用RViz設定
- `rviz/nav2_default.rviz`: ナビゲーション用RViz設定
- `maps/`: 保存済み地図を置くディレクトリ

## よく使う起動コマンド

地図作成:

```bash
ros2 launch autonomous_bot_nav mapping.launch.py
```

保存済み地図でNav2起動:

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/src/autonomous_bot_nav/maps/map.yaml
```

Gazebo向けNav2起動:

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/src/autonomous_bot_nav/maps/map.yaml use_sim_time:=true
```

初期姿勢とゴールを送信:

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/src/autonomous_bot_nav/maps/map.yaml \
  start_goal_client:=true publish_initial_pose:=true send_goal:=true \
  initial_x:=0.0 initial_y:=0.0 initial_yaw:=0.0 \
  goal_x:=1.0 goal_y:=0.0 goal_yaw:=0.0
```
