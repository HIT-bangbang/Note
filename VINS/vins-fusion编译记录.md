# ubuntu20 + XTDrone vins-fusion

# vins-fusion 第三方编译版本

注意这个版本是XTDrone给的，应该是做好了ubuntu20适配的。

# 安装ceres1.4.0

    克隆，切换版本
    mkdir build
    cd build
    cmake ..
    make
    make install

可能会遇到glog没装好的问题，直接二进制安装就行：

    sudo apt-get install libgoogle-glog-dev


