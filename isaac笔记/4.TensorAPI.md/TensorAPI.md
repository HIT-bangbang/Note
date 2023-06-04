# Tensor API

Gym tensor API使用GPU兼容的数据表示与仿真系统交互。它允许直接在GPU上访问物理状态，而无需从主机hosts来回复制数据。它还支持使用张量来apply controls，这使得可以设置完全在GPU上运行的实验。

张量本质上是values的多维数组。Gym张量API使用“全局”张量，即hold仿真中所有actor的values的张量。例如，使用单个张量来hold仿真中所有刚体的状态。类似地，可以使用单个张量将控制应用于仿真中的所有actors。这种方法具有重要的性能优势。您可以获取所有actor的最新状态，或者使用单个函数调用将控制应用于所有actor。这就不需要在多个环境中循环，也不需要发布每个actor的操作，这些操作会增加相当大的开销，尤其是在Python等语言中。使用全局缓冲区也有助于数据并行，特别是在GPU上，多个线程可以轻松地并行处理不同的缓冲区切片。


Tensor是用于存储GPU兼容数据的公认的数据结构。PyTorch和TensorFlow等流行框架将支持张量作为核心功能。Gym张量API独立于其他框架，但它被设计为易于与它们兼容。Gym张量API使用简单的张量描述符(simple tensor desciptors)，指定张量的设备、内存地址、数据类型和形状。没有专门的API来操作Gym张量中的数据。相反，可以使用互操作实用程序（interop utilities）将张量描述符转换为更可用的张量类型，如PyTorch张量。一旦Gym张量被“包装（wrap）”在PyTorch张量中，您就可以使用所有现有的PyTorch实用程序来处理张量的内容。

tensor API的一个重要方面是它可以与CPU和GPU张量一起工作。这使得在CPU和GPU上以最小的工作量运行相同的实验成为可能。

# Simulation Setup

张量API旨在在创建所有环境和actor之后，在仿真过程中使用。要设置仿真，可以使用“Creating a Simulation”章节中描述的“典型classic”API。要使用张量API，需要注意一些额外的细节。

张量API目前仅适用于PhysX，因此在创建仿真时必须使用参数`SIM_PhysX`。

若要使用GPU张量(注意这里是说的GPU张量，还没说CPU张量)，必须在用于创建仿真的`SimParams`中将`use_GPU_pipeline`标志设置为`True`。此外，您应该将PhysX配置为使用GPU：

```python
    sim_params = gymapi.SimParams()
    ...
    sim_params.use_gpu_pipeline = True
    sim_params.physx.use_gpu = True

    sim = gym.create_sim(compute_device_id, graphics_device_id, gymapi.SIM_PHYSX, sim_params)
```

如果`use_gpu_pipeline`为`False`，则Gym返回的张量将驻留在CPU上。


使用CPU pipeline，PhysX仿真可以在CPU或GPU上运行，具体由`PhysX.use_GPU`参数指定。使用GPU pipeline，PhysX仿真必须在GPU上运行。因此，当设置了`use_gpu_pipeline为True`时，将忽略`physx.use_gpu`参数，并使用GPU PhysX进行仿真。

最后，在所有环境都完全设置好之后，您必须调用prepare_sim来初始化张量API使用的内部数据结构： 
```python
gym.prepare_sim(sim)
```
# Physics State

调用`prepare_sim`后，您可以获得物理状态张量（physics state tensors）。这些张量以易于使用的格式表示仿真状态的缓存cache。需要注意的是，这些张量持有仿真状态的副本。它们与底层物理引擎使用的数据结构不同。每个物理引擎都使用自己的数据表示，Gym所暴露给用户的张量是抽象底层细节的通用表示。


尽管张量是物理状态的复制，但复制的过程非常快。使用GPU管道时，数据永远不会复制到host。设备到设备的拷贝速度很快，因此开销最小。此外，用户可以控制何时刷新张量，因此永远不会进行隐式复制。

## Actor Root State Tensor 根状态张量
Gym actor可以由一个或多个刚体组成。所有actor都有一个根体(root body)。 root state tensor保持仿真中所有actor 的 root body的状态。每个root body的state使用13个与`GymRigidBodyState`的layout相同的floats表示：3个float表示位置，4个float表示四元数，3个float用于线速度，3个float表示角速度。

要获取根状态张量：
```python
    _root_tensor = gym.acquire_actor_root_state_tensor(sim)
```
这返回一个通用张量描述符，它本身并不是很有用，这就是为什么我们在名称前面加了一个下划线。

* 为了访问张量的内容，可以使用提供的`gymthorch interop`模块将其封装在PyTorch张量对象中：
```python
root_tensor = gymtorch.wrap_tensor(_root_tensor)
```
如此一来，就可以在PyTorch中使用这个张量了，以及PyTorch提供的所有强大的实用程序——我们不需要重新发明所有的轮子。未来，我们可能会添加其他互操作模块，这将允许使用TensorFlow等其他框架访问Gym状态。

这个张量的形状是（num_actors，13），数据类型是float32。您可以按原样使用此张量，也可以使用标准pytorch语法创建更方便的views或切片：
例如：使用切片
```python
root_positions = root_tensor[:, 0:3]
root_orientations = root_tensor[:, 3:7]
root_linvels = root_tensor[:, 7:10]
root_angvels = root_tensor[:, 10:13]
```
您只需要在仿真开始之前获取张量并创建views一次。


要使用最新状态更新这些张量的内容，请调用：

    gym.refresh_actor_root_state_tensor(sim)

把所有这些放在一起，代码可能看起来像这样：
```python
# ...create sim, envs, and actors here...

gym.prepare_sim()

# acquire root state tensor descriptor
_root_tensor = gym.acquire_actor_root_state_tensor(sim)

# wrap it in a PyTorch Tensor and create convenient views
root_tensor = gymtorch.wrap_tensor(_root_tensor)
root_positions = root_tensor[:, 0:3]
root_orientations = root_tensor[:, 3:7]
root_linvels = root_tensor[:, 7:10]
root_angvels = root_tensor[:, 10:13]

# main simulation loop
while True:
    # step the physics simulation
    gym.simulate(sim)

    # refresh the state tensors
    gym.refresh_actor_root_state_tensor(sim)

    # ...use the latest state tensors here...

```

可以使用root_state_tensor通过为actor的root body设置新的位置、方向和速度来传送actor。此功能在重置过程中很有用，但不应每帧都执行，因为这会干扰物理引擎的作用，并可能导致非物理行为。


作为一个人为的例子，假设你想沿着y轴将所有actor提高一个单位。您可以这样修改root的位置：

```python
offsets = torch.tensor([0, 1, 0]).repeat(num_actors)
root_positions += offsets
```

这将在适当的位置修改根状态张量(root_state_tensor)，您可以这样应用更改： 

    gym.set_actor_root_state_tensor(sim, _root_tensor)

请注意，我们在这里使用`_root_tensor`，这是我们在开始时获得的原始Gym tensor descriptor。我们修改了张量的内容，现在我们正在使用它来应用物理引擎的更新。其效果是，所有actor都会向上传送一个单位，无论他们是单体演员还是多体演员。


另一个例子是对actor根进行周期性重置，每100步将其传送到原始位置一次： 
```python

# acquire root state tensor descriptor
_root_tensor = gym.acquire_actor_root_state_tensor(sim)

# wrap it in a PyTorch Tensor
root_tensor = gymtorch.wrap_tensor(_root_tensor)

# save a copy of the original root states
saved_root_tensor = root_tensor.clone()

step = 0

# main simulation loop
while True:
    # step the physics simulation
    gym.simulate(sim)

    step += 1

    if step % 100 == 0:
        gym.set_actor_root_state_tensor(sim, gymtorch.unwrap_tensor(saved_root_tensor))

```

请注意，我们使用torch `clone`方法创建了一个新的torch张量，其中包含原始根状态的副本。Gym方法`set_actor_root_state_tensor`无法直接消化torch张量，因此我们需要使用`gymtorch.unwrap_tensor`将该torch张量转换为Gym tensor descriptor。


要更新actor子集的状态，有一种方法叫做`set_actor_root_state_tensor_indexed`。它需要一个额外的actor索引张量来重置，该张量必须是32位整数(integers)。仿真中的actor总数A可以通过调用gym.`get_sim_aactor_count（sim）`来获得。有效的actor索引范围从0到A-1。根状态张量中特定actor的索引可以通过调用`gym.get_actor_index（env，actor_handle，gymapi.DOMAIN_SIM）`来获得。下面是将`set_actor_root_state_tensor_indexed`与PyTorch索引张量一起使用的示例：

```python
actor_indices = torch.tensor([0, 17, 42], dtype=torch.int32, device="cuda:0")

gym.set_actor_root_state_tensor_indexed(sim, _root_states, gymtorch.unwrap_tensor(actor_indices), 3)
```

请注意，传递给此方法的状态缓冲区与传递给set_actor_root_state_tensor的状态缓冲相同。换句话说，它是包含所有actor状态的整个张量，但状态更新将仅应用于索引张量中列出的actor。这种方法使实践中的生活更轻松，并具有一些性能优势。您不需要构造一个只包含要修改的状态的新状态张量。您可以简单地将更新后的状态写入正确索引处的原始根状态张量，然后在对`set_actor_root_state_tensor_indexed`的调用中指定这些索引。

为了避免与Gym共享PyTorch张量（尤其是索引张量）时出现一些常见错误，请阅读 `Tensor Lifetime` 章节。

## Degrees-of-Freedom

关节型的actor有许多自由度（DOF），其状态可以被查询和更改。每个自由度的状态使用两个32位浮点数表示，即自由度**位置**和自由度**速度**(DOF position and DOF velocity.)。

对于棱柱（平移）自由度，位置以米为单位，速度以米每秒为单位。对于旋转（旋转）自由度，位置以弧度为单位，速度以弧度每秒为单位。

自由度状态张量(DOF state tensor)包含仿真中所有自由度的状态。张量的形状是（num_dofs，2）。通过调用`gym.get_sim_dof_count(sim)`可以获得自由度的总数。自由度状态按顺序排列。张量以actor 0的所有DOFS开始，然后是actor 1的所有DOFs，依此类推。每个actor的DOFs顺序与函数`get_actor_dof_states`和`set_actor_dof _states`相同。actor的DOFs数量可以使用`gym.get_actor_dof_count(env，actor)`获得。张量中特定DOF的全局索引可以通过多种方式获得：

* 调用`gym.get_actor_dof_index(env，actor_handle，i，gymapi.DOMAIN_SIM)`将返回指定acotr的第i个DOF的全局dof索引。


* 调用`gym.find_actor_dof_index(env，actor_handle，dof_name，gymapi.DOMAIN_SIM)`将按名称查找全局DOF索引。

函数`acquire_dof_state_tensor`返回一个Gym tensor descriptor，该描述符可以封装为PyTorch张量，如前所述：

    _dof_states = gym.acquire_dof_state_tensor(sim)
    dof_states = gymtorch.wrap_tensor(_dof_states)

函数refresh_dof_state_tensor将仿真的最新数据更新到张量中：

    gym.refresh_dof_state_tensor(sim)


可以修改DOF状态张量DOF state tensor中的值，并将其应用于仿真。函数`set_dof_state_tensor`应用张量中的所有值。这意味着为仿真中的所有actor设置所有DOF位置和速度： 

    gym.set_dof_state_tensor(sim, _dof_states)

如果您修改了这些值，您可以传递从`acquire_dof_state_tensor`获得的原始张量描述符Gym tensor descriptor。或者，您可以创建自己的张量来保存新值，然后传递descriptor。


函数`set_dof_state_tensor_indexed`将给定张量中的值应用于`actor_index_tensor`中指定的actor。其他actor仍然不受到影响。这在仅重置选定的actor或环境时非常有用。actor索引必须是32位整数，就像从`get_actor_index`中获得的一样。以下代码段构造了一个索引张量，并将其传递给`set_dof_state_tensor_indexed`：
```python
actor_indices = torch.tensor([0, 17, 42], dtype=torch.int32, device="cuda:0")

gym.set_dof_state_tensor_indexed(sim, _dof_states, gymtorch.unwrap_tensor(actor_indices), 3)
```



关节型actor的状态可以使用其在root state tensor中的条目和其在DOF state tensor的条目来完全定义。重置base固定不动的actor，如操纵器臂，可以通过仅设置新的DOF状态来完成。重置没有固定在某处的机器人，如行走机器人，需要设置新的root state和新的DOF states。在这种情况下，您可以使用相同的actor索引张量，如下所示： 

```python
actor_indices = torch.tensor([0, 17, 42], dtype=torch.int32, device="cuda:0")
_actor_indices = gymtorch.unwrap_tensor(actor_indices)

gym.set_actor_root_state_tensor_indexed(sim, _root_states, _actor_indices, 3)
gym.set_dof_state_tensor_indexed(sim, _dof_states, _actor_indices, 3)
```

## All Rigid Body States


刚体状态张量(rigid body state tensor)包含仿真中所有刚体的状态。

每个刚体的状态与root state张量所描述的相同——13个浮点数确定的位置、方向、线速度和角速度。

刚体状态张量的形状是`(num_rigid_bodys，13)`。通过调用`gym.get_sim_rigid_body_count(sim)`可以获得仿真中刚体的总数。

刚体状态按顺序排列。

张量以actor 0的所有bodies开始，然后是actor 1的所有body，依此类推。

每个actor的body的顺序与函数get_actor_rigid_body_states和set_actor_regid_body-states相同。

某个actor的body数量可以使用`gym.get_actor_rigid_body_count(env，actor)`获得。张量中特定刚体的全局指数可以通过多种方式获得： 

* Calling `gym.get_actor_rigid_body_index(env, actor_handle, i, gymapi.DOMAIN_SIM)` will return the global rigid body index of the ith rigid body of the specified actor.

* Calling `gym.find_actor_rigid_body_index(env, actor_handle, rb_name, gymapi.DOMAIN_SIM)` will look up the global rigid body index by name.



函数`acquire_rigid_body_state_tensor`返回一个Gym tensor descriptor，该descriptor可以封装为PyTorch张量，如前所述：
    
    _rb_states = gym.acquire_rigid_body_state_tensor(sim)
    rb_states = gymtorch.wrap_tensor(_rb_states)

 函数`refresh_rigid_body_state_tensor`使用仿真的最新数据更新掉该张量： 
    
    gym.refresh_rigid_body_state_tensor(sim)


* **注意**：目前，刚体状态张量是只读的。只有使用根状态张量的actor root body才允许设置刚体状态。 

## Jacobians and Mass Matrices

雅可比矩阵和质量矩阵是机器人控制中的重要工具。雅可比矩阵将自由度的关节空间速度映射为其笛卡尔积和角速度，而质量矩阵包含取决于当前配置的机器人的广义质量。它们用于标准的机器人控制算法，如反向运动学或操作空间控制。


雅可比矩阵和质量矩阵都是使用张量来暴露的。该方法与其他张量函数略有不同，因为雅可比矩阵和质量矩阵的大小可能因actor的类型而异。获取雅可比张量或质量矩阵张量时，必须指定参与者名称。然后，张量将包含所有环境中具有该名称的所有actor的矩阵——假设每个具有该名字的actor都是相同类型的。调用create_actor时会提供actor名称。

* **假设**：本节的其余部分假设在作为franka asset实例的每个env中都有一个名为“franka”的actor：

```python
# acquire the jacobian and mass matrix tensors for all actors named "franka"
_jacobian = gym.acquire_jacobian_tensor(sim, "franka")
_massmatrix = gym.acquire_mass_matrix_tensor(sim, "franka")

# wrap as pytorch tensors
jacobian = gymtorch.wrap_tensor(_jacobian)
mm = gymtorch.wrap_tensor(_massmatrix)
```

* **注意** 雅可比矩阵可用于CPU和GPU pipeline，但质量矩阵目前仅可用于CPU。未来计划支持GPU质量矩阵。

未完待续…………

# Contact Tensors

# Control Tensors

各种状态张量（root state tensor, DOF state tensor, and rigid body state tensor）可用于get有关actor的信息和即时set新的姿态和速度。在重置期间，当actor需要返回到原始位姿或使用新的初始条件重新启动任务时，以这种方式设置状态是合适的。然而，应该谨慎地直接使用这些张量来设置新的状态。actor的位置和速度由物理引擎管理，该引擎考虑了碰撞、外力、内部约束和驱动。在仿真过程中，通常应避免直接设置新的位置和速度，因为这会覆盖物理引擎并导致非物理行为。

如果非要在模过程中管理actor行为，可以使用以下API应用DOF力或PD控制。

## DOF Controls
函数`set_dof_actuation_force_tensor`可用于对仿真中的所有自由度施加力。例如，以下是如何使用PyTorch对所有自由度应用随机力： 
```python
# get total number of DOFs
num_dofs = gym.get_sim_dof_count(sim)

# generate a PyTorch tensor with a random force for each DOF
actions = 1.0 - 2.0 * torch.rand(num_dofs, dtype=torch.float32, device="cuda:0")

# apply the forces
gym.set_dof_actuation_force_tensor(sim, gymtorch.unwrap_tensor(actions))
```

注意，驱动力仅适用于使用`set_actor_DOF_properties`将driveMode设置为`gymapi.DOF_MODE_EFFORT`的DOF。对于棱柱（平移）自由度，力以牛顿为单位。对于旋转（旋转）自由度，力以Nm为单位。力张量force tensor中自由度的排序与DOF state tensor中的排序相同。

函数`set_dof_position_target_tensor`可用于设置仿真中所有DOF的PD位置目标。请注意，位置目标仅对使用     `set_actor_DOF_properties`将driveMode设置为`gymapi.DOF_MODE_POS`的DOF生效。对于棱镜（平移）自由度，位置目标以米为单位。对于旋转（旋转）自由度，位置目标以弧度为单位。位置目标张量position target tensor中自由度的排序与DOF state tensor中的排序相同。

函数`set_dof_velocity_target_ttensor`可用于为仿真中的所有dof设置PD速度目标。请注意，速度目标仅对使用`set_actor_DOF_properties`将driveMode设置为`gymapi.DOF_MODE_VEL`的DOF生效。对于棱柱（平移）自由度，速度目标以米/秒为单位。对于旋转（旋转）自由度，速度目标以弧度每秒为单位。速度目标张量velocity target tensor中自由度的排序与DOF state tensor中的排序相同。

基于张量的控制函数有索引变体，可用于控制索引张量中指定的actor的子集，类似于`set_dof_state_tensor_indexed`。它们是`set_dof_actuation_force_tensor_indexed`、`set_dof_position_target_sensor_indexed`和`set_dof_velocity_sensor_inindexed`。

## Body Forces

可以使用两种不同的函数将力应用于刚体。

函数`apply_rigid_body_force_tensors`可用于在仿真中的每个刚体的质心处施加力和/或力矩：

```python
# apply both forces and torques
gym.apply_rigid_body_force_tensors(sim, force_tensor, torque_tensor, gymapi.ENV_SPACE)

# apply only forces
gym.apply_rigid_body_force_tensors(sim, force_tensor, None, gymapi.ENV_SPACE)

# apply only torques
gym.apply_rigid_body_force_tensors(sim, None, torque_tensor, gymapi.ENV_SPACE)
```

最后一个参数是一个`CoordinateSpace enum`枚举类型，它指定力和转矩矢量在哪个空间（LOCAL_space、ENV_space(default)或GLOBAL_space）。


如果希望仅对bodies的一个被选定的子集应用力或力矩，请确保将相应的张量条目设置为零。


另一个函数是`apply_rigid_body_force_at_pos_tensors`，它可以在给定位置对物体施加力。当力位置不在质心时，此功能将自动计算并应用所需的扭矩： 

请查看`apply_forces.py`和`apply_foforces_at_pos.py`示例以了解示例用法。


```python
# apply forces at given positions
gym.apply_rigid_body_force_at_pos_tensors(sim, force_tensor, pos_tensor, gymapi.ENV_SPACE)

```

# Common Problems

# Limitations