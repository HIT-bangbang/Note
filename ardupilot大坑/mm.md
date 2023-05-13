# 环境配置
按照官网来
git clone --recurse-submodules https://github.com/your-github-userid/ardupilot
cd ardupilot

执行自动安装脚本

./Tools/environment_install/install-prereqs-ubuntu.sh -y

更新环境配置，这里建议直接重启最保险
. ~/.profile


切换分支

git checkout ***

当然也可以从当前分支新建一个自己的分支并且切换过去

git checkout -b mytest-branch

更新子模块，注意每次切换了分支后，都要更新！！！！

git submodule update --init --recursive

配置板子

./waf configure --board CubeBlack

编译多旋翼固件
./waf copter


