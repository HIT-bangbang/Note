Simulation Tuning

## Simulation Parameters
SimParams允许您为物理解算器指定参数。

SimParams分为Common Parameters PhysX的Params 以及Flex的params

## Simulation Parameters Tuning

PhysX CPU或Flex都可以用作仿真后端。

对PhysX GPU关节解算器的支持将很快被添加。

有两个常见参数可以调整以提高性能和/或仿真稳定性——**时间步长**和**子步骤数量**以及一些引擎特定参数。

目前只有“Flex”引擎才支持可变形对象仿真。

## Common Parameters

* $dt$ :仿真时间步长，默认值为$1/60 s$


* $substeps$-仿真子步骤(substeps)的数量，默认值为2。有效的仿真时间步长是$dt/substeps$。


$dt$参数是用于推进仿真的时间步长，单位为秒。

精确和稳定的物理仿真需要相当短的时间步长，通常在$1/50$秒以下。时间步长过长会导致不稳定性，尤其是在快速移动的物体、强大的力或复杂的铰接组件的情况下。除了推进仿真之外，通常还会有一些代码用于查询世界的状态，并在循环的每次迭代中应用controls。然后，$dt$参数决定了与仿真交互的频率。通常，这种相互作用的频率低于稳定物理仿真所需的频率。

`num_substeps`参数可用于将每个时间步长细分为相等的子间隔，以实现稳定的物理仿真。在上面的代码片段中，dt是1/60，因此您可以在每仿真一秒中与仿真交互60次。但num_substeps是2，这意味着物理仿真以1/120秒的增量进行。这些是默认的仿真参数。

## Flex


* `solver_type`-有6个约束解算器可用于Flex，编号如下：

    * `0-XPBD（GPU）`-一种使用迭代下降方法的基于位置的解算器，该方法的精度不如牛顿解算器（下图），但对于中等刚度的系统来说，通常快速且稳健。 


    Flex还支持以许多不同的线性解算器作为后端的非线性牛顿解算器，如下所示：

    * `1-Newton Jacobi (GPU)` - Jacobi solver on GPU (CUDA)

    * `2-Newton LDLT (CPU)` - Cholesky backend on CPU (Eigen-based)

    * `3-Newton PCG1 (CPU)` - Preconditioned conjugate gradient on CPU (Eigen-based)

    * `4-Newton PCG2 (GPU)` - Preconditioned conjugate gradient on GPU (CUDA)

    * `5-Newton PCR (GPU)` - Preconditioned conjugate residual method on GPU (CUDA)



默认和推荐的是5-Newton PCR求解器，CPU后端对于大型系统可能非常慢，并且主要用于验证目的。 

* `num_outer_iterations`—解算器在每个仿真子步骤中进行的迭代次数。


* `num_inner_iterations`-每次外部迭代的线性解算器迭代次数，仅Newton solvers使用


* `relaxation`-控制解算器的收敛速度。默认值为0.75大于1的值可能会导致不稳定。牛顿解算器目前使用零速度启动迭代，因此，如果收敛不充分，过高的松弛可能会引入a kind of numerical equivalent to damping = 1 - (1 - relaxation)^numOuterIterations.


* `warm_start`-将在下一个仿真步骤中使用的缓存拉格朗日乘子的分数。加速收敛，默认保守值为0.4。在系统中存在快速移动和/或大的、快速变化的力的情况下，较大的值可能会导致更具弹性的行为，有时还会导致不稳定性。在你试图仿真一个移动速度很慢、有很多触点的系统的情况下，比如说，抓取复杂形状的机器人操作任务，你可以从尝试更高的热启动值中受益，最高可达1.0。


* `shape_collision_distance`-粒子相对于刚性形状保持的距离（以米为单位）（与半径参数分开）。


* `shape_collision_margin`-Distance in meters at which contacts are generated。Flex使用推测性接触模型，当特征对（例如：顶点/边）在时间步长开始时彼此处于该距离内时，将生成接触约束。如果身体移动很快，那么余量应该足够大，以确保在一个时间步长内不会错过接触。形式化这一点的一种方法是说，如果特征行进的最大速度是v，时间步长是dt，那么裕度应该是margin=v*dt/substeps。如果你从一个很大的高度放下物体，看到它们“弹出”回来，那么很可能不得不降低碰撞裕度（物体相互穿透，然后弹出）。

提高仿真稳定性（收敛性）和减少穿透penetrations的最通用方法是增加子步骤的数量。但这在仿真上可能相当昂贵。其他可以尝试的方法有：

在缺乏收敛的情况下，增加外部和内部迭代的数量可能会有所帮助。不稳定性的另一个来源可能是由于糟糕的初始配置和自碰撞的存在。它可以通过可视化系统中的接触力并在可能的情况下禁用机器人的自碰撞或固定初始配置来进行诊断。除了增加子步骤的数量，在可能的情况下使shapeCollisionDistance更大，还有助于防止快速移动的物体穿透。如果在缓慢移动的系统中存在贯穿件，如机械臂，首先应检查仿真中产生的所有接触力，并在需要时减少接触力，例如通过削弱机器人电机。


Flex中的形状表示是通过三角形网格进行的-包括球体、胶囊和长方体在内的所有基本体形状都由存储在GPU BVH结构中的三角形网格内部表示（球体和胶囊可以被认为是单个退化三角形+厚度）。这种统一表示意味着Flex可以处理动态物理形状的非凸和任意三角形网格，但与例如：凸多面体不同，三角形汤通常不会定义“内部/外部”区域。这意味着一旦发生相互渗透，就无法以明确的方式解决。为了避免相互渗透，建议在碰撞形状上使用足够厚度的层。

## PhysX

* `num_threads`—PhysX使用的CPU内核数。默认值为4。设置为0将在调用`PxScene:：simulate()`的线程上运行仿真。大于0的值将生成`numCores-1`进程


`solver_type`—使用的解算器类型。默认和建议使用的是1-TGS:temporal gauss seidel solver。它是一个非线性迭代求解器。


`contact_offset`-距离小于其contactOffset值总和的形状将生成contacts。默认0.02米


`rest_offset`-两个形状将在等于其restOffset值总和的距离处静止。默认值为0.01米


`num_position_editions`—PhysX解算器位置迭代次数。默认值4


`num_velocity_iterations`-PhysX解算器速度迭代计数。默认值1


`bounce_threshold_velocity`-相对速度低于此速度的接触不会反弹。默认值为0.2 m/s


`max_depenetration_velocity`-解算器允许引入的最大速度，以校正接触中的穿透penetrations。默认值为100.0 m/s

为了提高求解器的收敛性，通常只应增加位置迭代次数。速度迭代会对解算器收敛产生负面影响。它们在那里是为了减少穿透带来的一些能量增益，但使整体仿真不那么僵硬。TGS解算器的速度迭代的默认选择为0是合适的。必须根据具体情况来看待不稳定问题。然而，一般规则是：


如果它是由深度穿透引起的，那么应该使用更多的子步骤，或者限制解算器可以注入的能量来纠正错误将有所帮助。参见PxRigidBody:：设置最大穿透速度。默认值为5 m/s，但较大的值，如100m/s，有助于在存在较大作用力的情况下移除穿透。


如果可能的话，增加关节电枢值也可以提高仿真的稳定性。


如果不稳定性基本上是系统无法收敛，那么增加位置迭代或子步进的次数应该会有所帮助。如果使用PGS，子步对仿真质量的影响比增加迭代大得多，但子步要贵得多（迭代相对便宜）。如果使用TGS，子步和迭代在如何影响收敛方面更相似。迭代比子步骤便宜得多，但不如PGS解算器便宜。只要可能，强烈建议使用TGS而不是PGS。其他可能影响稳定性的参数有：形状接触偏移（余量）、静止偏移（膨胀）

## Scene parameters

`bounce_threshold_velocity`—触发反弹所需的相对速度。默认值为0.2 m/s，在某些情况下可能会非常高。减x小这一数值可以提供更自然的弹跳行为，例如在弹跳球的情况下。


`friction_offset_threshold`——开始计算摩擦力的接触距离。默认值为0.04米。如果仿真非常小的对象，则应将其减小。


`friction_correlation_distance`-如果接触点之间的距离小于correlation distance，则可以将接触点合并为单个friction anchor。默认值为0.025米。

## PhysX可视化调试器（PVD） 

使用PhysX仿真时，PhysX Visual Debugger（PVD）允许您可视化、调试物理场景表示，并与之交互：https://developer.nvidia.com/pvd-tutorials


只有当您使用PhysX后端时，PVD才会工作。


如果你是从源代码开始构建，请在`premake5.lua`中搜索这一段：

    local physxLibs = "profile"

`physxLibs`变量应设置为“profile”或“checked”，以便PVD工作。如果更改此变量，请确保重新编译。


默认情况下，Gym将尝试连接到本地主机上运行的PVD。如果要连接到其他计算机上的PVD，请将环境变量`GYM_PVD_HOST`设置为IP或主机名。


您可以在终端中设置环境变量，也可以在Python脚本中这样做：

```
import os
os.environ["GYM_PVD_HOST"] = "xxx.yyy.zzz.www"
```

如果要将PVD捕获保存到文件中，而不是实时连接到PVD，请将环境变量`GYM_PVD_file`设置为文件名。您可以省略extension。

例如：

    os.environ["GYM_PVD_FILE"] = "foo"

这会创建一个名为foo.pxd2的文件