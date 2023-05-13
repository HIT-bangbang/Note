设置飞控方向：
高级参数： AHRS_ORIENTATION

设置机架类型：

frame class

frame type


设置接收机通信：


Set SERIAL4_PROTOCOL = 23

Set RSSI_TYPE = 3


遥控器校准

关闭摇杆解锁和上锁，设置单独的解锁通道

ARMING_RUDDER = 0， 关闭打杆解锁
RC5_OPTION = 153 或者154 ？？  设置第五通道为解锁

设置电调协议Dshot输出,设置servo输出通道，对照可选硬件>>电机测试，更改电机输出序号

设置飞行模式通道 
fltmode_ch

设置飞行模式

关闭不需要的解锁检查
ARMING_CHECK = 0  关闭所有检查

在标准参数中也可以快速设置

不插电池，连接 地面站解锁，推推杆看看电机输出是否正确
 