```c++
#include <iostream>  
#include <highgui.h>  
  
using namespace std ;  
using namespace cv ;  
  
int main()  
{  
    // Mat is basically a class with two data parts: the matrix header and   
    //a pointer to the matrix containing the pixel values  
    Mat image = imread("1.png" , 0) ;  
      
    //Mat image1(image) ;//仅是创建了Mat的头部分，image1与image共享数据区  
    //Mat image1 = image ;//仅是创建了Mat的头部分，image1与image共享数据区  
    //Mat image1 = image.clone() ;//完全拷贝，把image中的所有信息拷贝到image1中  
    Mat image1 ;  
    image.copyTo(image1) ;//拷贝image的数据区到image1中，在拷贝数据前会有一步：image1.create(this->size , this->type)  
    for(int h = 0 ; h < image1.rows ; ++ h)  
    {  
        uchar* ptr = image1.ptr(h) ;  
        for(int w = 0 ; w < image1.cols ; ++ w)  
        {  
            ptr[w] = 0 ;  
        }  
    }  
    imshow("image" , image) ;  
    imshow("image1" , image1) ;  
    waitKey() ;  
    return 0 ;  
}  

```

在复制图像时，有两种情况，一种是浅拷贝，一种是深拷贝。所谓浅拷贝仅仅是引用，即创建了一个新的矩阵头，仍然指向原来的数据空间。而所谓的深拷贝，是指完全创建一整套新的Mat对象（包括矩阵头和数据空间）。

很显然，（）和 = 操作都属于浅拷贝。clone()和copyto()属于深拷贝，因为它们都会创建一个独立的空间，不会相互影响。但是也有区别
copyto多一种玩法。如下所示，可以通过mask有选择性的复制。
C++: void Mat:: copyTo (OutputArray m, InputArray mask ) const