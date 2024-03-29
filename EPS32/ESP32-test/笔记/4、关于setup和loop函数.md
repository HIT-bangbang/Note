# 注意，以下是基于espressif说的

ESP32的setup和loop其实是在looptask()任务中调用的两个函数

looptask()任务任务优先级为1，运行在核心1

loop()函数是写在一个for(;;)死循环中的，所以loop()里面的代码会一直重复执行

```c++
void loopTask(void *pvParameters)
{
    setup();
    for(;;) {
#if CONFIG_FREERTOS_UNICORE
        yieldIfNecessary();
#endif
        if(loopTaskWDTEnabled){
            esp_task_wdt_reset();
        }
        loop();
        if (serialEventRun) serialEventRun();
    }
}
```

loopTask()任务在app_main()被创建：


```c++
extern "C" void app_main()
{
#if ARDUINO_USB_CDC_ON_BOOT && !ARDUINO_USB_MODE
    Serial.begin();
#endif
#if ARDUINO_USB_MSC_ON_BOOT && !ARDUINO_USB_MODE
    MSC_Update.begin();
#endif
#if ARDUINO_USB_DFU_ON_BOOT && !ARDUINO_USB_MODE
    USB.enableDFU();
#endif
#if ARDUINO_USB_ON_BOOT && !ARDUINO_USB_MODE
    USB.begin();
#endif
    loopTaskWDTEnabled = false;
    initArduino();
    xTaskCreateUniversal(loopTask, "loopTask", getArduinoLoopTaskStackSize(), NULL, 1, &loopTaskHandle, ARDUINO_RUNNING_CORE);
}

```

在app_main()中调用的setup()函数和loop()函数在arduino.h里面有个声明，但是没有被实现的。

所以，在main文件中的setup()和loop()其实就是在将这两个函数实现出来。

删掉main.cpp中的setup()和main()会报错也是这个原因，因为loopTask()中调用了setup()和loop()这两个函数，但是它没被实现。


## 杀掉loopback()任务

我们使用freertos 在setup()函数中创建完所需的任务后，其实loopback()任务已经没用了，可以直接杀掉loopback()了

直接在setup()的最后面调用vTaskDeleye(NULL)就可以杀掉loopback()任务，

    vTaskDeleye(NULL) //参数为任务句柄，在某个任务中调用时，NULL就代表任务自身


如果不需要setup和loop的话，可以在源代码loopTask()里面删掉，甚至不需要looptask()这个任务，直接在app_main()里面删掉loopTask()的创建，然后写自己的任务

## 迁移任务到freertos一定要注意的

* 1、setup()是个函数，执行完就会释放内存，一定注意这里面的都是局部变量内存会被销毁
* 2、对于某个task要做的setup操作尽量都写到task函数里面去，而不是在setup里面调用
* 3、SETUP任务完成使命后，使用vTaskDelete(NULL)杀掉任务
* 4、setup()函数和loop()函数在核心1，自动分配核心的任务可能在核心0或者核心1，在与setup不同的核定可能会出现问题
* 5、开启WIFI或者蓝牙后，尽量使用核心1，不要使用核心0，因为蓝牙和WIFI均运行在核心0中，占用大量CPU资源。
