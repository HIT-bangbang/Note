# 安装ros
安装ros2就按照官方文档来即可，其中第一步，切换语言可以不用切换，locale一下发现就是UTF-8的直接安装即可


注意现在ros2中将不会自动安装gazebo 

# 安装gazebo

查看gazebo教程https://gazebosim.org/docs/garden/ros_installation，ros环境下安装的命令为

ros-humble-ros-gz

这会直接安装These packages will automatically install both Gazebo Fortress and the ros-gz bridge.

 export GAZEBO_MODEL_PATH=$GAZEBO_MODEL_PATH:/opt/ros/galactic/share/turtlebot3_gazebo/models
