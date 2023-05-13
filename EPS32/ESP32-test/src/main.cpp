#include<Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_sensor.h>
#include <Wire.h>
#include<math.h>
// #include"Quaternion.h"

Adafruit_MPU6050 mpu;
sensors_event_t a, g, temp;

#define YAW 0
#define PITCH 1
#define ROLL 2

#define X 0
#define Y 1
#define Z 2

double pre_E[3];//陀螺仪积分得到的
double obs_E[3];//加速度计算出来的欧拉角
double post_E[3];//融合，后验
double Euler[3];
unsigned long last_time;
unsigned long current_time;

void observe(sensors_event_t acc);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.setPins(18,19);

  // initialize mpu
  if (!mpu.begin()) {
    Serial.println("Failed to start MPU6050");
  } else {
    Serial.println("Start MPU6050");
  }

  // display MPU6050 settings
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G); //设置加速度计范围
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);  //设置陀螺仪范围
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);//Sets the bandwidth of the Digital Low-Pass Filter.其实对于低通滤波器来说，带宽就是截止频率
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  last_time = millis();
  current_time = millis();

  mpu.getEvent(&a, &g, &temp);
  observe(a);

  Serial.print("angle[Roll]:  ");
  Serial.println(obs_E[ROLL]);
  Serial.print("angle[Pitch]:  ");
  Serial.println(obs_E[PITCH]);
  obs_E[YAW] = 0;
  memcpy(post_E,obs_E,sizeof(obs_E));

}

// 获取数据并且并且打印
void displayReadings() {
  // get new sensor events with the readings
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // print out the values
  // acceleration values
  Serial.print("Acceleration X: ");
  Serial.print(a.acceleration.x);
  Serial.print(", Y: ");
  Serial.print(a.acceleration.y);
  Serial.print(", Z: ");
  Serial.print(a.acceleration.z);
  Serial.println(" m/s^2");

  //rotation values
  Serial.print("Rotation X: ");
  Serial.print(g.gyro.x);
  Serial.print(", Y: ");
  Serial.print(g.gyro.y);
  Serial.print(", Z: ");
  Serial.print(g.gyro.z);
  Serial.println(" rad/s");

  // temperature values
  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println(" degC");
}

// 更新预测值(陀螺仪积分)
void predict(sensors_event_t w, unsigned long delta_ms){
  double delta_s = double(delta_ms)/1000.0;
  pre_E[ROLL] =post_E[ROLL] + ( w.gyro.x + w.gyro.y * sin(post_E[PITCH])*sin(post_E[ROLL])/(post_E[PITCH]) + w.gyro.z * cos(post_E[ROLL])*sin(post_E[PITCH])/cos(post_E[PITCH]))*delta_s;
  pre_E[PITCH] =post_E[PITCH] + ( w.gyro.y * cos(post_E[ROLL]) - w.gyro.z*sin(post_E[ROLL])) * delta_s;
  pre_E[YAW] =post_E[YAW]+( w.gyro.y * sin(post_E[ROLL])/cos(post_E[PITCH]) + w.gyro.z*cos(post_E[ROLL])/cos(post_E[PITCH])) * delta_s;
  
}

//更新观测值(加速度计)
void observe(sensors_event_t acc){
  obs_E[ROLL] = atan2(acc.acceleration.y, acc.acceleration.z);
  obs_E[PITCH] = -atan2(acc.acceleration.x, sqrt(acc.acceleration.y*acc.acceleration.y+acc.acceleration.z*acc.acceleration.z));
}

void loop() {
  // put your main code here, to run repeatedly:
  //displayReadings();

  mpu.getEvent(&a, &g, &temp);

  current_time = millis();
  
  predict(g,current_time-last_time);//预测

  last_time = current_time;

  observe(a);//观测

  //更新(暂时是互补滤波)
  post_E[ROLL] = 0.2*pre_E[ROLL] +0.8*obs_E[ROLL];
  post_E[PITCH] = 0.2*pre_E[PITCH] +0.8*obs_E[PITCH];
  post_E[YAW] = pre_E[YAW];


  
  if(current_time%200 ==0){
    Serial.print("angle_pre[Roll]:  ");
    Serial.println(post_E[ROLL]);
    Serial.print("angle_pre[Pitch]:  ");
    Serial.println(post_E[PITCH]);
    Serial.print("angle_pre[YAW]:  ");
    Serial.println(post_E[YAW]);
  }

  // delay(500);
}
