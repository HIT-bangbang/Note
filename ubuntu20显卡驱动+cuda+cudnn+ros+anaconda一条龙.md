# Ubuntu20安装驱动

## 1、nvidia官网下载驱动
## 2、卸载原有驱动

    sudo apt-get remove --purge nvidia*

## 3、禁用nouveau(nouveau是通用的驱动程序)
可以先通过指令lsmod | grep nouveau查看nouveau驱动的启用情况，如果有输出表示nouveau驱动正在工作，如果没有内容输出则表示已经禁用了nouveau

    sudo gedit /etc/modprobe.d/blacklist.conf   # 或者(blacklist-nouveau.conf)

在打开的blacklist.conf末尾添加如下，保存文本关闭

    blacklist nouveau
    options nouveau modeset=0

在终端输入如下更新，更新结束后重启电脑（必须）

    sudo update-initramfs -u

    # 然后电脑重启系统
    sudo reboot

重启后在终端输入如下，没有任何输出表示屏蔽成功

    lsmod | grep nouveau

## 4、安装lightdm (可选，也可以不装， 没什么区别不用装)

    sudo apt-get install lightdm

## 5、安装驱动

进入其他界面

    Ctrl + Alt + F2~F6

禁用X-window服务

如果是使用默认的gdm3管理器

    sudo /etc/init.d/gdm3 stop

如果是使用第4步下载的

    sudo /etc/init.d/lightdm stop或者（sudo service lightdm stop）

cd到存放驱动的目录

    sudo chmod 777 NVIDIA-Linux-x86_64-515.65.01.run   #给你下载的驱动赋予可执行权限，才可以安装
    sudo ./NVIDIA-Linux-x86_64-515.65.01.run –no-opengl-files   #安装

–no-opengl-files 只安装驱动文件，不安装OpenGL文件。笔记本不加有可能出现循环登录，也就是loop login。但是我这个电脑没问题，加不加都行

    1.The distribution-provided pre-install script failed! Are you sure you want to continue?
    选择continue installation

    2.Would you like to register the kernel module souces with DKMS? This will allow DKMS to automatically build a new module, if you install a different kernel later?  
    选择 No 继续。

    3.问题没记住，
    选项是：install without signing

    4.问题大概是：Nvidia's 32-bit compatibility libraries? 
    选择 No 继续。

    5.Would you like to run the nvidia-xconfigutility to automatically update your x configuration so that the NVIDIA x driver will be used when you restart x? Any pre-existing x confile will be backed up.  
    ​​​​​​​选择 Yes  继续

## 6、

安装结束后输入sudo service lightdm start 或者 sudo service gdm3 start 重启x-window服务，即可自动进入登陆界面，不行的话，输入sudo reboot重启，再看看。（重启后不行，尝试在bios中去掉安全启动设置，改成 secure boot：disable）

终端输入nvidia-smi 检查是否装好

# 安装cuda

官网 https://developer.nvidia.com/cuda-toolkit-archive

找到和显卡驱动对应版本的cuda

一定选择runfile（local）进行下载

安装过程一定取消安装Drivier，否则会顶掉前面安装的显卡驱动

bashrc里面添加：

    export CUDA_HOME=/usr/local/cuda
    export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${CUDA_HOME}/lib64
    export PATH=${CUDA_HOME}/bin:${PATH}

终端输入 nvcc -V 查看安装情况

# 安装 cudnn

https://developer.nvidia.com/rdp/cudnn-download

下载教程官网：https://docs.nvidia.com/deeplearning/cudnn/install-guide/index.html

选择 Tar格式的下载包进行安装最简单

# 安装ros

# 安装anaconda

注意安装顺序，先装ros再装anaconda

官网下载安装包

执行.sh文件

最后一部初始化初始化Anaconda选择yes，他会在bashrc里面添加好配置

这样打开终端后自动进入base环境

如果想打开终端时自动激活某个环境，在bashrc里面添加conda activate gym

# 在虚拟环境中安装ros需要的包
一般有rospy等等，逢山开路，遇水架桥即可，编译运行随便一个demo，遇到报错，看报错信息直接sudo apt-get install就行