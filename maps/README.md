# Maps

Place the map YAML and image generated for your Gazebo world here, or pass an absolute map YAML path to the launch file.

Example:

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/maps/map.yaml
```

`/absolute/path/to/map.yaml` is only a placeholder. Replace it with a real YAML file path.
The YAML file should reference the map image, for example:

```yaml
image: map.pgm
mode: trinary
resolution: 0.05
origin: [0.0, 0.0, 0.0]
negate: 0
occupied_thresh: 0.65
free_thresh: 0.25
```

RViz starts by default from `navigation.launch.py`. To launch without RViz:

```bash
ros2 launch autonomous_bot_nav navigation.launch.py map:=$PWD/maps/map.yaml use_rviz:=false
```
