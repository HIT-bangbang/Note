# diatone 

rosrun mavros mavsys rate --all 200   都有用

rosrun mavros mavsys rate --extra1 200 只对data有用 这和官网说的不一样，并不能改变imu_raw的rate

rosrun mavros mavsys rate --raw-sensors 200 只更改raw sensors stream的rate 这个符合预期，只有imu date_raw被改为了200hz

rostopic hz /mavros/imu/data
rostopic hz /mavros/imu/data_raw
rostopic echo /mavros/imu/data



# matekH743

rosrun mavros mavsys rate --all 200 都有用，但是只能跑到130HZ左右

rosrun mavros mavsys rate --extra1 200 只对data有用 这和官网说的不一样，并不能改变imu_raw的rate

rosrun mavros mavsys rate --raw-sensors 200 只更改raw sensors stream的rate 这个符合预期，只有imu date_raw被改为了200hz

matek似乎带宽不够，--all 200全部数据一起提高速率跑不到200
必须降低其他数据的刷新率，之后把IMU开高才可以开到200，不能全部开到200


# 注意

注意！！ QGC每次连接到FC都会发送stream rate信息，或者是使飞控重启？使得刷新率变为初始值

rosrun mavros mavsys rate --help

可能更好的办法是使用 rosrun mavros mavsys rate --stream-id id "rate"  
研究一下


mavros 用

