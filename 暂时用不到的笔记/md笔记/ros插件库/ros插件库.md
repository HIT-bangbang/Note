# pluginlib
## 1.概述
pluginlib is a C++ library for loading and unloading plugins from within a ROS package. Plugins are dynamically loadable classes that are loaded from a runtime library (i.e. shared object, dynamically linked library). With pluginlib, one does not have to explicitly link their application against the library containing the classes -- instead pluginlib can open a library containing exported classes at any point without the application having any prior awareness of the library or the header file containing the class definition. Plugins are useful for extending/modifying application behavior without needing the application source code.

pluginlib是一个C++库，用于从ROS包中加载和卸载插件。插件是从运行库（即共享对象、动态链接库）加载的可动态加载类。使用pluginlib，用户不必将其应用程序与包含类的库显式链接——相反，pluginlib可以在任何时候打开包含导出类的库，而应用程序事先不知道库或包含类定义的头文件。插件对于扩展/修改应用程序行为非常有用，而不需要应用程序源代码。

## 2.example

To understand how pluginlib works, let's consider a small example. First, suppose that a ROS package exists containing a polygon base class ("polygon_interface package"). Let's also say that there are two different kinds of polygons supported in the system: a rectangle which lives in the "rectangle_plugin" package and a triangle that lives in the "triangle_plugin" package. The implementers of both the rectangle_plugin and triangle_plugin packages would include special export lines in their package.xml file telling the build system that they intend to provide plugins for the polygon class in the polygon_interface package. These export lines, in effect, register the classes with the ROS build/packaging system. This means that someone wishing to see all polygon classes available in the system can run a simple rospack query which will return a list of available classes, in this case, rectangle and triangle.

为了了解pluginlib的工作原理，让我们考虑一个小例子。首先，假设存在一个包含多边形基类的ROS包（“polygon_interface包”）。我们还假设系统中支持两种不同类型的多边形：一种是矩形，位于“rectangle_plugin”包中，另一种是三角形，位于“triangle_plusgin”包。rectangle_plugin和triangle_plusgin包的实现者将在package.xml文件中包含特殊的导出行(export lines)告诉构建系统他们打算为polygoninterface包中的多边形类提供插件。实际上，这些export lines在ROS构建/打包系统中注册类。这意味着希望查看系统中所有可用多边形类的人可以运行一个简单的rospack查询，该查询将返回可用类的列表，在本例中为矩形和三角形。

## 3.Providing a Plugin
### 3.1 Registering/Exporting a Plugin

为了允许动态加载类，必须将其标记为导出类exported class。这是通过特殊宏PLUGINLIB_EXPORT_CLASS完成的。这个宏可以放在组成插件库的任何源文件（.cpp）中，但通常放在导出类的.cpp文件的末尾。对于上面的示例，我们可以在包“example_pkg”中创建一个class_list.cpp文件，如下所示，并将其编译到librectangle库中：

```c++
#include <pluginlib/class_list_macros.h>
#include <polygon_interface/polygon.h>
#include <rectangle_plugin/rectangle.h>

//Declare the Rectangle as a Polygon class
PLUGINLIB_EXPORT_CLASS(rectangle_namespace::Rectangle, polygon_namespace::Polygon)
```
### 3.2 The Plugin Description File

The plugin description file is an XML file that serves to store all the important information about a plugin in a machine readable format. It contains information about the library the plugin is in, the name of the plugin, the type of the plugin, etc. If we consider the rectangle_plugin package discussed above, the plugin description file (e.g. rectangle_plugin.xml), would look something like this:

插件描述文件是一个XML文件，它用机器可读的格式存储插件的所有重要信息。它包含有关插件所在库、插件名称、插件类型等的信息。如果我们考虑上面讨论的rectangle_plugin包，插件描述文件（例如rectangle_prugin.xml）将如下所示：
```xml
<library path="lib/librectangle">
  <class type="rectangle_namespace::Rectangle" base_class_type="polygon_namespace::Polygon">
  <description>
  This is a rectangle plugin
  </description>
  </class>
</library>
```
For a detailed description of plugin description files and their associated tags/attributes please see the following:http://wiki.ros.org/pluginlib/PluginDescriptionFile

#### 3.2.1 Why Do We Need This File
We need this file in addition to the code macro to allow the ROS system to automatically discover, load, and reason about plugins. The plugin description file also holds important information, like a description of the plugin, that doesn't fit well in the macro.
除了代码宏之外，我们还需要这个文件，以允许ROS系统自动发现、加载和推理插件。插件描述文件还包含一些重要信息，例如插件的描述，这些信息在宏表述中并不适合。

## 3.3 在ROS包系统中注册插件

In order for pluginlib to query all available plugins on a system across all ROS packages, each package must explicitly specify the plugins it exports and which package libraries contain those plugins. A plugin provider must point to its plugin description file in its package.xml inside the export tag block. Note, if you have other exports they all must go in the same export field.

Considering the rectangle_plugin package again, the relevant lines would look as follows:

为了让pluginlib跨所有ROS包查询系统上的所有可用插件，每个包必须明确指定其导出的插件以及包含这些插件的包库。插件提供程序必须指向导出标记块内package.xml中的插件描述文件。注意，如果您有其他导出，则它们都必须位于同一导出字段中。
再次考虑rectangle_plugin包，相关行如下所示：
```xml
<export>
  <polygon_interface plugin="${prefix}/rectangle_plugin.xml" />
</export>
```
For a detailed discussion of exporting a plugin, please see the following documentation.http://wiki.ros.org/pluginlib/PluginExport

    **注意：** 为了使上述导出命令正常工作，提供的包必须直接依赖于包含插件接口的包。例如，rectangle_plugin的catkin/package.xml中必须包含以下行：
```xml
<build_depend>polygon_interface</build_depend>
<run_depend>polygon_interface</run_depend>
 ```

 ## 3.4 Querying ROS Package System For Available Plugins

在终端中可以通过rospack查询ROS包系统，以查看任何给定包都可以使用哪些插件。例如返回从nav_core包导出的所有插件：

    rospack plugins --attrib=plugin nav_core


# 4.Using a Plugin

pluginlib provides a ClassLoader class available in the class_loader.h header that makes it quick and easy to use provided classes. For detailed documentation of the code-level API for this tool please see pluginlib::ClassLoader documentation. Below, we'll show a simple example of using the ClassLoader to create an instance of a rectangle in some code that uses a polygon:

pluginlib提供了class_loader.h头文件中可用的ClassLoader类，使其快速且易于使用所提供的类。有关此工具的代码层面的API的详细文档，请参阅pluginlib:：ClassLoader文档。下面，我们将展示一个使用ClassLoader在一些使用多边形的代码中创建矩形实例的简单示例：

```c++
#include <pluginlib/class_loader.h>
#include <polygon_interface/polygon.h>

//... some code ...

pluginlib::ClassLoader<polygon_namespace::Polygon> poly_loader("polygon_interface", "polygon_namespace::Polygon");

try
{
  boost::shared_ptr<polygon_namespace::Polygon> poly = poly_loader.createInstance("rectangle_namespace::Rectangle");

  //... use the polygon, boost::shared_ptr will automatically delete memory when it goes out of scope
}
catch(pluginlib::PluginlibException& ex)
{
  //handle the class failing to load
  ROS_ERROR("The plugin failed to load for some reason. Error: %s", ex.what());
}
```
**Important Note:**  The ClassLoader must not go out scope while you are using the plugin. So, if you are loading a plugin object inside a class, make sure that the class loader is a member variable of that class.
**重要提示：** 当您使用插件时，ClassLoader不能超出范围。因此，如果要在类中加载插件对象，请确保class loader是该类的成员变量。

## 5.changes——自Groovy之前版本以来的变化