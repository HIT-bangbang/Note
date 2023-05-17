# diatone 

rosrun mavros mavsys rate --all 200   都有用

rosrun mavros mavsys rate --extra1 200 只对data有用 这和官网说的不一样，并不能改变imu_raw的rate

rosrun mavros mavsys rate --raw-sensors 200 只更改raw sensors stream的rate 这个符合预期，只有imu date_raw被改为了200hz

rostopic hz /mavros/imu/data
rostopic hz /mavros/imu/data_raw
rostopic echo /mavros/imu/data



# matekH743

rosrun mavros mavsys rate --all 200 imu发布频率只能跑到130HZ左右
matek似乎带宽不够，--all 200全部数据一起提高速率跑不到200
必须降低其他数据的刷新率，之后把IMU开高才可以开到200，不能全部开到200


# 注意

注意！！ mission planner和QGC每次连接到FC都会发送stream rate信息，或者是使飞控重启？使得刷新率变为初始值，这个可以解决，具体查看官方文档

rosrun mavros mavsys rate --help

可能更好的办法是使用 rosrun mavros mavsys rate --stream-id id "rate"  
研究一下


# mavros中IMU topic信息发布

关于IMU信息的发布，mavros中发布两种imu信息，分别为

**/imu/data

**/imu/data_raw

**注意： ** 这两种消息并不是说**/imu/data是经过飞控过滤之后的数据，而data_raw是原始数据。其区别在于**/imu/data根据飞机姿态转换了坐标系，并且加入了航向信息带来的orientation信息

mavros 中接受的mavlink消息并发布imu话题的代码如下：

https://github.com/mavlink/mavros/blob/d7a6f0c3eacc7cb7ade858ececee523a4945ecb5/mavros/src/plugins/imu.cpp#L132

```c++
  Subscriptions get_subscriptions() override
  {
    return {
      make_handler(&IMUPlugin::handle_attitude),
      make_handler(&IMUPlugin::handle_attitude_quaternion),
      make_handler(&IMUPlugin::handle_highres_imu),
      make_handler(&IMUPlugin::handle_raw_imu),
      make_handler(&IMUPlugin::handle_scaled_imu),
      make_handler(&IMUPlugin::handle_scaled_pressure),
    };
  }
```

px4飞控和ardupilot飞控发来的消息不同，所以mavros通过受到的消息的组合进行对应的回调：

具体查看0.4.0版本的更新日志

plugins: imu_pub: Add RAW_IMU, SCALED_IMU and SCALED_PRESSURE handlers 

Fix #13. Refactor message processing. 

Combination of used messages: 

On APM: ATTITUDE + RAW_IMU + SCALED_PRESSURE 

On PX4: ATTITUDE + HIGHRES_IMU 

On other: ATTITUDE + (RAW_IMU|SCALED_IMU + SCALED_PRESSURE)|HIGHRES_IMU 

Published topics: 

* ~imu/data - ATTITUDE + accel data from *_IMU 

* ~imu/data_raw - HIGHRES_IMU or SCALED_IMU or RAW_IMU in that order 

* ~imu/mag - magnetometer (same source as data_raw) 

* ~imu/temperature - HIGHRES_IMU or SCALED_PRESSURE 

* ~imu/atm_pressure - same as temperature

例如，在ardupilot中：

PC每次接收到RAW_IMU的mavlink信息之后，进入回调直接发布~imu/data_raw，同时会记录信息，等别的publisher用

接收到ATTITUDE信息之后，结合前面存储的RAW_IMU信息，发布~imu/data_raw

所以 官网的说法是错误的： 

mavlink

rosrun mavros mavsys rate --extra1 200 只会改变extra1数据组中数据的rate，ATTITUDE在这其中，这会让~imu/data 发布的频率改变，但是raw sensors数据组中的RAW_IMU属于并不受改变。这样只会改变~imu/data发布的频率，但是**注意！！**，~imu/data中的imu数据还是来自于RAW_IMU的！虽然~imu/data发布地快了，但是imu数据刷新依然是很慢的。

同理：rosrun mavros mavsys rate --raw-sensors 200 只会改变~imu/data_raw的频率

rosrun mavros mavsys rate --all 200   所有消息全部提高为200Hz，可能出现带宽不够的情况

只有同时提高ATTITUDE和RAW_IMU的频率才可以
rosrun mavros mavsys rate --extra1 200
rosrun mavros mavsys rate --raw-sensors 200



# 比较方便的只提高ATTITUDE和RAW_IMU速率的方法

ardupilot在启动的时候会搜索SD卡根目录下的文件，并进行相应的操作

在SD卡根目录下新建一个文件，名为：

**message-intervals-chan0.txt**

如果是使用serial 1 的mavilink协议，那么就改成chan1，serial x的情况以此类推

文件内容例如：

    30 50
    28 100
    29 200

第一个数字为messiage的ID，详见 https://mavlink.io/en/messages/common.html

第二个数字为时间间隔，单位为ms

This sample file content will stream ATTITUDE (ID=30) at 20Hz and SCALED_PRESSURE (ID=29) at 5Hz. Message ID 28 is RAW_PRESSURE which ArduPilot does not send - this line will be ignored.

注意格式一定要严格，中间是一个空格

用到的：

RAW_IMU  (#27)
ATTITUDE (#30)
SCALED_PRESSURE ( #29 )

最终文件内容：

    27 5
    29 5
    30 5
