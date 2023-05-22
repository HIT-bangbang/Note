# 1、Eigen::VectorXd 向量

Eigen::VectorXd默认为列向量；

## （1）初始化（仅适用于维数小于4情况）
```c++
Eigen::Vector2d a(5.0, 6.0);//2维，初始化时直接赋值
Eigen::Vector3d b(5.0, 6.0, 7.0);
Eigen::Vector4d c(5.0, 6.0, 7.0, 8.0);
Eigen::VectorXd x(n);//自定义n维向量（double类型）
x(1) = 2;//赋值
```

## （2）输出与索引：

输出的话，直接写变量名即可，即：

    cout <<  << a << endl;//默认a是列向量,直接输出列向量

    cout <<  << a.transepose() << endl;//以行向量形式输出时需要转置

索引的话，类似matlab那样用小括号即可，即：

    cout <<  << a(0) << a(1) << a(2) << endl;    //因为默认是列向量，故而以行向量形式输出时需要转置

也可以用.x()  .y()的形式索引

    std::cout<< a.x() <<std::endl;//相当于a(0)
    std::cout<< a.y() <<std::endl;//相当于a(1)

## （3）运算：

```c++
Eigen::Vector3d v(1,2,3);
Eigen::Vector3d w(0,1,2);
cout << "Dot product: " << v.dot(w) << endl;       //点积
cout << "Cross product:\n" << v.cross(w) << endl;  //叉积
cout << "Cross product:\n" << v + w << endl;      //相加
```

# 2、Eigen::MatrixXd：矩阵

这里d代表double类型，如果是float类型则应为MatrixXf；

如果是方阵的话，就可以直接Eigen::Matrix2d、Eigen::Matrix3d

## （1）初始化：
```c++
// 对于方阵 
Eigen::Matrix2d mat1;
mat1 << 2,3,2.2,1;//赋值

Eigen::Matrix3f mat2;
m << 1, 2, 3,
     4, 5, 6,
     7, 8, 9;

// 一般矩阵
//形式1
Eigen::MatrixXd mat3(5,2);
mat3 << 2,3,2.2,1,2,3,2,1,2,3;
//形式2 可以指定数据类型
Eigen::Matrix <double, m, n> mat4;   // 自定义m行，n列矩阵

mat(0,1) = 2； //对某个元素赋值

```
## 特殊矩阵
```c++
单位矩阵：Eigen::MatrixXd::Identity(m, n);
全零矩阵：Eigen::MatrixXd::Zero(m, n); 
全一矩阵：Eigen::MatrixXd::Ones(m, n);   
随机矩阵：Eigen::MatrixXd::Random(m, n);

**使用：**
先定义：Eigen::Matrix3d mat;
再赋值：mat =  Eigen::MatrixXd::Identity(m, n);
三维矩阵（定长）：Eigen::Matrix4d mat[10];
索引 mat[2](m,n)

```


## （2）输出与索引：
与Eigen::VectorXd一样，输出的时候可以直接写变量名；索引的时候类似matlab那种小括号表示
```c++
// 输出
cout <<  mat1 << endl << mat2 << endl;
// 索引
mat1(1,1) = 2;
```

## 矩阵运算

```c++
+-*/                     // 加减乘除
mat.inverse();           // 逆矩阵
mat.dot(w);              // 向量mat点积向量w,
mat.cross(w);            // 向量mat叉乘向量w
mat.transpose()          // 转置
mat.norm()               //向量求模，矩阵范数
```

## 访问行 列 块
```c++
vec.size();            // 访问向量长度
mat.rows();            // 访问矩阵行数
mat.cols();            // 访问矩阵列数
mat.block<m,n>(i,j)    //矩阵块操作，从i,j开始，取m×n大小的矩阵
```



# 坐标变换

## （1）平移 Eigen::Translation3d：

```c++
// 赋值
Eigen::Translation3d translation(x,y,z);
```

## （2）旋转 Eigen::Quaternion3d：

运用四元数（quaternion）来描述旋转，构造的时候按照（w,x,y,z）顺序，但在实际内存中，四元数的顺序却是x,y,z,w

```c++
// 可以直接给定数值
Eigen::Quaternion3d quater(0, 0, 1, 0);//(w,x,y,z)

// 也可以分别赋值
Eigen::Quaternion3d quater;
quater.w() = 0;
quater.x() = 0;
quater.y() = 1;
quater.z() = 0;
```

（3）齐次变换矩阵 Eigen::Affine3d：
```c++
//仿射变换矩阵
Eigen::Affine3d affine = translation * quater.toRotationMatrix();
```

# 常用函数

Eigen::Quaterniond::FromTwoVectors 返回两向量之间的转换矩阵

仿射变换矩阵：Eigen::Affine3d mat;

    转换4*4矩阵为仿射矩阵：Eigen::Affine3d mat（T）;
    得到旋转矩阵：mat.linear();
    得到平移矩阵：mat.translation();

创建四元数：Eigen::Quaterniond q(w, x, y, z);

    访问四元数：q.x() q.y() q.z() q.w();
    四元数转向量：q.coeffs();
    四元数转旋转矩阵：q.toRotationMatrix();
    四元数插值：q_ini.slerp(s,q_fin);
