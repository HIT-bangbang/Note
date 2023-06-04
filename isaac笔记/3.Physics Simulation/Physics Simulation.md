# Creating Actors

一个actor是GymAsset的一个实例。函数`create_actor`将一个actor添加到环境中，并返回一个稍后可用于与该actor交互的actor句柄。出于性能原因，最好在actor创建过程中保存句柄，而不是每次在模拟运行时都查找句柄。许多附带的示例都是这样做的：

Actor句柄特定于创建Actor的环境。对Actor进行操作的API函数需要 对环境的引用和Actor句柄，因此它们通常被缓存在一起。

有相当多的函数对actor进行操作。它们被命名为`get_actor_*`、`set_actor_**`或`apply_ector_*`请使用API reference获取完整列表。


# Aggregates聚合体

Aggregates仅可用于PhysX后端。创建聚合对Flex没有任何影响。

Aggregates是actor的集合。聚合不提供额外的模拟功能，但允许您告诉PhysX一组actor将聚集在一起，这反过来又允许PhysX优化其空间数据操作。

创建Aggregates不是必须的，但这样做可以适度提高性能。要将多个actor放入一个aggregate中，您应该将对`create_actor`的调用包含在对`begin_aggregate`和`end_aggregate`的调用之间，如下所示：

    gym.begin_aggregate(env, max_bodies, max_shapes, True)
    gym.create_actor(env, ...)
    gym.create_actor(env, ...)
    gym.create_actor(env, ...)
    ...
    gym.end_aggregate(env)



只有来自同一env的actor才能包含在一个Aggregate中。创建Aggregate时，有必要指定刚体和形状的最大数量，该数量应为将放置在聚合中的所有actor中的刚体和形状总数。该信息可以从用于创建演员的资产（`get_asset_rigid_body_count`和`get_asset_rigid_shape_count`）中获得。 

For an example of using aggregates, take a look at `python/rlgpu/tasks/franka.py`

# actor组件

 每个actor都有一组刚体(rigid bodies)、关节(joints)和自由度(DOFs)。你可以这样得到数量：

    num_bodies = gym.get_actor_rigid_body_count(env, actor_handle)
    num_joints = gym.get_actor_joint_count(env, actor_handle)
    num_dofs = gym.get_actor_dof_count(env, actor_handle)

一旦创建了actor，那么此时就不能添加或删除actor组件了。

## Rigid Bodies

每个刚体由一个或多个刚性形状组成。可以自定义每个actor的刚体和形状属性，如`body_physics_props.py`示例所示。

## Joints

Gym importers保留URDF和MJCF模型中定义的关节。固定式、旋转式和棱柱式joint都经过了完备的测试并且完全支持(Fixed, revolute, and prismatic joints are well-tested and fully supported)。使用这些关节类型，可以创建各种各样的模型。计划为球形和平面接头提供额外的支持。 

## Degrees-of-Freedom

每个自由度都可以被独立地驱动。您可以将控件应用于单个DOF，也可以使用数组一次处理所有actor的DOF，如下一节所述。 

## Controlling Actors

控制actor是使用自由度来完成的。对于每个自由度，可以设置驱动模式drive mode、极限limits、刚度stiffness、阻尼damping和目标targets。您可以为每个actor设置这些值，并覆盖从asset加载的默认设置。 

# 缩放Scaling Actors


actor可以在运行时缩放其大小。缩放actor将更改其碰撞几何体collision geometry、质量属性mass properties、关节位置joint positions和棱柱关节限制prismatic joint limits。有关示例用法，请参见`examples/actor_scaling.py`。 

* 注意

    在使用GPU pipeline时，actor缩放具有已知的限制。刚体的质心不会更新，这只会影响质心不在实体原点的实体。此外，在与缩放相同的模拟步骤中重置变换和速度或施加力可能会产生不正确的结果。我们预计这些限制将在未来得到解决。 

缩放actor后，可能需要手动调整其某些属性以获得所需的结果。例如，其自由度属性DOF（刚度、阻尼、最大力、电枢等stiffness, damping, max force, armature）、驱动力、tendon属性等可能需要调整。

# DOF Properties and Drive Modes

自由度属性矩阵可以通过如下的方式获取和设置：
* 对于asset:
    get_asset_dof_properties
* 对于某个actor：
    get_actor_dof_properties/set_actor_dof_properties

`get`方法返回structured Numpy arrays,该数组里面包含了该actor/asset的所有DOF属性，如下表所示：
<table class="docutils align-default">
<thead>
<tr class="row-odd"><th class="head"><p>Name</p></th>
<th class="head"><p>Data type</p></th>
<th class="head"><p>Description</p></th>
</tr>
</thead>
<tbody>
<tr class="row-even"><td><p>hasLimits</p></td>
<td><p>bool</p></td>
<td><p>Whether the DOF has limits or has unlimited motion.</p></td>
</tr>
<tr class="row-odd"><td><p>lower</p></td>
<td><p>float32</p></td>
<td><p>Lower limit.</p></td>
</tr>
<tr class="row-even"><td><p>upper</p></td>
<td><p>float32</p></td>
<td><p>Upper limit.</p></td>
</tr>
<tr class="row-odd"><td><p>driveMode</p></td>
<td><p>gymapi.DofDriveMode</p></td>
<td><p>DOF drive mode, see below.</p></td>
</tr>
<tr class="row-even"><td><p>stiffness</p></td>
<td><p>float32</p></td>
<td><p>Drive stiffness.</p></td>
</tr>
<tr class="row-odd"><td><p>damping</p></td>
<td><p>float32</p></td>
<td><p>Drive damping.</p></td>
</tr>
<tr class="row-even"><td><p>velocity</p></td>
<td><p>float32</p></td>
<td><p>Maximum velocity.</p></td>
</tr>
<tr class="row-odd"><td><p>effort</p></td>
<td><p>float32</p></td>
<td><p>Maximum effort (force or torque).</p></td>
</tr>
<tr class="row-even"><td><p>friction</p></td>
<td><p>float32</p></td>
<td><p>DOF friction.</p></td>
</tr>
<tr class="row-odd"><td><p>armature</p></td>
<td><p>float32</p></td>
<td><p>DOF armature.</p></td>
</tr>
</tbody>
</table>

实例：franka机械臂的代码，设置机械臂关节的性质
```python
# get joint limits and ranges for Franka
franka_dof_props = gym.get_actor_dof_properties(envs[0], franka_handles[0])
franka_lower_limits = franka_dof_props['lower']
franka_upper_limits = franka_dof_props['upper']
franka_ranges = franka_upper_limits - franka_lower_limits
franka_mids = 0.5 * (franka_upper_limits + franka_lower_limits)
franka_num_dofs = len(franka_dof_props)

# override default stiffness and damping values
franka_dof_props['stiffness'].fill(1000.0)
franka_dof_props['damping'].fill(1000.0)

# Give a desired pose for first 2 robot joints to improve stability
franka_dof_props["driveMode"][0:2] = gymapi.DOF_MODE_POS

franka_dof_props["driveMode"][7:] = gymapi.DOF_MODE_POS
franka_dof_props['stiffness'][7:] = 1e10
franka_dof_props['damping'][7:] = 1.0
```


## DOF_MODE_NONE

将driveMode设置为`DOF_MODE_NONE`将允许关节在其运动范围内自由移动：
```python
    props = gym.get_actor_dof_properties(env, actor_handle)
    props["driveMode"].fill(gymapi.DOF_MODE_NONE)
    props["stiffness"].fill(0.0)
    props["damping"].fill(0.0)
    gym.set_actor_dof_properties(env, actor_handle, props)
```
This is used in the `projectiles.py` example, where the ant actors are essentially ragdolls.

## DOF_MODE_EFPORT力控制
`DOF_MODE_EFPORT`允许您使用`apply_eactor_DOF_efforts`将力应用于DOF。如果自由度是linear，则作用力是以牛顿为单位的力。如果自由度是angular，则作用力是以Nm为单位的扭矩。DOF属性只需设置一次，但efforts必须在每一帧中应用。在每一帧中多次施加的作用力是累积的，但在下一帧开始时会重置为零：
```python
# configure the joints for effort control mode (once)
props = gym.get_actor_dof_properties(env, actor_handle)
props["driveMode"].fill(gymapi.DOF_MODE_EFFORT)
props["stiffness"].fill(0.0)
props["damping"].fill(0.0)
gym.set_actor_dof_properties(env, actor_handle, props)

# apply efforts (every frame)
efforts = np.full(num_dofs, 100.0).astype(np.float32)
gym.apply_actor_dof_efforts(env, actor_handle, efforts)
```

## 位置控制DOF_MODE_POS    速度控制DOF_MODE_VEL
`DOF_MODE_POS`和`DOF_MODE_VEL`采用PD控制器，该控制器可以使用刚度stiffness和阻尼damping参数进行调整。控制器将施加与posError * stiffness + velError * damping成比例的自由度力。 

`DOF_MODE_POS`用于位置控制，通常刚度和阻尼都设置为非零值：
```python
props = gym.get_actor_dof_properties(env, actor_handle)
props["driveMode"].fill(gymapi.DOF_MODE_POS)
props["stiffness"].fill(1000.0)
props["damping"].fill(200.0)
gym.set_actor_dof_properties(env, actor_handle, props)
```

 可以使用`set_actor_dof_position_targets`设置位置目标：
```python
targets = np.zeros(num_dofs).astype('f')
gym.set_actor_dof_position_targets(env, actor_handle, targets)
```

如果自由度是Linear的，则目标以米为单位。如果DOF是角angular的，则目标是以弧度为单位的。

这里有一个更有趣的例子，它将为每个自由度在limits内随机设置位置：
```python
dof_props = gym.get_actor_dof_properties(envs, actor_handles)
lower_limits = dof_props['lower']
upper_limits = dof_props['upper']
ranges = upper_limits - lower_limits

pos_targets = lower_limits + ranges * np.random.random(num_dofs).astype('f')
gym.set_actor_dof_position_targets(env, actor_handle, pos_targets)
```
`DOF_MODE_VEL`用于速度控制。刚度参数应设置为零。PD控制器施加的扭矩将与阻尼参数成比例：
```python
props = gym.get_actor_dof_properties(env, actor_handle)
props["driveMode"].fill(gymapi.DOF_MODE_VEL)
props["stiffness"].fill(0.0)
props["damping"].fill(600.0)
gym.set_actor_dof_properties(env, actor_handle, props)
```


可以使用`set_actor_dof_velocity_targets`设置速度目标。如果自由度是linaer，则目标以米每秒为单位。如果DOF是angular，则目标以弧度每秒为单位： 
```python
vel_targets = np.random.uniform(-math.pi, math.pi, num_dofs).astype('f')
gym.set_actor_dof_velocity_targets(env, actor_handle, vel_targets)
```

* 与efforts不同，位置和速度目标不需要每帧都设置，只需要在更改目标时设置。

## 注意

    当使用 `apply_eactor_dof_efforts`、`set_actor_dof_position_targets`和`set_actor_dof_velocity_targets`时，必须始终传递长度为num_dofs的数组。如果该DOF被配置为使用该驱动模式，则来自阵列的控制值将仅应用于其对应的DOF。例如，如果调用set_actor_dof_position_targets，但其中一个自由度被配置为使用dof_MODE_EFPORT，则该自由度将保持在努力模式，其位置目标值将被忽略。 

除了使用actor DOF arrays外，每个DOF都可以独立控制，如`DOF_controls.py`示例所示。使用单独的DOF更灵活，但由于多个Python API调用的开销，效率可能较低。

## Tensor Control API

新的张量API提供了控制(applying controls,详细见`Tensor API/control tensor`)的替代方法。用于设置DOF属性的API保持不变，但您可以使用CPU或GPU张量应用力或设置PD目标。这使得完全在GPU上运行模拟成为可能，而无需在主机和设备之间复制数据。 

# Physics State

Gym提供了一个API，用于获取和设置物理状态为结构化Numpy数组。 

## Rigid Body States刚体状态

刚体状态包括位置（Vec3）、方向（Quat）、线速度（Vec3）。这允许您在最大坐标（maximal coordinates 其实就是笛卡尔坐标）下处理模拟状态。

可以获取actor、环境或整个仿真的刚体状态阵列：
```python
body_states = gym.get_actor_rigid_body_states(env, actor_handle, gymapi.STATE_ALL)
body_states = gym.get_env_rigid_body_states(env, gymapi.STATE_ALL)
body_states = gym.get_sim_rigid_body_states(sim, gymapi.STATE_ALL)
```
这些方法返回结构化的numpy数组。最后一个参数是一个位字段(bit field)，用于指定应返回的状态类型。STATE_POS表示应计算位置，STATE_VEL表示应计算速度，STATE_ALL表示应计算两者。返回的数组的结构总是相同的，但只有在设置了相应的标志时，才会计算位置和速度值。在内部，Gym维护一个状态缓存缓冲区，用于存储这些值。Numpy数组只是缓冲区切片的包装器。根据底层物理引擎的不同，获取和设置状态可能需要进行非平凡的计算，并且可以使用位标志来避免不必要的操作。



状态数组切片可以这样访问： 

```python
body_states["pose"]             # all poses (position and orientation)
body_states["pose"]["p"])           # all positions (Vec3: x, y, z)
body_states["pose"]["r"])           # all orientations (Quat: x, y, z, w)
body_states["vel"]              # all velocities (linear and angular)
body_states["vel"]["linear"]    # all linear velocities (Vec3: x, y, z)
body_states["vel"]["angular"]   # all angular velocities (Vec3: x, y, z)
```
```python
gym.set_actor_rigid_body_states(env, actor_handle, body_states, gymapi.STATE_ALL)
gym.set_env_rigid_body_states(env, body_states, gymapi.STATE_ALL)
gym.set_sim_rigid_body_states(sim, body_states, gymapi.STATE_ALL)
```
在`projectiles.py`示例中演示了保存和恢复状态。一开始，我们为整个仿真保存刚体状态的副本。当用户按下R时，这些状态将恢复。

要确定状态阵列中特定刚体的偏移，请使用`find_actor_rigid_body_index`方法，如下所示：
```python
i1 = gym.find_actor_rigid_body_index(env, actor_handle, "body_name", gymapi.DOMAIN_ACTOR)
i2 = gym.find_actor_rigid_body_index(env, actor_handle, "body_name", gymapi.DOMAIN_ENV)
i3 = gym.find_actor_rigid_body_index(env, actor_handle, "body_name", gymapi.DOMAIN_SIM)
```

最后一个参数指定索引域：

* 使用域domain_ACTOR获取get_ACTOR_rigid_body_states返回的索引，存入state buffer


* 使用域domain_ENV获取get_ENV_rigid_body_states返回的的索引，存入state buffer


* 使用域domain_SIM获取get_SIM_rigid_body_states回的的索引，存入state buffer

## DOF States


您也可以使用reduced coordinates： 
```python
dof_states = gym.get_actor_dof_states(env, actor_handle, gymapi.STATE_ALL)
gym.set_actor_dof_states(env, actor_handle, dof_states, gymapi.STATE_ALL)
```
自由度状态阵列包括位置和速度（single floats）。对于线性自由度，位置以米为单位，速度以米每秒为单位。对于角自由度，位置以弧度为单位，速度以弧度每秒为单位。

您可以访问位置和速度切片(slices)，如下所示：
```python
dof_states["pos"]   # all positions
dof_states["vel"]   # all velocities
```

可以使用`find_actor_DOF_index`方法确定特定DOF的偏移。

请注意，DOF状态不包括根刚体的位姿或速度，因此它们不会完全捕捉演员状态。因此，我们没有提供获得和设置整个环境或仿真的自由度状态的方法。这一限制应在未来加以解决。

`joint_monkey.py`示例说明了如何使用DOF states。

## Physics State Tensor API

新的张量API允许使用CPU或GPU张量来获取和设置状态。这使得完全在GPU上运行模拟成为可能，而无需在主机和设备之间复制数据。