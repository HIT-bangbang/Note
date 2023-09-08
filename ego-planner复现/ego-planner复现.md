# ego-planner复现

配置步骤参照github

https://github.com/ZJU-FAST-Lab/Fast-Drone-250

realsense 

https://github.com/IntelRealSense/librealsense/blob/master/doc/distribution_linux.md



仓库中自带的3rd_party.zip包中有ceres 2.0.0 rc1版本和glog(ceres需要)。ceres可以直接安装2.0.0正式版，glog推荐直接apt安装，不用源码编译（我的电脑上不知道为什么已经装好了）

ceres官方安装教程：

    http://ceres-solver.org/installation.html#linux

**注意** 该仓库里面带的哪个VINS-fusion是修改过的，

原版的vins不支持ceres2.0以上版本和ros-noetic自带的opencv4，但是有人修改了的版本  参考issue

https://github.com/HKUST-Aerial-Robotics/VINS-Fusion/issues/167

原版vins只能用ceres1.14和opencv3，但是ceres1.14又不支持高版本的eigen，需要给eigen降级，还是很麻烦的。应该也有解决办法

