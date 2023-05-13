**PX4是pixhawk的原生固件，专门为pixhawk开发的**

APM（Ardupilot Mega）也是硬件

**Ardupilot是APM的固件，所以称ArduPilot固件也叫APM**

后来APM硬件性能不太够，所以APM固件也就针对兼容了Pixhawh硬件平台

所以在pixhawk硬件平台上可以运行PX4固件（原生固件），也可以运行APM固件

APM固件程序比较混乱，零散。维护者多，代码风格不太统一，而且是单片机这种调用程序，不好入门。但是成熟稳定，支持硬件多。
PX4固件，在nuttx嵌入式实时操作系统上运行。采用多任务，模块化设计。相对来说方便入门，代码风格比较统一。但是支持硬件少，相比APM固件不太稳定。