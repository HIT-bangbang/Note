

# 张金航2021/9/5
# rosserial协议解析以及移植

## 一、rosserial 协议解析 Protocol
### 1、数据包格式Packet Format
```c++
1st Byte - Sync Flag (Value: 0xff)
2nd Byte - Sync Flag / Protocol version
3rd Byte - Message Length (N) - Low Byte
4th Byte - Message Length (N) - High Byte
5th Byte - Checksum over message length
6th Byte - Topic ID - Low Byte
7th Byte - Topic ID - High Byte
x Bytes  - Serialized Message Data
Byte x+1 - Checksum over Topic ID and Message Data
```
The Protocol version byte was 0xff on ROS Groovy, 0xfe on ROS Hydro, Indigo, and Jade.

Topics ID 0-100 are reserved for system functions, as defined in the rosserial_msgs/TopicInfo message.
 rosserial_msgs/TopicInfo部分代码如下

```c++
# special topic_ids
uint16 ID_PUBLISHER=0
uint16 ID_SUBSCRIBER=1
uint16 ID_SERVICE_SERVER=2
uint16 ID_SERVICE_CLIENT=4
uint16 ID_PARAMETER_REQUEST=6
uint16 ID_LOG=7
uint16 ID_TIME=10
uint16 ID_TX_STOP=11
```

此处的Topic ID的称谓值得注意，我觉得理解为 Topic类别ID 更好，他区分了这个数据包话题是发布、订阅或service服务端客户端等等

The checksums on the length and data are used to make sure that a particular packet has not been corrupted.
The checksum over the message length is computed as follows:

```c++
Message Length Checksum = 255 - ((Message Length High Byte + 
                               Message Length Low Byte) % 256 )
```

The checksum over the Topic ID and data is computed as follows:
	Message Data Checksum = 255 - ((Topic ID Low Byte +
                                Topic ID High Byte + 
                                Data byte values) % 256)

### 2、话题协商Topic Negotiation
Before data transfer can begin, the PC/Tablet side must query the Arduino or other embedded device for the names and types of topics which will be published or subscribed to.
在通讯开始前，PC（上位机）比如向嵌入式硬件询问，将要被订阅和发送的话题的名词和类型。
Topic negotiation consists of a query for topics, a response with the number of topics, and packets to define each topic. The request for topics uses a topic ID of 0.
话题的request的topic ID 使用的ID号为0
以下给出一个query for topic的例子：

```c++
  0xff 0xfe 0x00 0x00 0xff 0x00 0x00 0xff
flag(定值0xff) flag(版本号) 数据长度码低位 数据长度码高位 数据长度码校验 topic ID 低位 topic ID 高位 传输的数据（此处没有数据发送，故不存在） topicID和数据共有的校验
```

下位机收到上位机的请求信息后，应该发送一系列的响应数据包。
A series of response packets (message type rosserial_msgs/TopicInfo, each containing information about a particular topic, with the following data in place of the serialized message:
数据包中的Serialized Message Data部分应替换成以下的格式：

```c++
uint16 topic_id //注意，这里的topic_ID不同于数据包中第6、7个字节的topic_ID
string topic_name
string message_type
string md5sum
int32 buffer_size
```

Here, the topic name is the name of the topic, for instance "cmd_vel", and message type is the type of the message, for instance "geometry_msgs/Twist".
If a response packet is not received correctly, another query may be sent.

### 时间 
Time synchronization is handled by sending a std_msgs::Time in each direction. The embedded device can request the current time from the PC/Tablet by sending an empty Time message. The returned time is used to find clock offset.
相互之间的时间同步通过发送消息 std_msgs::Time实现。 嵌入式设备可以向PC/平板发送空的时间消息获取当前时间，响应返回的时间可以用于时间同步（检查时钟偏差）

参考：
http://wiki.ros.org/rosserial/
https://blog.csdn.net/x_r_su/article/details/52734403
https://blog.csdn.net/qq_38288618/article/details/102931684

## 二、关于rosserial向stm32的移植
参考：
https://blog.csdn.net/wubaobao1993/article/details/70808959
http://wiki.ros.org/rosserial_client/Tutorials/Adding%20Support%20for%20New%20Hardware

The first step to using a new piece of hardware is defining the interface between the library and the hardware. The library is templated on a hardware object that must provide the following interface:
移植到新硬件的第一步是定义库和硬件之间的接口。
库以硬件的对象为模板，该对象必须提供以下接口：

    class Hardware
    {
    
      Hardware();
    
      // any initialization code necessary to use the serial port
      void init(); 
    
      // read a byte from the serial port. -1 = failure
      int read()
    
      // write data to the connection to ROS
      void write(uint8_t* data, int length);
    
      // returns milliseconds since start of program
      unsigned long time();
    
    };

写好这个类之后，就建立了rosserial库和底层的接口，可以通过以下的代码进行调用并创建节点句柄：
    #include "ros/node_handle.h"
    ros::NodeHandle_<Hardware> nh;

为了和PC端的ros代码格式保持一致，可以将上述代码中的 “NodeHandle_<Hardware>” 进行重命名：
    /*
     * ros.h
     */

    #ifndef _ROS_H_
    #define _ROS_H_
    
    #include "ros/node_handle.h"
    #include "HardwareImpl.h"
    
    namespace ros
    {
      typedef ros::NodeHandle_<HardwareImpl> NodeHandle;
    }
    
    #endif

总结：按照wiki的方法，生成rosserial的库（无所谓arduino或是其他平台），重写XXXhardware.h文件，文件中按照Wiki定义的接口要求重写 Class Hardware。
需要用到队列。





















