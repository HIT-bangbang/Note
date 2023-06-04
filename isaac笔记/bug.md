    gym.set_camera_location(camera_handle, env, gymapi.Vec3(0.61,0.1,1), gymapi.Vec3(0.6,0.1,0))
x坐标不能相同，否则图像是黑的。原因未知，万向锁？？

教程里面第二种方法代码不对，其中评议平移应该是gymapi.Vec3()

    transform = gymapi.Transform()
    #应该是
    transform.p = gymapi.Vec3(0.5,0,1)
    transform.r = gymapi.Quat.from_axis_angle(gymapi.Vec3(0,1,0), np.radians(45.0))
    gym.set_camera_transform(camera_handle, env, transform)