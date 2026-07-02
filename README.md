# autonomous_bot

Nav2による自律移動

## Requirements

- ROS 2がインストール済み
- `turtlebot3_gazebo`、`turtlebot3_teleop`、`nav2_bringup`、`slam_toolbox`

不足している場合はインストールコマンド:

```bash
rosdep install --from-paths src --ignore-src -r -y
```

## Build

```bash
colcon build --packages-select autonomous_bot_nav
source install/setup.sh
```

## Run

### 1. GazeboでTurtleBot3 worldを起動する

ターミナル1でGazeboを起動します。

```bash
source /usr/share/gazebo/setup.sh
export TURTLEBOT3_MODEL=burger
ros2 launch turtlebot3_gazebo turtlebot3_world.launch.py
```

このlaunchにより、Gazebo上にTurtleBot3、`/scan`、`/odom`、TF、`/clock` などが立ち上がります。

### 2. SLAMを起動して地図を作る

#### SLAMを起動

```bash
ros2 launch autonomous_bot_nav mapping.launch.py use_sim_time:=true
```

#### キーボード操作(/cmd_vel)
```bash
ros2 run turtlebot3_teleop teleop_keyboard
```

#### 地図の保存

```bash
ros2 run nav2_map_server map_saver_cli -f src/autonomous_bot_nav/maps/map
```

保存後、次のファイルが作成または更新される

- `src/autonomous_bot_nav/maps/map.yaml`
- `src/autonomous_bot_nav/maps/map.pgm`

### 3. 保存した地図でNav2を起動

自己位置推定のアルゴリズムを変更したい場合，`localization:=`パラメータを変更すること

- `localization:=amcl`（デフォルト）
- `localization:=emcl2`

#### Simulator

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/src/autonomous_bot_nav/maps/turtlebot3.yaml use_sim_time:=true localization:=emcl2
```

#### Real Robot
事前作成済みの地図がある場合は，mapパスで指定すること

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/src/autonomous_bot_nav/maps/map.yaml use_sim_time:=false localization:=emcl2
```


RVizの**2D Pose Estimate** で地図上のロボットの初期姿勢を設定する

### 4. RVizからゴールを送る

RVizからゴールの2D Poseを指定

## 現在の構成

このワークスペースは、`autonomous_bot_nav` という単一のROS 2パッケージで構成されています。
Gazebo上のTurtleBot3など、外部で起動したロボットから `/scan`、`/odom`、TFを受け取り、このパッケージ側でSLAMによる地図作成とNav2による自律走行を行います。

```text
autonomous_bot/
├── README.md
├── src/
│   └── autonomous_bot_nav/
│       ├── package.xml
│       ├── CMakeLists.txt
│       ├── launch/
│       │   ├── mapping.launch.py
│       │   └── navigation.launch.py
│       ├── config/
│       │   ├── slam_toolbox_params.yaml
│       │   └── nav2_params.yaml
│       ├── maps/
│       │   ├── map.yaml
│       │   └── map.pgm
│       └── rviz/
│           ├── mapping.rviz
│           └── nav2_default.rviz
├── build/
├── install/
└── log/
```

主なファイルの役割は次の通りです。

- `src/autonomous_bot_nav/launch/mapping.launch.py`: `slam_toolbox` とRVizを起動して地図を作成します。
- `src/autonomous_bot_nav/launch/navigation.launch.py`: 保存済み地図を使ってNav2、AMCL、RVizを起動します。
- `src/autonomous_bot_nav/config/slam_toolbox_params.yaml`: SLAM Toolboxの地図作成用パラメータです。
- `src/autonomous_bot_nav/config/nav2_params.yaml`: AMCL、Planner、Controller、CostmapなどのNav2設定です。
- `src/autonomous_bot_nav/maps/`: 保存済み地図の `map.yaml` と `map.pgm` を置くディレクトリです。
- `src/autonomous_bot_nav/rviz/`: 地図作成用とナビゲーション用のRViz設定を置くディレクトリです。

前提としている主なトピックとTF構成は次の通りです。

```text
LaserScan: /scan
Odometry:  /odom
TF:        map -> odom -> base_footprint
```

全体の流れは、TurtleBot3 Gazeboを起動し、`mapping.launch.py` で地図を作成して保存し、その地図を `navigation.launch.py` に渡してAMCL自己位置推定とNav2ゴール走行を行う構成です。

## Nav2の内部構成

このパッケージのNav2設定では、自己位置推定、経路計画、経路追従に次の構成を使っています。

- 自己位置推定: AMCL
  - ノード設定: `amcl`
  - 使用モデル: `nav2_amcl::DifferentialMotionModel`
  - 入力: `/scan`, `/odom`
  - フレーム: `map`, `odom`, `base_footprint`
- 経路計画 Planner: NavfnPlanner
  - 設定名: `GridBased`
  - プラグイン: `nav2_navfn_planner/NavfnPlanner`
  - `use_astar: true` のため、A*ベースでグローバル経路を作成します。
  - `allow_unknown: true` のため、未知領域も経路候補に含めます。
- 経路追従 Controller: Regulated Pure Pursuit
  - 設定名: `FollowPath`
  - プラグイン: `nav2_regulated_pure_pursuit_controller::RegulatedPurePursuitController`
  - 目標並進速度: `desired_linear_vel: 0.4`
  - 回頭時の角速度: `rotate_to_heading_angular_vel: 1.0`
  - 障害物衝突チェック: `use_collision_detection: true`
- Local costmap
  - `voxel_layer`
  - `inflation_layer`
  - 入力: `/scan`
- Global costmap
  - `static_layer`
  - `obstacle_layer`
  - `inflation_layer`
  - 入力: `/scan`
- Behavior / Recovery
  - `Spin`
  - `BackUp`
  - `DriveOnHeading`
  - `AssistedTeleop`
  - `Wait`
- Velocity smoother
  - 最大速度: `[0.4, 0.0, 1.0]`
  - 最小速度: `[-0.26, 0.0, -1.0]`
  - フィードバック方式: `OPEN_LOOP`

まとめると、AMCLで自己位置を推定し、NavfnPlannerが保存済み地図とCostmapから大域経路を作り、Regulated Pure Pursuit Controllerがその経路を追従します。障害物情報は `/scan` を使ってLocal/Global costmapに反映されます。
