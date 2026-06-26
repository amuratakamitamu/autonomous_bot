# autonomous_bot

Gazebo上のTurtleBot3を使って、SLAMによる地図作成からNav2による自律走行までを試すROS 2ワークスペースです。

主要パッケージは `src/autonomous_bot_nav` にあります。詳細なパラメータやlaunch引数は [src/autonomous_bot_nav/README.md](src/autonomous_bot_nav/README.md) を参照してください。

## 前提

- ROS 2がインストール済み
- `turtlebot3_gazebo`、`turtlebot3_teleop`、`nav2_bringup`、`slam_toolbox` などが利用可能

不足している依存関係は、ワークスペースのルートで次のようにインストールします。

```bash
rosdep install --from-paths src --ignore-src -r -y
```

## ビルド

```bash
colcon build --packages-select autonomous_bot_nav
source install/setup.bash
```

以降の相対パスを含むコマンドは、ワークスペースのルートで実行する前提です。必要に応じて各ターミナルでROS 2環境とこのワークスペースをsourceしてから実行します。

```bash
source /opt/ros/$ROS_DISTRO/setup.bash
source install/setup.bash
export TURTLEBOT3_MODEL=burger
```

## 全体の流れ

### 1. GazeboでTurtleBot3 worldを起動する

ターミナル1でGazeboを起動します。

```bash
source /usr/share/gazebo/setup.sh
export TURTLEBOT3_MODEL=waffle_pi
ros2 launch turtlebot3_gazebo turtlebot3_world.launch.py
```

このlaunchにより、Gazebo上にTurtleBot3、`/scan`、`/odom`、TF、`/clock` などが立ち上がります。

### 2. SLAMを起動して地図を作る

ターミナル2でSLAM ToolboxとRVizを起動します。

```bash
ros2 launch autonomous_bot_nav mapping.launch.py use_sim_time:=true
```

RVizで地図が表示されたら、ターミナル3でTurtleBot3を操作してワールド内を走らせます。

```bash
ros2 run turtlebot3_teleop teleop_keyboard
```

地図が必要な範囲を覆ったら、ターミナル4で保存します。このコマンドはワークスペースのルートで実行してください。

```bash
ros2 run nav2_map_server map_saver_cli -f src/autonomous_bot_nav/maps/map
```

保存後、次のファイルが作成または更新されます。

- `src/autonomous_bot_nav/maps/map.yaml`
- `src/autonomous_bot_nav/maps/map.pgm`

### 3. 保存した地図でNav2を起動する

地図作成用のlaunchを止めてから、ターミナル2でNav2を起動します。Gazeboは起動したままにします。

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/src/autonomous_bot_nav/maps/map.yaml use_sim_time:=true
```

`src` ディレクトリ内から実行する場合は、地図パスを次のように指定します。

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/autonomous_bot_nav/maps/map.yaml use_sim_time:=true
```

RVizが開いたら、**2D Pose Estimate** で地図上のロボットの初期姿勢を設定します。
ロボットがいる位置をクリックし、ロボットの正面方向へドラッグして離します。
レーザースキャンが地図の壁と重なるまで数秒待ち、ずれている場合はもう一度 **2D Pose Estimate** を入れ直してからゴールを送ってください。

### 4. RVizからゴールを送る

RVizの **Nav2 Goal** を選び、移動させたい位置をクリックして向きをドラッグします。
Nav2が経路を計画し、Gazebo上のTurtleBot3がゴールへ移動します。

### 5. launch引数で初期姿勢とゴールを送る

RViz操作の代わりに、`nav_goal_client` から初期姿勢とゴールを送ることもできます。

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/src/autonomous_bot_nav/maps/map.yaml \
  use_sim_time:=true start_goal_client:=true publish_initial_pose:=true send_goal:=true \
  initial_x:=0.0 initial_y:=0.0 initial_yaw:=0.0 \
  goal_x:=1.0 goal_y:=0.0 goal_yaw:=0.0
```

角度はラジアンで指定します。

### 6. AMCLのglobal localizationを使う

初期姿勢を正確に設定しにくい場合は、AMCLのglobal localizationを使えます。

```bash
ros2 service call /reinitialize_global_localization std_srvs/srv/Empty {}
```

サービス呼び出し後、レーザースキャンが地図と合うまで、ロボットをその場でゆっくり回転させるか少し動かしてください。

## よく使うコマンド

Gazebo起動:

```bash
ros2 launch turtlebot3_gazebo turtlebot3_world.launch.py
```

地図作成:

```bash
ros2 launch autonomous_bot_nav mapping.launch.py use_sim_time:=true
```

地図保存:

```bash
ros2 run nav2_map_server map_saver_cli -f src/autonomous_bot_nav/maps/map
```

保存済み地図でNav2起動:

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/src/autonomous_bot_nav/maps/map.yaml use_sim_time:=true
```

実機など `/clock` を使わない環境:

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/src/autonomous_bot_nav/maps/map.yaml
```

AMCLのglobal localization:

```bash
ros2 service call /reinitialize_global_localization std_srvs/srv/Empty {}
```
