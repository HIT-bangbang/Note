# 注意，以下是基于espressif说的

ESP32的setup和loop其实是在looptask()任务中的，该任务优先级为1，运行在核心1
loop()函数其实是写在一个死循环中的，所以loop()里面的代码会一直重复执行
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
在app_main()中创建了loopTask()任务，并且调用了loopTask()函数。

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

在app_main()中调用的setup()函数和loop()函数其实是没有定义的实现的，只在arduino.h里面有个声明。
需要我们在main文件中定义setup()和loop()，将它实现出来。所以删掉setup和main会报错。


## 杀掉loopback()任务

setup()任务结束后，其实loopback()后面的事情已经没用了，可以直接杀掉loopback()了

    vTaskDeleye(NULL) //参数为任务句柄，在某个任务中调用时，NULL就代表任务自身



我感觉，如果不需要setup和loop的话，可以在loopTask()里面删掉，甚至不需要looptask()这个任务，直接在app_main()里面删掉loopTask()的创建，然后写自己的任务

## 迁移任务到freertos一定要注意的

1、setup()函数执行完就会释放内存，一定注意这里面的都是局部变量内存会被销毁
2、对于某个task要做的setup操作尽量都写到task函数里面去，而不是在setup里面调用
3、SETUP任务完成使命后，使用vTaskDelete(NULL)杀掉任务
4、setup()函数和loop()函数在核心1，自动分配核心的任务可能在核心0或者核心1，在与setup不同的核定可能会出现问题
