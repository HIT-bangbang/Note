#include<math.h>

class Quaternion{
    public:
    Quaternion(float q1=0.0,float q2=0.0,float q3=0.0,float q4=0.0);
    Quaternion operator*(Quaternion &q);//乘
    Quaternion operator+(Quaternion &q);//加
    Quaternion normal();//取得模
    Quaternion normalize();//四元数单位化
    double* q2e(); 



    private:
        double w;
        double x;
        double y;
        double z;
        // double euler[3];//xyz-rpy


};