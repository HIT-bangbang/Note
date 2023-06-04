# Force Sensors

## Rigid Body Force Sensors


力传感器可以连接到刚体上，以测量在用户指定的参考系下所受到的力和扭矩。读数表示parent body所experienced的净力和扭矩，包括外力、接触力和内力（例如，由于关节驱动器）。静止在地平面上的物体的净力为零。


力传感器是在assets上创建的。这样，您只需要定义一次力传感器，就可以在assets实例化的每个actor上创建力传感器。


若要创建力传感器，请指定传感器将附着到的rigid body index以及传感器相对于body原点的相对位姿。多个传感器可以连接到同一个body上： 
```python
body_idx = gym.find_asset_rigid_body_index(asset, "bodyName")

sensor_pose1 = gymapi.Transform(gymapi.Vec3(0.2, 0.0, 0.0))
sensor_pose2 = gymapi.Transform(gymapi.Vec3(-0.2, 0.0, 0.0))

sensor_idx1 = gym.create_asset_force_sensor(asset, body_idx, sensor_pose1)
sensor_idx2 = gym.create_asset_force_sensor(asset, body_idx, sensor_pose2)
```

您还可以为每个传感器传递其他属性，以确定如何计算力：

```python
sensor_props = gymapi.ForceSensorProperties()
sensor_props.enable_forward_dynamics_forces = True
sensor_props.enable_constraint_solver_forces = True
sensor_props.use_world_frame = False

sensor_idx = gym.create_asset_force_sensor(asset, body_idx, sensor_pose, sensor_props)
```
* See `isaacgym.gymapi.ForceSensorProperties` for more details.

创建actor后，您可以获得连接的力传感器的数量，并使用它们的索引访问各个力传感器：

```python
actor_handle = gym.create_actor(env, asset, ...)

num_sensors = gym.get_actor_force_sensor_count(env, actor_handle)
for i in range(num_sensors):
    sensor = gym.get_actor_force_sensor(env, actor_handle, i)
```

在仿真过程中，您可以查询最新的传感器读数，如下所示：
```python
sensor_data = sensor.get_forces()
print(sensor_data.force)   # force as Vec3
print(sensor_data.torque)  # torque as Vec3
```

连接到同一body上的两个传感器将报告相同的力，但如果它们的相对姿态不同，则扭矩通常会不同。力是在body的质心处测量的，但扭矩是在传感器的局部参考系处测量的。


可以通过调用`gym.get_sim_force_sensor_count(sim)`来获仿真中力传感器的总数。 

### Tensor API

函数`acquire_force_sensor_sensor`返回一个Gym张量描述符Gym tensor descriptor，该描述符可以封装为PyTorch张量，如`Tensor API文档`中所述：

```python
_fsdata = gym.acquire_force_sensor_tensor(sim)
fsdata = gymtorch.wrap_tensor(_fsdata)
```
fsdata这个张量的形状是(num_force_sensors，6)，数据类型是float32。对于每个传感器，前三个浮点数是力，后三个浮点数是扭矩。

在每个仿真步骤之后，您可以通过调用获取最新的传感器读数：

    gym.refresh_force_sensor_tensor(sim)


### Limitations

Force sensors力传感器目前仅可用于PhysX后端。

## Joint Force Sensors

要在关节型actor的每个自由度上启用力的读取，请在设置envs时调用`enable_actor_dof_force_sensors`方法：
```python
actor = gym.create_actor(env, ...)
gym.enable_actor_dof_force_sensors(env, actor)
```

由于性能原因，默认情况下不会启用这些传感器。尽管性能影响通常很小，但最好只在需要时启用传感器。在仿真过程中，可以使用`get_actor_dof_forces`函数检索力： 

    forces = gym.get_actor_dof_forces(env, actor_handle)

这将返回一个作用在该actor的自由度上的合力的numpy数组。

* 注意，如果之前没有对actor调用`enable_actor_dof_force_sensors`，则返回的力将始终为零。

### Tensor API



函数acquire_dof_force_tensor返回一个Gym张量描述符Gym tensor descriptor，该描述符可以封装为PyTorch张量，如`Tensor API文档`中所述： 
```python
_forces = gym.acquire_dof_force_tensor(sim)
forces = gymtorch.wrap_tensor(_forces)
```

这是与仿真中的每个DOF相对应的float32值的一维张量。


在每个仿真的step后可以通过调用下面的函数获取最新的传感器读数： 

    gym.refresh_dof_force_tensor(sim)


### Limitations

DOF force sensors 目前仅可用于PhysX后端。

