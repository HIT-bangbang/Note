# 1. 自动启动和结束
ros::NodeHandle管理着一个内部引用数，使得开启和结束一个节点（node）可以简单地按照下面一行代码完成：

    ros::NodeHandle nh;

# 2. 命名空间（Nnamespaces）
NodeHandles可以指定一个命名空间给它的构造函数：

    ros::NodeHandle nh("my_namespace");

这行代码将创建一个相对于NodeHandle的域名，<node_namespace>/my_namespace，而不是直接地表示为<node_namespace>

你也可以指定一个父节点的NodeHandle和命名空间，后面可以跟子节点的NodeHandle和命名空间：
```
ros::NodeHandle nh1("ns1");
ros::NodeHandle nh2(nh1, "ns2");
```
这将会把nh2放在<node_namespace>/ns1/ns2的命名空间下。

## 全局域名（Global Names）

如果确实需要，你可以指定一个全局域名：

    ros::NodeHandle nh("/my_global_namespace");

这样的方式通常不是很推荐，因为它会阻止节点被放进命名空间（如使用roslaunch）。但是，有时候，在代码中使用全局命名空间也会很有用。

## 私有域名（Private Names）

使用私有域名比起直接调用一个带有私有域名（"~name"）的NodeHandle函数要麻烦一点。相反，你必须创建一个新的位于私有命名空间的NodeHandle：
```
ros::NodeHandle nh("~my_private_namespace");
ros::Subscriber sub = nh.subscribe("my_private_topic", ...);
```
上面这个例子将会订阅话题<node_name>/my_private_namespace/my_private_topic

注意：区分node_namespace和node_name，彼此关系其实是: node_name = node_namespace + nodename
.launch 文件中 ns==“node_namespace”
node中 ns=="node_namespace"

## 举例
```xml
<!-- node_namespace -->
<node pkg="publish_node" type="publish_node_A" name="fusionA" output="screen" ns="node_namespace">
```

```c++
//node name
ros::init(argc, argv, "node_name"); 
```

```c++

//n命名空间为/node_namespace
ros::NodeHandle n; //n 

// n1命名空间为/node_namespace/sub
ros::NodeHandle n1("sub"); 

// n2命名空间为/node_namespace/sub/sub2
ros::NodeHandle n2(n1,"sub2");

//pn1 命名空间为/node_namespace/node_name
ros::NodeHandle pn1("~"); 

//pn2 命名空间为/node_namespace/node_name/sub
ros::NodeHandle pn2("~sub"); 

//pn3 命名空间为/node_namespace/node_name/sub
ros::NodeHandle pn3("~/sub"); 

//gn命名空间为/global
ros::NodeHandle gn("/global"); 

```