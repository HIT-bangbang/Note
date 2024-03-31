# 一、万物始于点灯

digitalWrite(led,HIGH);填数字就是说的芯片的GPIO引脚，写P1就是说的是电路板的引脚丝印号。一般就写数字了

```c++
int led = 2;
void setup() {
  pinMode(led,OUTPUT);
  // put your setup code here, to run once:
}

void loop() {
  digitalWrite(led,HIGH);
  delay(200);
  digitalWrite(led,LOW);
  delay(200);
  // put your main code here, to run repeatedly:
}
```

# 二、初试多任务，延时函数

例1：

```c++
#include <arduino.h>
// 任务一
// 参数为空指针
void task1(void *pt)
{
  while(1){
    Serial.println("task1");
    vTaskDelay(1000); //使用支持多任务的delay函数，注意这里的参数是ticks
    //delay(1000); // 经过测试，其实delay()也是支持多任务的，不会堵塞其他任务 
}
  }
// 任务二
void task2(void *pt)
{
  while(1){
    Serial.println("task2");
    vTaskDelay(5000);
    // delay(5000);

  }
}

void setup(){
  Serial.begin(9600);
  xTaskCreate(task1,"print1",1024,NULL,1,NULL);
  xTaskCreate(task2,"print1",1024,NULL,2,NULL);

}
// 必须要一个空的loop否则编译不过
 void loop(){}
```
例二：

```c++
void task1(void *pt) {
  pinMode(23, OUTPUT);
  while (1) {
    digitalWrite(23, !digitalRead(23));
    /*
      Tick to Time
        pdMS_TO_TICKS
        portTICK_PERIOD_MS
      vTaskDelay(pdMS_TO_TICKS(1000));
      vTaskDelay(1000/portTICK_PERIOD_MS);
    */
    vTaskDelay(1000); //注意 是 ticks的数量 不是时间
  }
}

void task2(void *pt) {
  pinMode(21, OUTPUT);
  while (1) {
    digitalWrite(21, !digitalRead(21));
    vTaskDelay(3000);
  }
}


void setup() {
  Serial.begin(9600);
  Serial.print("ESP32 Tick Period - ");
  Serial.print(portTICK_PERIOD_MS);
  Serial.println("ms");

  if (xTaskCreate(task1,
                  "Blink 23",
                  1024,
                  NULL,
                  1,
                  NULL) == pdPASS)
    Serial.println("Task1 Created.");

  if (xTaskCreate(task2,
                  "Blink 21",
                  1024,
                  NULL,
                  1,
                  NULL) == pdPASS)
    Serial.println("Task2 Created.");


}

void loop() {
}
```

## 延时函数

    vTaskDelay(ticks);

参数为ticks，在esp32中默认每1ms会有一个tick，所以这里直接填要演示的ms数字就可以了。如果是其他的设备，稳妥的方法是使用pdMS_TO_TICKS()函数或者使用portTICK_PERIOD_MS宏：

    /*
      Tick to Time
        pdMS_TO_TICKS
        portTICK_PERIOD_MS
      vTaskDelay(pdMS_TO_TICKS(1000));
      vTaskDelay(1000/portTICK_PERIOD_MS);
    */

eps32中的delay()其实就是用的vTaskDelay(),但是在其他开发板不一定，所以还是写vTaskDelay比较稳妥

# 三、任务之间传递参数

## 关于地址

esp32:  1 byte=8 bit

## 例子

```c++
/*
  程序： FREERTOS - 单个参数传递
        大家在看本程序的时候，需要对指针非常的了解
        知道 * -> &的作用
  作业： 添加LED3_PIN 15
  公众号：孤独的二进制
*/
byte LED1_PIN = 23;
byte LED2_PIN = 21;

void task1(void *pt) {
  byte * pbLEDPIN;          //定义一个byte类型的指针
  pbLEDPIN = (byte *)pt;    //强制类型转换为byte类型，赋值给pbLEDPIN

  byte LEDPIN;              //定义byte类型的变量
  LEDPIN = *pbLEDPIN;       //将pbLEDPIN指针指向的内容取出，并且赋值给LEDPIN
  pinMode(LEDPIN, OUTPUT);
  while (1) {
    digitalWrite(LEDPIN, !digitalRead(LEDPIN));
    vTaskDelay(1000);
  }
}

void task2(void *pt) {
  byte LEDPIN = *(byte *)pt;        //更加简洁的写法
  pinMode(LEDPIN, OUTPUT);
  while (1) {
    digitalWrite(LEDPIN, !digitalRead(LEDPIN));
    vTaskDelay(3000);
  }
}

void setup() {
  Serial.begin(9600);

  byte * pbLED1PIN;
  pbLED1PIN = &LED1_PIN;        //搞一个指针先指向存储的参数

  void * pvLED1PIN;
  pvLED1PIN = (void *)pbLED1PIN;       //将指针转换为空指针作为task的参数

  if (xTaskCreate(task1,               //该线程要执行的task函数名，其实就是个指针
                  "Blink 23",          //该线程任务名称
                  1024,                 //分配内存空间，任务栈大小
                  pvLED1PIN,            //参数空指针
                  1,                    //优先级
                  NULL)                 //任务句柄
                  == pdPASS)      //
    Serial.println("Task1 Created.");

  if (xTaskCreate(task2,
                  "Blink 21",
                  1024,
                  (void *)&LED2_PIN,        //更加简洁的写法
                  1,
                  NULL) == pdPASS)
    Serial.println("Task2 Created.");
}
void loop() {}
```

例二：

```c++
#include <arduino.h>
// 任务一
// 参数为空指针
void task1(void *pt)
{
  byte num = *(byte *)pt;
  while(1){
    Serial.print("task:");
    Serial.println(num);
    vTaskDelay(1000);
  }
}
// 任务二
void task2(void *pt)
{
  byte num = *(byte *)pt;
  while(1){
    Serial.print("task:");
    Serial.println(num);
    vTaskDelay(5000);
  }
}

void setup(){
  Serial.begin(9600);
  byte task1_num = 1;
  byte task2_num = 2;

  xTaskCreate(task1,"print1",1024,(void *)(&task1_num),1,NULL);
  xTaskCreate(task2,"print1",1024,(void *)(&task2_num),2,NULL);

}
 void loop(){}
```
# 四、通过结构体向一个任务传递多个参数

```c++
#include <arduino.h>

typedef struct {
  int a;
  int b;
} DATA;

void task1(void *pt){
  DATA* data1 = (DATA *)pt;
  while(1){
    // int data1_a = data1->a;
    // int data1_b = data1->b;
    Serial.println("task:");
    Serial.print("DATA_a=");
    Serial.println((data1->a));
    Serial.print("DATA_b=");
    Serial.println((data1->b));
    vTaskDelay(2000);
  }
}
// // // 任务二
// void task2(void *pt){
//   DATA* data2 = (DATA *)pt;
//   int data2_a = data2->a;
//   int data2_b = data2->b;
//   while(1){
//     Serial.println("task2:");
//     Serial.print("DATA_a=");
//     Serial.println(data2_a);
//     Serial.print("DATA_b=");
//     Serial.println(data2_b);
//     vTaskDelay(1000);
//   }
// }

DATA task1_data,task2_data;//注意一定要在这里进行定义。（需要定义为全局变量）

void setup(){
  Serial.begin(9600);

  // DATA task1_data;//要在这里进行定义时，setup函数结束，这一块内存将会被释放！不能在这里
  task1_data.a = 1;//必须要在setup()里面对全局变量赋值
  task1_data.b = 100;

  //DATA task2_data;//要在这里进行定义时，setup函数结束，这一块内存将会被释放！
  task2_data.a = 2;
  task2_data.b = 200;

  //两个任务调用用一个函数
  if (xTaskCreate(task1,"print1",1024,(void *)(&task1_data),1,NULL) == pdPASS)
    Serial.println("task1 Created.");
  if (xTaskCreate(task1,"print2",1024,(void *)(&task2_data),1,NULL) == pdPASS)
    Serial.println("task1 Created.");

  // if (xTaskCreate(task2,"print2",1024,(void *)(&task2_data),1,NULL) == pdPASS);
  // Serial.println("task2 Created.");
}
 void loop(){}

```
## 注意的问题

1、注意全局变量应该定义在所有函数外面。如果定义在setup里面，在setup函数结束之后该块内存将会被释放，
无法在loop或者task里面使用。

2、两个任务可以调用多个函数，但是需要有不同的任务名
3、通过
```c++
  if (xTaskCreate(task1,"print2",1024,(void *)(&task2_data),1,NULL) == pdPASS)
```
来判断任务是否创建成功。

4、必须要在setup()里面对全局变量赋值

## 使用new函数解决了setup()中声明变量被释放的问题
```c++
#include <arduino.h>

typedef struct {
  int a;
  int b;
} DATA;

void task1(void *pt){
  DATA* data1 = (DATA *)pt;
  while(1){
    Serial.println("task:");
    Serial.print("DATA_a=");
    Serial.println((data1->a));
    Serial.print("DATA_b=");
    Serial.println((data1->b));
    vTaskDelay(2000);
  }
}

DATA * task1_data,*task2_data;//注意一定要在这里进行声明。（需要定义为全局变量）

void setup(){
  Serial.begin(9600);
  
  task1_data = new DATA{1,100};//注意这里task1_data是指针
  task2_data = new DATA{2,200};

  //两个任务调用用一个函数
  if (xTaskCreate(task1,"print1",1024,(void *)(task1_data),1,NULL) == pdPASS)
    Serial.println("task1 Created.");
  if (xTaskCreate(task1,"print2",1024,(void *)(task2_data),1,NULL) == pdPASS)
    Serial.println("task1 Created.");
}
 void loop(){}

```
## 使用局部变量解决了setup()中声明变量被释放的问题
```c++
#include <arduino.h>

typedef struct {
  int a;
  int b;
} DATA;

void task1(void *pt){
  DATA* data1 = (DATA *)pt;
  int data1_a = data1->a;
  int data1_b = data1->b;

  while(1){
    //正常打印。
    Serial.println("task:");
    Serial.print("DATA_a=");
    Serial.println((data1_a));
    Serial.print("DATA_b=");
    Serial.println((data1_b));
    Serial.println("--------------------------");
    //下面就会出错
    Serial.println("task:");
    Serial.print("DATA_a=");
    Serial.println((data1->a));
    Serial.print("DATA_b=");
    Serial.println((data1->b));
    Serial.println("--------------------------");


    vTaskDelay(2000);
  }
}

void setup(){
  Serial.begin(9600);
  DATA  task1_data,task2_data; //局部变量setup结束就会被销毁

  task1_data.a = 1;
  task1_data.b = 100;

  task2_data.a = 2;
  task2_data.b = 200;

  //两个任务调用用一个函数
  if (xTaskCreate(task1,"print1",1024,(void *)(&task1_data),1,NULL) == pdPASS)
    Serial.println("task1 Created.");
  if (xTaskCreate(task1,"print2",1024,(void *)(&task2_data),1,NULL) == pdPASS)
    Serial.println("task1 Created.");
}
 void loop(){}
```