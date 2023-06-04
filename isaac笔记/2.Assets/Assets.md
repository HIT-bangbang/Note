# 加载资产（asset）

Gym目前支持加载URDF和MJCF文件格式。加载资产文件将创建一个GymAsset对象，该对象包括所有实体、碰撞形状、视觉附件、关节和自由度（DOF）的定义。某些格式也支持柔体和粒子。



加载资产时，可以指定资产根目录和相对于根的资产路径。这种拆分是必要的，因为导入器有时需要在资产目录树中搜索外部参照文件，如网格（meshes）或材质（materials）。资产根目录可以指定为绝对路径，也可以指定为相对于当前工作目录的路径。在Python示例中，我们加载的资产如下所示： 

```python  
    asset_root = "../../assets" #资产根目录
    asset_file = "urdf/franka_description/robots/franka_panda.urdf" #相对于根的路径
    asset = gym.load_asset(sim, asset_root, asset_file) #加载模型
```

load_asset方法使用文件扩展名来确定资产文件格式。支持的扩展名包括urdf文件的.udf和MJCF文件使用的.xml。

有时，您可能希望将额外信息传递给资产加载器(asset importer)。这是通过指定一个可选的AssetOptions参数来实现的：

```python
    asset_options = gymapi.AssetOptions()
    asset_options.fix_base_link = True
    asset_options.armature = 0.01

    asset = gym.load_asset(sim, asset_root, asset_file, asset_options)
```

# 覆写惯量属性

Gym使用Assimp加载资源中指定的网格(meshes)。某些网格可以在网格文件中直接指定材质或纹理。如果资源文件还为该网格指定了材质，则资源中的材质将优先。若要改用网格材质，请使用 

    asset_options.use_mesh_materials = True.

Gym尝试直接从网格加载法线。如果网格的法线不完整，Gym将生成平滑的顶点法线。若要强制Gym始终生成平滑顶点法线/面法线，请分别使用 



    asset_options.mesh_normal_mode = gymapi.COMPUTE_PER_VERTEX

或者

    asset_options.mesh_normal_mode = gymapi.COMPUTE_PER_FACE


如果网格具有表示凸分解的子网格，Gym可以将子网格作为单独的形状加载到资源中。要启用此功能，请使用

    asset_options.convex_decomposition_from_submeshes = True.



刚体的惯性财产对仿真精度和稳定性至关重要。每个刚体都有一个质心和一个惯性张量。URDF和MJCF允许指定这些值，但有时这些值不正确。例如，有许多URDF资产似乎具有任意的惯性张量，这可能会导致不希望的模拟伪像。为了克服这一点，Isaac Gym允许使用从碰撞形状的几何形状计算的值来明确地覆盖质心和惯性张量： 

    asset_options.override_com = True
    asset_options.override_inertia = True

默认情况下，这两个选项都为`False`，这意味着导入程序将使用原始资产中给定的值。
具体使用见`python/examples/convex_decomposition.py`

# 凸分解 Convex Decomposition

Isaac Gym支持用于碰撞形状的三角形网格的自动凸分解。这仅在使用PhysX时才需要，因为PhysX需要用于碰撞的凸网格（Flex可以直接使用三角形网格）。在没有凸分解的情况下，每个三角形网格形状都使用单个凸包进行近似。这是有效的，但单个凸包可能无法准确地表示原始三角形网格的形状。凸分解创建由多个凸形状组成的更精确的形状近似。这对于抓取等应用尤其有用，因为物理物体的确切形状很重要。 



默认情况下，凸分解是禁用的。您可以在资产导入期间使用AssetOptions中的标志启用它： 

    asset_options.vhacd_enabled = True



Gym使用第三方库（V-HACD）来执行凸分解。有许多参数会影响分解速度和结果的质量。它们在isaacgym.gyapi.VhacdParams类中暴露给Python，该类包含在AssetOptions中： 

    asset_options.vhacd_params.resolution = 300000
    asset_options.vhacd_params.max_convex_hulls = 10
    asset_options.vhacd_params.max_num_vertices_per_ch = 64

具体使用见`python/examples/convex_decomposition.py`


凸分解可能需要很长时间，这取决于参数和网格数。为了避免每次都重新计算凸分解，Gym使用了一个简单的缓存策略。第一次分解网格时，分解结果将存储在缓存目录中，以便在后续运行中快速加载。默认情况下，缓存目录为${HOME}/.isaacgym/vhacd。缓存目录及其包含的任何文件都可以随时安全删除。

您可以通过单击查看器GUI中的查看器选项卡并启用“渲染碰撞网格(“Render Collision Meshes”)”复选框来查看凸分解的结果。 

# 一些程序化的资产 用程序就可以简单描述的 Procedural Assets

以按程序创建简单的几何资源，如长方体、胶囊和球体：
```python
    asset_options = gym.AssetOptions()
    asset_options.density = 10.0

    box_asset = gym.create_box(sim, width, height, depth, asset_options)
    sphere_asset = gym.create_sphere(sim, radius, asset_options)
    capsule_asset = gym.create_capsule(sim, radius, length, asset_options)
```

# Asset Options

See  `isaacgym.gymapi.AssetOptions`

# Asset Introspection

可以检查每个资源中的零部件集合，包括刚体、运动类型和自由度。有关示例用法，请参见examples/asset_info.py。

# 创建actors

注意，加载资产不会自动将其添加到仿真环境中。GymAsset是actor的蓝图，可以在具有不同位姿和个性化属性的模拟中多次实例化。 

也就是说，`asset = gym.load_asset`仅仅是加载了模型，并未实例化，因为后续我们要进行并行的仿真，要以此为模板，实例化很多很多的该asset，每一个可能有不同的初始化参数。

如果actor的冲突过滤器(`collision_filter`)设置为-1，那么actor将使用资产加载器加载的过滤器。这对于指定非零联系人/联系人或指定其他联系人的MJCF文件非常重要。将碰撞过滤器设置为0将启用actor中所有形状之间的碰撞。将碰撞过滤器设置为任何>0的值都将禁用所有自碰撞。

# 一些限制

asset pipeline是一项正在进行的工作，因此存在一些局限性。


URDF导入器只能加载OBJ格式的网格。许多URDF模型都带有STL碰撞网格和DAE视觉网格，但这些网格需要手动转换为当前导入器的OBJ。


MJCF导入程序仅支持基本形状，如长方体、胶囊和球体。网格加载当前在该导入程序中不可用。


MJCF导入器支持一对实体之间的多个关节，这有助于定义独立命名和可控的自由度。这在类人关节_20_5.xml模型中用于定义肩部、臀部和其他复合关节的独立运动限制。


MJCF导入程序仅支持世界体不超过一个直接子体的文件。这意味着可能不支持定义整个环境的MJCF文件。例如，一个MJCF文件不能同时包含机器人和地平面。

# 例子

Take a look at the Python examples `asset_info.py` and `joint_monkey.py` for working with assets.