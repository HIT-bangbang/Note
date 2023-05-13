# include"Quaternion.h"
# include<math.h>


Quaternion::Quaternion(float q1=0.0,float q2=0.0,float q3=0.0,float q4=0.0){
        this->w = q1;
        this->x = q2;
        this->y = q3;
        this->z = q4;
}
Quaternion Quaternion::operator*(Quaternion &q){
        Quaternion temp;
        temp.w = this->w * q.w - this->x * q.x - this->y * q.y - this->z * q.z;
        temp.x = this->w * q.x - this->x * q.w - this->y * q.z - this->z * q.y;
        temp.y = this->w * q.y - this->x * q.z - this->y * q.w - this->z * q.x;
        temp.z = this->w * q.z - this->x * q.y - this->y * q.x - this->z * q.w;
        return temp;
}

Quaternion Quaternion::operator+(Quaternion &q){
        Quaternion temp;
        temp.w = this->w + q.w;
        temp.x = this->x + q.x;
        temp.y = this->y + q.y;
        temp.z = this->z + q.z;
        return temp;
}

Quaternion Quaternion::normal(){
        double temp = sqrt((this->w,2.0)*((this->w,2.0))+(this->x,2.0)*((this->x,2.0))+(this->y,2.0)*((this->y,2.0))+(this->z,2.0)*((this->z,2.0)));
}

