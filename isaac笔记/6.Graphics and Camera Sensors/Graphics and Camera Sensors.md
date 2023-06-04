# Graphics and Camera Sensors

Isaac Gym公开API以程序方式控制场景的视觉方面。

此外，Isaac Gym公开了API来管理来自许多相机的视图，并将这些相机视为机器人上的传感器。

以下部分介绍了相机属性、相机传感器、视觉属性修改以及与图形和相机传感器相关的其他主题。

# Camera Properties


Isaac Gym中的所有相机，包括viewer的相机，都可以使用`GymCameraProperties`结构体中传递的参数创建。
注：viewer就是那个可视化的gui

camera属性结构包含财产，例如视野、分辨率、近剪裁距离和远剪裁距离(field of view, resolution, near and far clipping distances)以及一些其他选项。

以正常方式创建viewer时： 

    viewer = gym.create_viewer(sim, gymapi.CameraProperties())

这将构造具有默认值的camera属性结构，并用于viewer摄影机。也可以按以下代码自定义viewer camera属性： 

```python
camera_props = gymapi.CameraProperties()
camera_props.horizontal_fov = 75.0
camera_props.width = 1920
camera_props.height = 1080
viewer = gym.create_viewer(sim, camera_props)
```
## Camera Properties:

* `width` - image width in pixels

* `height` - image height in pixels

* `horizontal_fov` - horizontal field of view in radians. The vertical field of view will be height/width * horizontal_fov

* `near_plane` - distance from the camera point to the near clipping plane in world units (meters)

* `far_plane` - distance from the camera point to the far clipping plane in world units (meters)

* `supersampling_horizontal` 水平超采样- integer multiplier for supersampling. Setting to a value larger than 1 will result in rendering an image of width*supersampling_horizontal pixels wide and resampling to output resolution

* `supersampling_vertical` 竖直超采样- integer multiplier for supersampling. Setting to a value larger than 1 will result in rendering an image of height*supersampling_vertical pixels tall and resampling to output resolution

* `use_collision_geometry` - if set to True, camera will render collision geometry

* `enable_tensors` - if set to True, tensor access will be enabled for this camera. See GPU Tensor Access section below for more.

# Camera Sensors

Isaac Gym的摄像头传感器是用来模拟摄像头的，因为它们要么被发现在观察机器人，要么被连接到机器人上。相机传感器可以很容易地创建，如下所示：

```python
camera_props = gymapi.CameraProperties()
camera_props.width = 128
camera_props.height = 128
camera_handle = gym.create_camera_sensor(env, camera_props)
```

与viewer camera一样，自定义的camera属性也可以传递到camera创建中。camera sensor与viewer相机的不同之处在于，它无法通过viewer进行控制，并且它连接到特定的环境。这意味着，当camera sensor创建图像时，它只能“看到”选定环境中的actor。

## 放置 camera sensor
### 方法一
摄像头传感器的位置可以通过三种方式之一进行设置。

首先，可以使用以下方法直接放置camera sensor： 

    gym.set_camera_location(camera_handle, env, gymapi.Vec3(x,y,z), gymapi.Vec3(tx,ty,yz))

其中`(x，y，z)`是相机在环境的局部坐标系中的坐标，`(tx，ty，tz)`是相机正在观察的点的坐标，同样是在环境局部坐标中。

### 方法二
放置相机的第二种方法是直接指定`GymTransform`，它指定相机在环境局部坐标中的位置和姿态，例如：

```python
transform = gymapi.Transform()
transform.p = (x,y,z)
transform.r = gymapi.Quat.from_axis_angle(gymapi.Vec3(0,1,0), np.radians(45.0))
gym.set_camera_transform(camera_handle, env, transform)

```
### 方法三

最后，可以将相机直接连接到刚体上。这对于创建在body移动时跟踪body的摄影机或模拟camera直接连接到actor的某个部位非常有用。相机可以按如下方式附着到刚体：


```python
local_transform = gymapi.Transform()
local_transform.p = (x,y,z)
local_transform.r = gymapi.Quat.from_axis_angle(gymapi.Vec3(0,1,0), np.radians(45.0))
gym.attach_camera_to_body(camera_handle, env, body_handle, local_transform, gymapi.FOLLOW_TRANSFORM)
```
最后一个参数决定了连接的方式： 

* `gymapi.FOLLOW_POSITION` 相机将保持与刚体的固定的偏移offset(local_transform.p)，但不会随刚体旋转。
* `gymapi.FOLLOW_TRANSFORM` 相机将保持固定偏移，并随body旋转。

当连接到刚体时，摄影机的变换将在`step_graphics(sim)`中更新。

所有camera sensors都在一个API调用中一起渲染，以实现最佳性能：

    gym.render_all_camera_sensors(sim)

在`render_all_camer_sensors(sim)`之后，所有相机的所有输出都会准备好被使用retrieve了。 

# Camera Image Types

每个相机都渲染许多不同的输出图像。访问图像时，API采用图像选择器image selector，该选择器指示应用程序要检索或访问哪个输出图像。

## Camera Sensor Image Types:

* `IMAGE_COLOR` - 4x 8 bit unsigned int - RGBA color   RGBA颜色

* `IMAGE_DEPTH` - float - negative distance from camera to pixel in view direction in world coordinate units (meters)在视图方向上从相机到像素的负距离，单位为世界坐标（米）

* `IMAGE_SEGMENTATION` - 32bit unsigned int - ground truth semantic segmentation of each pixel. See
32位无符号整数-每个像素的基本真值语义分割
* `IMAGE_OPTICAL_FLOW` - 2x 16bit signed int - screen space motion vector per pixel, normalized 每个像素的屏幕空间运动矢量，标准化的

将深度图像线性化回世界坐标，也就是说像素的值以仿真的单位为单位，即米。

分割图像的每个像素都包含生成该像素的身体的分割ID。请参阅下面的分割图像部分。

为光流图像返回的值是16位带符号整数。以下代码显示如何将光流图像中的值转换为以像素为单位的值：

```python
optical_flow_image = gym.get_camera_image(sim, camera_handle, gymapi.IMAGE_OPTICAL_FLOW)
optical_flow_in_pixels = np.zeros(np.shape(optical_flow_image))
# Horizontal (u)
optical_flow_in_pixels[0,0] = image_width*(optical_flow_image[0,0]/2**15)
# Vertical (v)
optical_flow_in_pixels[0,1] = image_height*(optical_flow_image[0,1]/2**15)
```

## Accessing Camera Images


相机传感器渲染的图像可以通过调用所需图像类型的`get_camera_image`来访问，如下所示： 

    color_image = gym.get_camera_image(sim, env, camera_handle, gymapi.IMAGE_COLOR)

第三个参数选择应该返回哪个相机的哪种图像。上面的代码要求提供彩色（RGBA）图像。图像以numpy数组的形式返回。当图像包含矢量输出（如image_COOLOR和image_OPTICAL_FLOW）时，这些矢量被紧密地封装在移动最快的维度中。 

例子见 `graphics.py`


## GPU Tensor Access

当使用camera sensor输出来训练已经放在GPU上的模型时，一个关键的优化是防止将图像复制到Isaac Gym中的CPU。为了实现更好的性能，Isaac Gym提供了一种直接从GPU访问相机输出的方法，而无需复制回CPU内存。


为了激活张量访问，必须在相机属性结构体中的`enable_tensors`标志设置为True的情况下创建相机：

```python
camera_props = gymapi.CameraProperties()
camera_props.enable_tensors = True
cam_handle = gym.create_camera_sensor(env, camera_props)
```

在设置阶段，应通过调用如下的应用程序来存取相机的张量结构： 

    camera_tensor = gym.get_camera_image_gpu_tensor(sim, env, cam_handle, gymapi.IMAGE_COLOR)

The returned GPU tensor has a gpu device side pointer to the data resident on the GPU as well as information about the data type and tensor shape. Sharing this data with a deep learning framework requires a tensor adapter, like the one provided in the gymtorch module for PyTorch interop:

返回的GPU张量具有指向驻留在GPU上的数据的GPU设备侧指针以及关于数据类型和张量形状的信息。与深度学习框架共享这些数据需要一个张量适配器，就像PyTorch interop的gymthorch模块中提供的一样：

```python
camera_tensor = gym.get_camera_image_gpu_tensor(sim, env, cam_handle, gymapi.IMAGE_COLOR)
torch_camera_tensor = gymtorch.wrap_tensor(camera_tensor)
```

现在，在仿真的主循环中，应用程序必须声明何时开始和停止访问张量，以防止内存危险：

```python
while True:

    gym.simulate(sim)
    gym.fetch_results(sim, True)
    gym.step_graphics(sim)
    gym.render_all_camera_sensors(sim)
    gym.start_access_image_tensors(sim)
    #
    # User code to digest tensors
    #
    gym.end_access_image_tensors(sim)
```

`start_access_image_tensors(sim)`API通知Isaac Gym 有user code 想直接访问图像张量缓冲区image tensor buffers。此API将stall，直到上一次调用`render_all_camer_sensors（sim）`时所有图像张量被写入完毕。为了最大限度地减少停滞，这个API被编写为，为所有相机传感器停滞一次。（注意，这些sensor得是在enable_tensors标志设置为True的情况下创建的。）


在调用end_access_image_tensors（sim）之前，Isaac Gym不会再次写入张量。按照约定，对图像张量的访问应该在主循环的同一次迭代中开始和停止。在张量访问的开始和结束之间调用`render_all_camera_sensors（sim）`可能会导致应用程序死锁。


有关更多信息，请参阅`PyTorch interop example`示例和`tensor API`章节。

# Visual Properties

Isaac Gym包含许多API，旨在允许对视觉属性进行编程修改，以实现视觉域随机化等技术。本节简要介绍了修改body或场景scene的视觉属性的例程。

## Colors

任何刚体的颜色都可以使用API进行设置： 

    gym.set_rigid_body_color(env, actor_handle, rigid_body_index, gymapi.MESH_VISUAL_AND_COLLISION, gymapi.Vec3(r, g, b))

在上面的调用中：
* env是包含actor的环境，
* actor_handle是环境中actor的句柄，
* rigid_body_index是要修改其颜色的actor内的rigid body的索引。
* 第四个参数是一个网格选择器mesh selector，它允许caller指定是设置刚体的视觉网格的颜色`mesh_visual`、刚体的碰撞网格的颜色`mesh_collision`，还是同时设置两者的颜色`mesh-visual_and_collision`。
* 最后一个参数是新颜色的颜色向量，其中红色、绿色和蓝色是介于0.0和1.0之间的浮点值。这将设置整个刚体的颜色。如果网格具有不同颜色的子网格，则所有子网格的颜色都设置为指定的颜色。

刚体的当前颜色也可以通过以下方式查询：

    current_color = gym.get_rigid_body_color(env, actor_handle, rigid_body_index, gymapi.MESH_VISUAL)

使用网格选择器`mesh_VISUAL_AND_COLLISION`调用`get_rigid_body_color`时，将返回视觉网格的颜色。如果网格包含子网格submeshes，则返回第一个子网格的颜色。

## Textures


## Texture & Color Interaction


## Reset

在任何时候，整个actor的颜色和纹理都可以通过调用重置为从asset文件加载的原始值：

    gym.reset_actor_materials(env, actor, gymapi.MESH_VISUAL_AND_COLLISION)

这将重置指定类型（视觉、碰撞或两者兼有）的actor中所有bodies的材质。

## Segmentation ID

## Lights

# Lines API

# Camera Intrinsics

Isaac Gym的一些高级用途，例如为了将深度图像投影到3D点云，需要完全了解用于创建输出图像的投影项。为了帮助实现这一点，Isaac Gym提供了对用于渲染相机视图的projection_matrix和view_matrix的访问。他们是：
```python
projection_matrix = np.matrix(gym.get_camera_proj_matrix(sim, env, camera_handle))

view_matrix = np.matrix(gym.get_camera_view_matrix(sim, env, camera_handle))
```

这两个函数都返回一个numpy数组，该数组在上面的代码示例中被转换为numpy矩阵。

# Step Graphics

当需要渲染时，转换和信息必须从物理仿真传递到图形系统。为了支持图形和物理不以相同的更新速率运行的用例，例如，如果图形只在每$N$步渲染一次。Isaac Gym允许手动控制此过程。

当使用许多相机传感器时，这一点更为重要，因为渲染器必须知道何时移动到新帧，但render()调用并不是隐式的帧边界，因为对于viewer和camer sensor，每一step可能有许多render()调用。帧边界更新通过`step_graphics(sim)`API进行显式更新。


然而，在大多数情况下，`step_graphics()`将在调用`draw_viewer()`和/或`render_all_camer_sensors()`之前立即调用，如下所示：

```python
while not gym.query_viewer_has_closed(viewer):
    gym.simulate(sim)
    gym.fetch_results(sim, True)

    gym.step_graphics(sim)
    gym.render_all_camera_sensors(sim)
    gym.draw_viewer(viewer)

```