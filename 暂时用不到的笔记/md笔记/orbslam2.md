该笔记主要参考：

计算机视觉life关于orbslam2的讲解；

https://blog.csdn.net/Darlingqiang/article/details/124520820

https://blog.csdn.net/weixin_43013761/article/details/123092196

# 1. 主程序

## 1.1 变量命名规则 

ORB-SLAM2中的变量遵循的命名规则: 

变量名的第一个字母为`m`表示该变量为某类的成员变量. 

    变量名的第一、二个字母表示数据类型:
        p表示指针类型
        n表示int类型
        b表示bool类型
        s表示std::set类型
        v表示std::vector类型
        l表示std::list类型
        KF表示KeyFrame类型
        t表示变量存储时间
        str文件地址

这种将变量类型写进变量名的命名方法叫做匈牙利命名法.

举例：

以小写字母m（member的首字母）开头的变量表示类的成员变量。比如：

	int mSensor;
	int mTrackingState;
	std::mutex mMutexMode;
对于某些复杂的数据类型，第2个甚至第3个字母也有一定的意义，比如：
mp开头的变量表示指针（pointer）型类成员变量；

```cpp
Tracking* mpTracker;
LocalMapping* mpLocalMapper;
LoopClosing* mpLoopCloser;
Viewer* mpViewer;
```

mb开头的变量表示布尔（bool）型类成员变量；

```cpp
bool mbOnlyTracking;
```

mv开头的变量表示数组（vector）型类成员变量；

```cpp
std::vector<int> mvIniLastMatches;
std::vector<cv::Point3f> mvIniP3D;
```

mpt开头的变量表示指针（pointer）型类成员变量，并且它是一个线程（thread）;

```cpp
std::thread* mptLocalMapping;
std::thread* mptLoopClosing;
std::thread* mptViewer;
```

ml开头的变量表示列表（list）型类成员变量；
mlp开头的变量表示列表（list）型类成员变量，并且它的元素类型是指针（pointer）；
mlb开头的变量表示列表（list）型类成员变量，并且它的元素类型是布尔型（bool）；

```cpp
list<double> mlFrameTimes;
list<bool> mlbLost;
list<cv::Mat> mlRelativeFramePoses;
list<KeyFrame*> mlpReferences;
```

## 1.2 多线程中的锁

为防止多个线程同时操作同一变量造成混乱,引入锁机制:

将[成员函数](https://so.csdn.net/so/search?q=成员函数&spm=1001.2101.3001.7020)本身设为私有变量(`private`或`protected`),并在操作它们的公有函数内加锁. [保护成员扩大的访问范围表现在：基类的保护成员可以在派生类的成员函数中被访问。引入保护成员的理由是：基类的成员本来就是派生类的成员，因此对于那些出于隐藏的目的不宜设为公有，但又确实需要在派生类的成员函数中经常访问的基类成员，将它们设置为保护成员，既能起到隐藏的目的，又避免了派生类成员函数要访问它们时只能间接访问所带来的麻烦.](https://blog.csdn.net/lhh_qrsly/article/details/124937658)
```
class KeyFrame {
protected:
    KeyFrame* mpParent; //
    
public:
    void KeyFrame::ChangeParent(KeyFrame *pKF) {
        unique_lock<mutex> lockCon(mMutexConnections);      // 加锁
        mpParent = pKF;
        pKF->AddChild(this);
    }

    KeyFrame *KeyFrame::GetParent() {
        unique_lock<mutex> lockCon(mMutexConnections);      // 加锁
        return mpParent;
    }
}
```

一把锁在某个时刻只有一个线程能够拿到,如果程序执行到某个需要锁的位置,但是锁被别的线程拿着不释放的话,当前线程就会暂停下来;直到其它线程释放了这个锁,当前线程才能拿走锁并继续向下执行.

- 什么时候加锁和释放锁?

  `unique_lock lockCon(mMutexConnections);`这句话就是加锁,锁的有效性仅限于大括号`{}`之内,也就是说,程序运行出大括号之后就释放锁了.因此可以看到有一些代码中加上了看似莫名其妙的大括号.
```
void KeyFrame::EraseConnection(KeyFrame *pKF) {
    // 第一部分加锁
    {
        unique_lock<mutex> lock(mMutexConnections);
        if (mConnectedKeyFrameWeights.count(pKF)) {
            mConnectedKeyFrameWeights.erase(pKF);
            bUpdate = true;
        }
    }// 程序运行到这里就释放锁,后面的操作不需要抢到锁就能执行
    
    UpdateBestCovisibles();

}
```

## 1.3 main程序

先要找一个入口，以Example中的单目的mono_tum.cc为例，这个.cc相当于整个程序的main函数。

这部分代码主要做了四件事情：
（1）读取图片及时间戳信息
（2）创建SLAM系统
（3）进行SLAM
（4）停止SLAM

（1）读取图片及时间戳信息
通过LoadImages函数完成，该函数把图片的路径+图片名称（string）、时间戳信息（double）分别读入三个vector容器：striFile、vstrImageFilenames、vTimestamps。

（2）创建SLAM系统
使用ORB_SLAM2::System类的构造函数创建SLAM系统，初始化了系统的各个线程，准备好处理输入的帧。

（3）进行SLAM
使用cv::Mat来创建存储图像像素矩阵，使用cv::imread读取图片。
使用了ORB_SLAM2::System中的TrackMonocular函数，输入就是：图片、对应时间戳。

（4）停止SLAM
使用System类中的Shutdown函数停止所有线程。

```
int main(int argc, char **argv)// 这个是主函数
{
    if(argc != 4)
    {
        cerr << endl << "Usage: ./mono_tum path_to_vocabulary path_to_settings path_to_sequence" << endl;
        return 1;
    }
/*argc 是参数的个数，argv[]是参数，argv[0]是可执行文件名，argv[1]是第一个参数…
如你得exe文件名是:myprog.exe，那么
myprog 12 22 32
则argv[0]=”myprog”，argv[1]=”12”，argv[2]=”22”… */

    // Retrieve paths to images
    vector<string> vstrImageFilenames;
    vector<double> vTimestamps;
    string strFile = string(argv[3])+"/rgb.txt";
    LoadImages(strFile, vstrImageFilenames, vTimestamps);
    
// C++ STL中的verctor好比是C语言中的数组，但是vector又具有数组没有的一些高级功能。与数组相比，vector就是一个可以不用再初始化就必须制定大小的边长数组
 
    int nImages = vstrImageFilenames.size();
    
    // Create SLAM system. It initializes all system threads and gets ready to process frames.
    ORB_SLAM2::System SLAM(argv[1],argv[2],ORB_SLAM2::System::MONOCULAR,true);

    // Vector for tracking time statistics
    vector<float> vTimesTrack;
    vTimesTrack.resize(nImages);

    cout << endl << "-------" << endl;
    cout << "Start processing sequence ..." << endl;
    cout << "Images in the sequence: " << nImages << endl << endl;
    
    // Main loop
    cv::Mat im;
    for(int ni=0; ni<nImages; ni++)
    {
        // Read image from file
        im = cv::imread(string(argv[3])+"/"+vstrImageFilenames[ni],CV_LOAD_IMAGE_UNCHANGED);
        double tframe = vTimestamps[ni];
    
        if(im.empty())
        {
            cerr << endl << "Failed to load image at: "
                 << string(argv[3]) << "/" << vstrImageFilenames[ni] << endl;
            return 1;
        }

#ifdef COMPILEDWITHC11
        std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
#else
        std::chrono::monotonic_clock::time_point t1 = std::chrono::monotonic_clock::now();
#endif

        // Pass the image to the SLAM system
        SLAM.TrackMonocular(im,tframe);

#ifdef COMPILEDWITHC11
        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
#else
        std::chrono::monotonic_clock::time_point t2 = std::chrono::monotonic_clock::now();
#endif

        double ttrack= std::chrono::duration_cast<std::chrono::duration<double> >(t2 - t1).count();
    
        vTimesTrack[ni]=ttrack;
    
        // Wait to load the next frame
        double T=0;
        if(ni<nImages-1)
            T = vTimestamps[ni+1]-tframe;
        else if(ni>0)
            T = tframe-vTimestamps[ni-1];
    
        if(ttrack<T)
            usleep((T-ttrack)*1e6);
    }
    
    // Stop all threads
    SLAM.Shutdown();
    
    // Tracking time statistics
    sort(vTimesTrack.begin(),vTimesTrack.end());
    float totaltime = 0;
    for(int ni=0; ni<nImages; ni++)
    {
        totaltime+=vTimesTrack[ni];
    }
    cout << "-------" << endl << endl;
    cout << "median tracking time: " << vTimesTrack[nImages/2] << endl;
    cout << "mean tracking time: " << totaltime/nImages << endl;
    
    // Save camera trajectory
    SLAM.SaveKeyFrameTrajectoryTUM("KeyFrameTrajectory.txt");
    
    return 0;
}

/**
 * @brief 导入图片
 * 
 * @param[in] strFile                   读入的文件名称
 * @param[in&out] vstrImageFilenames    彩色图片名称
 * @param[in&out] vTimestamps           记录时间戳
 */
void LoadImages(const string &strFile, vector<string> &vstrImageFilenames, vector<double> &vTimestamps)
{
    ifstream f;
    f.open(strFile.c_str());

    // skip first three lines
    // 前三行是注释，跳过 
    //getline(f,s); 读取每一行的内容到s里面去
    string s0;
    getline(f,s0);
    getline(f,s0);
    getline(f,s0);

    while(!f.eof())
    {
        string s;
        getline(f,s);
        if(!s.empty())
        {
            stringstream ss;
            ss << s;
            double t;
            string sRGB;
            ss >> t;
            vTimestamps.push_back(t);
            //将strFile文件中的时间戳信息存到vTimestamps中去
            ss >> sRGB;
            vstrImageFilenames.push_back(sRGB);
            //将png图片路径存在vstrImageFilenames中去。
            //从stringstream流中的数据输入字符串到一个变量里，是以遇到空格跳到下一个字符串的这样的形式连续读取的。
        }
    }
}
/*#include <fstream>
ofstream         //文件写操作 内存写入存储设备
ifstream         //文件读操作，存储设备读区到内存中
fstream          //读写操作，对打开的文件可进行读写操作
 c_str()是Borland封装的String类中的一个函数，它返回当前字符串的首字符地址。换种说法，c_str()函数返回一个指向正规C字符串的指针常量，内容与本string串相同。这是为了与C语言兼容，在C语言中没有string类型，故必须通过string类对象的成员函数c_str()把string对象转换成C中的字符串样式。*/
 /* 指针常量的本质是一个常量，并且使用指针来修饰它，那么说明这个常量的值是一个指针，其格式应为：int * const p, 而常量指针本质是指针，并且这个指针乃是一个指向常量的指针。其格式为：int const * p或者 const int* p.
 
EOF，为End Of File的缩写，通常在文本的最后存在此字符表示资料结束。

在文本文件中，数据都是以字符的ASCII代码值的形式存放。我们知道，ASCII代码值的范围是0~127，不可能出现-1，因此可以用EOF作为文件结束标志。

实际上 EOF 的值通常为 -1 */
```

我们把重点代码拆分出来:

```
int main(int argc, char **argv){
	// 从图像文件夹中，找到所有RGB图像路径以及深度图路径
	LoadImages(strFile, vstrImageFilenames, vTimestamps);

    //初始化ORB-SLAM2系统
    ORB_SLAM2::System SLAM(argv[1],argv[2],ORB_SLAM2::System::MONOCULAR,true);

	//对图像序列中的每张图像展开遍历
    for(int ni=0; ni<nImages; ni++)
    	// 根据路径读取RBG图像已经深度图
    	im = cv::imread(string(argv[3])+"/"+vstrImageFilenames[ni],CV_LOAD_IMAGE_UNCHANGED);
    	
     // 根据输入的图像进行追踪
     SLAM.TrackMonocular(im,tframe);
        
	 SLAM.Shutdown(); //系统停止
}
```

是的，你没有看错，其系统架构就是这么简单，就仅仅如下几个过程而已:

```cpp
	1、加载图像数据，配置文件
	2、启动系统，然后for循环进行追踪
	3、系统停止
```

 **---------------==插播==白话来说SVD奇异值分解(1)→原理推导与奇异值求解举例----------------------------------------------------------**

**一、前言**

奇异值分解是一个有着很明显的物理意义的一种方法，它可以将一个比较复杂的矩阵用更小更简单的几个子矩阵的相乘来表示，**这些小矩阵描述的是矩阵的重要的特性。**就像是描述一个人一样，给别人描述说这个人长得浓眉大眼，方脸，络腮胡，而且带个黑框的眼镜，这样寥寥的几个特征，就让别人脑海里面就有一个较为清楚的认识，实际上，人脸上的特征是有着无数种的，之所以能这么描述，是因为人天生就有着非常好的抽取重要特征的能力，让机器学会抽取重要的特征，SVD是也一个重要的方法。在机器学习领域，有相当多的应用与奇异值都可以扯上关系，比如做feature reduction的PCA，做数据压缩（以图像压缩为代表）的算法，还有做搜索引擎语义层次检索的LSI（Latent Semantic  Indexing）。**奇异值分解则是[特征分解](https://baike.baidu.com/item/特征分解/12522621?fromModule=lemma_inlink)在任意矩阵上的推广。特征值分解和奇异值分解两者有着很紧密的关系，特征值分解和奇异值分解的目的都是一样，就是提取出一个矩阵最重要的特征。**这篇博客，主要使用最通俗的语言来讲解SVD奇异值分解，通过该篇博客，将知道 SVD 的来龙去脉，底层原理。同时知道如何利用他去做图片压缩，PCA，求解矩阵(如 Fundamental 矩阵，Homography 矩阵)等。我会详细的讲解 SVD 的每一个细节。由浅到深，由窄到广。那么我们现在就开始吧。

**首先，要明确的是，一个矩阵其实就是一个线性变换**，因为一个矩阵乘以一个向量后得到的向量，其实就相当于将这个向量进行了线性变换。比如说下面的一个矩阵：

  ![img](https://img-blog.csdn.net/20161024222545207)


   它其实对应的线性变换是下面的形式：


  ![img](https://img-blog.csdn.net/20161024223435916)


   因为这个矩阵M乘以一个向量(x,y)的结果是：


  ![img](https://img-blog.csdn.net/20161024223335086)


  上面的矩阵是对称的，所以这个变换是一个对x，y轴的方向一个拉伸变换（每一个对角线上的元素将会对一个维度进行拉伸变换，当值>1时拉长，当值<1时缩短），当矩阵不是对称的时候，假如说矩阵是下面的样子：


  ![img](https://img-blog.csdn.net/20161024223535337)


  它所描述的变换是下面的样子： 

  ![img](https://img-blog.csdn.net/20161024223610996)
 这其实是在平面上对一个轴进行的拉伸变换（如蓝色的箭头所示），在图中，蓝色的箭头是一个最主要的变化方向（变化方向可能有不止一个），如果我们想要描述好一个变换，那我们就描述好这个变换主要的变化方向就好了。反过头来看看之前特征值分解的式子，分解得到的Σ矩阵是一个对角阵，里面的特征值是由大到小排列的，这些特征值所对应的特征向量就是描述这个矩阵变化方向（从主要的变化到次要的变化排列）
 当矩阵是高维的情况下，那么这个矩阵就是高维空间下的一个线性变换，这个线性变化可能没法通过图片来表示，但是可以想象，这个变换也同样有很多的变换方向，我们通过特征值分解得到的前N个特征向量，那么就对应了这个矩阵最主要的N个变化方向。我们利用这前N个变化方向，就可以近似这个矩阵（变换）。也就是之前说的：提取这个矩阵最重要的特征。总结一下，特征值分解可以得到特征值与特征向量，特征值表示的是这个特征到底有多重要，而特征向量表示这个特征是什么，可以将每一个特征向量理解为一个线性的子空间，我们可以利用这些线性的子空间干很多的事情。不过，特征值分解也有很多的局限，比如说变换的矩阵必须是方阵。

**二、奇异值分解简单原理介绍**

在推导数学公式，以及几何意义之前，我们先来看看其物理层面的应用，或者说是一个感性的认识。这样有助于后面更深层次的理解。首先这里有个基本的概念简单说一下, 一个 m × n 维的矩阵，我们可以分解成 m × k 以及 k × n 的矩阵相乘，如下图所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/fb826440d5674acb8ba4eb278d0d39dd.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_17,color_FFFFFF,t_70,g_se,x_16#pic_center)

到了这一步还不够，我们还要继续分解，根据上面的原理，我们是不是可以对矩阵 X m × k 做同样的分解，把他分解成两个矩阵。那么我们现在来做个假设(后面有推导该公式的细节以及计算过程)：
$$
\color{red} A_{m\times n}=U_{m\times m}\Sigma_{m\times n}V^T_{n \times n}                                                 (1)
$$
其下标 m×m,m×n,n×n 分别表示对应形状。大家可能比较奇怪了，把一个矩阵分解成这个样子，有什么作用。如果矩阵 A m × n 使用其上三个矩阵来表示，似乎并没有节省空间。这个先不着急，继续往下分析，如果公式(1)中的具备如下特征：

![在这里插入图片描述](https://img-blog.csdnimg.cn/1370bb09dda54f1794a56e2511117405.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_13,color_FFFFFF,t_70,g_se,x_16#pic_center)

也就是说，对于 m 行 n 列的矩阵 A, 通过SVD分解之后，拆分成了3个子矩阵，其中 U矩阵为 m 行 m 列的方阵， V 为 n 行 n列的方阵， Σ为只有对角线有值的矩阵，其中的值称之为奇异值。看一个例子，原始矩阵如下:

![image-20220916174542943](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916174542943.png)

奇异值分解的结果如下：

![在这里插入图片描述](https://img-blog.csdnimg.cn/6678acb00e44445e90ca8280b7331d9d.png#pic_center)

在奇异值分解中，Σ 矩阵的奇异值是按照从大到小的顺序排列的，而且减少的特别快，经常排名前 10% 的奇异值就占据了全部奇异值 99% 以上的比例。基于这个性质，我们可以只提取前几个奇异值及其对应的矩阵来近似的描述原来的矩阵。

那么我们现在就来做个实验，我们只获取矩阵的如下部分来复原矩阵A:

![在这里插入图片描述](https://img-blog.csdnimg.cn/6678acb00e44445e90ca8280b7331d9d.png#pic_center)

也就是

![image-20220916201148449](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916201148449.png)

这样就是然后我们求得 
$$
A_{4\times 5}=U_{4\times 3}\Sigma_{3\times 3} V^{T}_{3\times 5}
$$
其结果如下：

![image-20220916201316285](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916201316285.png)

可以看到其与(2)式中的 A 4 × 5 结果是完全一致的，完成了完美的复原。为什么这里能完成完美的复原呢? 因为 Σ 矩阵的奇异值表示其对应的**特征 ( 后面有详细讲解 )** 重要性，其因为 Σ 3 × 3  已经包含了 Σ 4 × 5的所有非零元素，所以其是可以完美复原出来的。现在我们来看看，其他的不同矩阵取值，如下图所示：

![   ](https://img-blog.csdnimg.cn/8c4e187244f3468bb7f425a7116e6d01.png#pic_center)也就是

![image-20220916201705504](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916201705504.png)

这样就是然后我们求得 
$$
A_{4\times 5}=U_{4\times 2}\Sigma_{2\times 2} V^{T}_{2\times 5}
$$
 其结果如下：

![image-20220916201827434](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916201827434.png)

这个时候我们与前面 (2) 式 A 4 × 5 相比，可以发现其第1行与第4列的1和2，已经不见了。也就是说明其复原过程中出现了信息丢失。上面的两个实验可以总结如下图所示:

![img](https://img-blog.csdnimg.cn/af7e40465c9448bbbc0bffe00dd256cc.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_17,color_FFFFFF,t_70,g_se,x_16#pic_center)

上图主要表示了如下公式( k 与 r 越接近说明图像还原度越高，当然压缩效果也没有那么明显: 
$$
\tag{8} \color{blue} A_{m \times n}= U_{m \times m} \Sigma_{m \times n} V_{n \times n}^{T} \approx U_{m \times k} \Sigma_{k \times k} V_{k \times n}^{T}
$$
**三、图像压缩**

那么我们如何使用 SVD 来做图像压缩呢? 通过上面的介绍，我们就可以完成图像压缩了。这里我们把一张图像的像素看作前面的矩阵 A m × n ，然后编写代码如下：

```
import cv2
import numpy as np

# 调整该值重复运行代码保存图像

k = 200
img = cv2.imdecode(np.fromfile('./1.png', dtype=np.uint8), 0)
u, w, v = np.linalg.svd(img)
u = u[:, :k]
w = np.diag(w)
w = w[:k, :k]
v = v[:k, :]
img = u.dot(w).dot(v)
cv2.imencode('.jpg', img)[1].tofile('k={}.jpg'.format(k))
```

调整代码中的 k 值，重复保存图像。注意，根据前面的介绍我们可以得知 0 < k < h, k值越大，图像的还原度越高。原图如下：

![请添加图片描述](https://img-blog.csdnimg.cn/f5f22113a51c4173b40d817a1f988993.png#pic_center)

本人调整 k = 100 , k = 50 , k = 20 值保存结果如下：

![image-20220916203421623](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916203421623.png)

我们输入一张图像，通过SVD奇异值分解分解之后，得到 u, w, v 三个矩阵，根据自己的需求，对这三个矩阵进行适当的剪切，代码中的剪切大小通过变量 k k k 控制。我们只需要保存剪切之后的矩阵，就可以复原图像了。注意这里的 w 为对角矩阵，只需要保存对角线的元素即可。

那么我们来计算一下，这张图像压缩多少空间。因为上图中我们可以看到 k = 100 k=100 k=100 的时候，图像基本是没有太大损失的。所以我们按 k = 100来计算。原始图像像素为200x200=40000。压缩之后为 200x100+100+100x100=20000+100+10000=30100。可以看到其压缩了1/4左右的大小，并且基本没有像素损失。

上面我们感性的了解了SVD的原理和其在图像压缩中的应用，下面我们对其求解方法进行介绍。

**四、特征值分解→EVD**

两个矩阵 A  B  相乘, 其为 A 的行与 B 的列，对应相乘相加，所以出现如下公式:
$$
\color{red} A_{m\times n}=U_{m\times m}\Sigma_{m\times n}V^T_{n \times n}                                                 (1)
$$
通过前面介绍我们知道 Σ  为只有对角线有值的矩阵，其中的值称之为奇异值。奇异值表示其对应 **特征** 的重要性。在对其求解方法进行介绍之前，我们需要回顾一下特征值与特征向量,首先其定义如下：
$$
\color{blue} A \vec x=\lambda \vec x
$$
其中 A 为 m × m 的**方阵** (该前置条件)， x ⃗是一个 m 维的列向量，满足上述公式，则我们说 λ 是矩阵 A的一个特征值，而 x ⃗是矩阵 A 的特征值在 λ 所对应的特征向量。为什么可以写成这个样子，主要是因为：

- **矩阵乘法对应了一个变换，是把任意一个向量变成另一个方向或长度都大多不同的新向量。**
- **如果矩阵对某一个向量或某些向量只发生伸缩变换，不对这些向量产生旋转的效果，**
  **那么这些向量就称为这个矩阵的特征向量，伸缩的比例就是特征值。**

如果想稍微了解具体一点的，可以参考一下这篇博客:[特征值与特征向量的意义](https://blog.csdn.net/didi_ya/article/details/109386104)。一个矩阵有多个相互对应的 x ⃗ 与 λ  。那么我们写成一个通用式子:
$$
Axi                                        =λix                                        (10)
$$
一个  m  阶的方阵，那么则有：

![image-20220916211741170](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916211741170.png)

这里我们令:

![image-20220916211811338](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916211811338.png)

则我们可以得到

![image-20220916211831074](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916211831074.png)

进一步推导（由于这里的特征向量两两正交，所以U为正交阵，正交阵的逆矩阵等于其转置-有兴趣的同学可以百度一下正交矩阵）：

![image-20220916211859049](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916211859049.png)

到这里为止，对于方阵的特征分解可以说是完成了。但是这里出现了一个问题，上面的结论是基于 方阵 推导出来的，那么如果 A 并非一个方阵，而是一个任意的矩阵呢? 比如前面提到的  Am×n 或者长方形的图像。那么就无法 EVD 特征分解了。这个时候就轮到 SVD 奇异值分解登场了。
**五、特征值分解→SVD**

通过前面的介绍，我们知道知道一个方阵是可以进行 EVD 特征分解的，经过推导得到公式(13)

![image-20220916212013839](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916212013839.png)

那么一个矩形的矩阵如 Am×n我们如何进行分解呢? 首先我们可以把 A m × n 转换成 m × m  的或者 n × n 矩阵，那么我们就可以使用 EVD 进行分解了，比如 A A T  与 A T A  分别会得到 m × m， n × n 的矩阵。我们回顾一下 SVD 奇异值分解公式：

![image-20220916212115892](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916212115892.png)

**注意**其中 U 是一个 m × m  的矩阵， Σ  是一个 m × n的矩阵，除了主对角线上的元素以外全为 0，主对角线上的每个元素都称为奇异值， V 是一个 n × n 的矩阵。 U 和 V  都是酉矩阵(有兴趣的朋友可以百度一下)，即满足 U T U = E ， V T V = E ，这里的 E 表示单位矩阵，即对角线为1，其余都是0的矩阵 ] 

如果我们将 A 的转置和 A 做矩阵乘法，那么会得到 n × n 的一个方阵 A T A。既然 A T A 是方阵，那么我们就可以进行特征分解，得到的特征值和特征向量满足下式：

![image-20220916212353547](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916212353547.png)

这样我们就可以得到矩阵 A TA的n个特征值和对应的n个特征向量 v ⃗了。将 A TA的所有特征向量张成一个 n × n的矩阵 V，就是我们SVD公式里面的 V 矩阵了。一般我们将 V 中的每个特征向量叫做 A 的右奇异向量。

如果我们将 A和 A 的转置做矩阵乘法，那么会得到 m × m  的一个方阵 A A T  既然是方阵，那么我们就可以进行特征分解，得到的特征值和特征向量满足下式：
![image-20220916213119963](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916213119963.png)

这样我们就可以得到矩阵 A A T 的m个特征值和对应的m个特征向量 u ⃗ 了。将 A A T 的所有特征向量张成一个 m × m 的矩阵 U，就是我们SVD公式里面的 U 矩阵了。一般我们将 U 中的每个特征向量叫做 A 的左奇异向量。

U 和 V 我们都求出来了，现在就剩下奇异值矩阵 Σ  没有求出了。由于 Σ  除了对角线上是奇异值其他位置都是0，那我们只需要求出每个奇异值 σ 就可以了(后面有举例如何求解)。我们注意到:

![image-20220916212616229](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916212616229.png)

其上的 σ i  就是每个特征对应的奇异值，那么进而求出出奇异值矩阵 Σ 。
上面还有一个问题没有讲，就是我们说 A T A A^TA ATA 的特征向量组成SVD中的 V V V 矩阵，而 A A T AA^T AAT 的特征向量组成SVD中的 U U U 矩阵，这有什么根据吗？这个其实很容易证明，我们以V矩阵的证明为例(用到： U T U = I , Σ T Σ = Σ 2 U^{T} U=I)。

![image-20220916213454322](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916213454322.png)

可以看出(参照公式13) ，A T A  的特征向量组成 SVD 中的 V矩阵。类似的方法可以得到 A A T  的特征向量组成 SVD 中的 U 矩阵。进一步我们还可以看出特征值矩阵等于奇异值矩阵的平方，也就是说特征值和奇异值满足如下关系：

![image-20220916213643308](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916213643308.png)

这样也就是说，我们可以不用 σ i = A v i / u i 来计算奇异值，也可以通过求出 A T A 的特征值取平方根来求奇异值。如果大家看到这里来，比较蒙圈了，我们就来看一个例子吧。

**七、SVD 计算举例**

这里我们用一个简单的例子来说明矩阵是如何进行奇异值分解的。我们的矩阵A定义为：

![image-20220916214452702](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916214452702.png)

我们首先求出 A T A ，A A T 

![image-20220916214545576](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916214545576.png)

然后我们还需要求得 A T A 的特征值与特征向量, 先来回顾一下矩阵特征值和特征向量的计算过程： A  为 m 阶矩阵，若数 λ 和 m  维非 0 列向量 x ⃗ 满足 A x ⃗ = λ x ⃗，那么数 λ 称为 A的特征值， x ⃗称为 A 对应于特征值λ的特征向量。式 A x ⃗ = λ x ⃗ 也可写成 ( A − λ E ) x ⃗ = 0，并且 ∣ λ E − A ∣  叫做A 的特征多项式。当特征多项式等于0的时候，称为 A的特征方程，特征方程是一个齐次线性方程组，求解特征值的过程其实就是求解特征方程的解。推导细节如下（其中的 E 为单位矩阵，即对角线为1，其余都为0的矩阵）：

![image-20220916214815743](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916214815743.png)

也就是 ∣ λ E − A ∣ 对应的行列式为0即可。根据上述公式我们可以求解出出 A A T 的特征值与特征：

![image-20220916214939277](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916214939277.png)

利用 σ i = A v i / u i 其中 i = 1 , 2 i=1,2 i=1,2 求奇异值的：

![image-20220916215140389](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220916215140389.png)

**八、结语**

通过该篇博客，我们知道了如何对一个矩阵进行 SVD 奇异值分解，并且列举了一个图像压缩的例子。但是这仅仅其中的一部分应用，我们还可以用来求解超定方程 A x = 0 (最优解)。 当然，该博客的篇幅已经很长了，所以令起一篇博客继续为大家介绍如何求解超定方程，为什么最小奇异值对应的特征值，为超定方程的解。

**---------------==插播==白话来说SVD奇异值分解(1)→原理推导与奇异值求解举例==结束==--------------------------------------------**

**----------------------------==插播==白话来说SVD奇异值分解(2)→超定方程求解,最小奇异值特征为最优解--------------------------------**

**一、前言**

弄明白矩阵的特征向量，特征值，EVD(特征分解)，SVD(奇异值分解)等相关知识之后，就可以思考接下来的问题了，为什么最小奇异值特征可以作为超定方程的最优解？

**二、适定、欠定、超定方程**

在工程上，很多问题最终都会转换成
$$
A_{(m,n)}\vec x_{(n,1)}=\vec b_{(m,1)}(1)
$$
方程的求解。其中 A ( m , n ) 表示 m × n  的矩阵； x ( n , 1 ) 表示 n 个元素的列向量； b ( m , 1 )表示 m 个元素的列向量。当然 x ( n , 1 ) 与 b ( m , 1 ) 也可当作矩阵来看待。

- `若方程(1)至少有一个精确解，称为一致方程。`
- `若方程(1)无任何精确解，称为非一致方程。`

根据矩阵 A ( m , n ) 秩的大小，矩阵方程又可以分成以下三种类型：

- (1)适定方程→: 若 m = n, 并且 r a n k ( A ) = n , 即矩阵 A 非奇异, 则称矩阵方程 A x ⃗ = b ⃗ 为适定 (well-determined) 方程。

- (2)欠定方程→: 若独立的方程个数小于独立的末知参数个数, 则称矩阵方程 A x ⃗ = b ⃗ 为欠定 (under-determined) 方程。

- (3)超定方程→: 若独立的方程个数大于独立的末知参数个数, 则称矩阵方程 A x ⃗ = b ⃗ 为超定 (over-determined) 方程。

  下面是术语 “适定”、“欠定” 和 “超定” 的含义。

**适定的双层含义** 方程组的解是唯一的; 独立的方程个数与独立末知参数的个数相同, 正好可以唯一地确定该方程组的解。适定方程 A x ⃗ = b ⃗ 的唯一解由 x ⃗ = A − 1 b ⃗ 给出。 适定方程为一致方程。

**欠定的含义** 独立的方程个数比独立的末知参数的个数少, 意味着方程个数不足于确定方程组的唯一解。事实上, 这样的方程组存在无穷多组解 x ⃗ 。欠定方程为一致方程。

**超定的含义** 独立的方程个数超过独立的末知参数的个数, 对于确定方程组的唯一解显得方程过剩。因此, 超定方程 A x ⃗ = b ⃗没有使得方程组严格满足的精确解 x ⃗ 。超定方程为非一致方程。

**三、超定方程求解**

在计算机视觉或者说 slam 中，经常遇到超定方程求解的情形。比如三角化地图点，pnp，以及 Fundamental 与 Homography 矩阵的求解。那么我们就来介绍一下 超定方程的求解。通过前面的介绍，**我们已经知道超定方程没有精确解的，那么只能去求他的最优解**。这个时候我们就需要**引入最小二乘法了**(关于最小二乘法的相关知识大家可以百度一下)。

------

 **插播最小二乘法的介绍**

**定义**：最小二乘法（又称最小平方法）是一种数学[优化](https://baike.baidu.com/item/优化?fromModule=lemma_inlink)技术。它**通过最小化误差的平方和寻找数据的最佳[函数](https://baike.baidu.com/item/函数/301912?fromModule=lemma_inlink)匹配**（例如数据(1,2),(2,4),(3,6)的最佳函数匹配是y=2x）。利用最小二乘法可以简便地求得未知的数据，并使得这些求得的数据与实际数据之间误差的平方和为最小 。最小二乘法还可用于[曲线拟合](https://baike.baidu.com/item/曲线拟合/5893992?fromModule=lemma_inlink)，其他一些优化问题也可通过最小化能量或最大化熵用最小二乘法来表达。

**基本思路**：

​		最小二乘法是解决曲线拟合问题最常用的方法。其基本思路是：令

##### ![img](https://bkimg.cdn.bcebos.com/formula/48e7bd3cffb2f0fc474a42e72f6c3432.svg)

其中，![img](https://bkimg.cdn.bcebos.com/formula/d7c9a7d2b8f93e9cffcf76a51e33b4e6.svg) 是事先选定的一组线性无关的函数，![img](https://bkimg.cdn.bcebos.com/formula/bc541226c79a0b278302e563767aac15.svg) 是待定系数![img](https://bkimg.cdn.bcebos.com/formula/ae9e4324d78020bdcc51011a92776331.svg) ，拟合准则是使![img](https://bkimg.cdn.bcebos.com/formula/b126a95f2d0e40931bbdf76be8ffee9e.svg) 与![img](https://bkimg.cdn.bcebos.com/formula/c71970b5ee6d4054e765801fba11ecf0.svg)的距离![img](https://bkimg.cdn.bcebos.com/formula/450133ba6f3eb6c4517c0324c6689494.svg)的平方和最小，称为[最小二乘准则](https://baike.baidu.com/item/最小二乘准则/19137702?fromModule=lemma_inlink) [8] 。

**基本原理**

设(x,y)是一对观测量，且![img](https://bkimg.cdn.bcebos.com/formula/4d455d0e961486d92cacddf3881c3db4.svg)满足以下的理论函数 [9] ：

![img](https://bkimg.cdn.bcebos.com/formula/83bc1067bc2805f96300d7fe86a5a73e.svg)

其中![img](https://bkimg.cdn.bcebos.com/formula/1914a103c51bc1551adfd87f82b0dae2.svg) 为待定参数 。

为了寻找函数![img](https://bkimg.cdn.bcebos.com/formula/1fc8cbc77f6a754c26e0cff72a696a0a.svg)的参数![img](https://bkimg.cdn.bcebos.com/formula/0e4d68e44abb832aee6a24ef62a5fdb1.svg)的最优估计值，对于给定![img](https://bkimg.cdn.bcebos.com/formula/4f88ebed045531c75bb193bac407450c.svg)组（通常![img](https://bkimg.cdn.bcebos.com/formula/3ee219d4dd268ec0f8b970eb74a93cbc.svg)）观测数据![img](https://bkimg.cdn.bcebos.com/formula/ff9c0ee23bbd7706a68df7d62a7f7c50.svg)，求解目标函数

![img](https://bkimg.cdn.bcebos.com/formula/34734d73c8b627846467333de63e6e6a.svg)

取最小值的参数![img](https://bkimg.cdn.bcebos.com/formula/5aed3faf02831299e6fd089ea524c1ef.svg)。求解的这类问题称为[最小二乘问题](https://baike.baidu.com/item/最小二乘问题/19128644?fromModule=lemma_inlink)，求解该问题的方法的几何语言称为最小二乘拟合 [9] 。

**对于无约束[最优化问题](https://baike.baidu.com/item/最优化问题/9632567?fromModule=lemma_inlink)，最小二乘法的一般形式为：**

![img](https://bkimg.cdn.bcebos.com/formula/7a336d111315f189a8f8e86622b45ffe.svg)

其中![img](https://bkimg.cdn.bcebos.com/formula/fa9efc58ef79c35fa17f422aa1c7d79a.svg)称为残差函数。当![img](https://bkimg.cdn.bcebos.com/formula/fa9efc58ef79c35fa17f422aa1c7d79a.svg)是![img](https://bkimg.cdn.bcebos.com/formula/4675185d7b4028ecceae6b1456352cd4.svg)的线性函数时，称为线性最小二乘问题，否则称为非线性最小二乘问题。

**最小二乘优化问题**

在无约束最优化问题中，有些重要的特殊情形，比如目标函数由若干个函数的平方和构成，这类函数一般可以写成 [8] ：

![img](https://bkimg.cdn.bcebos.com/formula/a5f8cfb89fc3d37e1840b13722ade356.svg)

其中![img](https://bkimg.cdn.bcebos.com/formula/15f68d8cc279ca71a9fc608e4d58629e.svg) ，通常要求m≥n，我们把极小化这类函数的问题 [8] ：

![img](https://bkimg.cdn.bcebos.com/formula/3a2eec20b79e7235c3b251a40ce39056.svg)

称为最小二乘优化问题。最小二乘优化是一类比较特殊的优化问题 [8] 。

------

我们有了最小二乘法的基本概念和求解一般形式，我们继续介绍超定方程的求解：
$$
A_{(m,n)}\vec x_{(n,1)}=\vec b_{(m,1)}(1)
$$
这里我们先讨论一种情况，即 b ⃗ ( m , 1 ) = 0 , 也就是求解如下超定方程(m>n,也就是行大于列)：
$$
A_{(m,n)}\vec x_{(n,1)}=0(2)
$$
很显然，上述公式中存在一个0解，但是我们工程实际应用中都是需要求非零解，为了求非零解，我们对 A 加上一个约束 ∣ ∣ x ⃗ ∣ ∣ 2 = 1。也就是限制 x ⃗ 的长度为1，并构建成一个带约束的最小二乘问题：
$$
\hat{{x}}=\arg \min \|{A} {\vec x}\|^{2}, \text { subject to }\|{\vec x}\|^{2}=1(3)
$$
这是一个带约束的最小二乘问题，我们把拉格朗日搬出来：

![image-20220917102713159](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220917102713159.png)

为了求极值，我们分别对 A 和 λ 求偏导数，令为0:

![image-20220917102749422](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220917102749422.png)

把(5)式整理一下：

![image-20220917102817966](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220917102817966.png)

看到其上的公式(8), 根据上一篇博客我们讲解的内容，可以看出 λ 和 x ⃗ 分别是 A T A  的特征值和特征向量。也就是说超定方程 A ( m , n ) x ⃗ ( n , 1 ) = 0的解就是 A T A 对应的特征向量，那么问题来了。这么多个特征向量，我们应该选择那一个呢?我们展开 ∥ A x ∥ 2  看一下(利用(3)式中的 ∥ x ∥ 2 = 1 )：
![image-20220917102949906](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220917102949906.png)

也就是说， 每一个奇异值λ都是一个残差项∥ A x ∥ 2，我们想要 ∥ A x ∥ 2 最小，就需要 λ最小。对于 SVD 奇异值分解公式如下(上一篇博客有推导)

![image-20220917103149928](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220917103149928.png)

其上的 Σ m × n 是对角矩阵，对角线元素称为奇异值，一般来说奇异值是按照从大到小的顺序降序排列。因为每一个奇异值都是一个残差项，因此最后一个奇异值最小，其含义是最优的残差。因此其对用的奇异值向量就是最优解。

在结束之前我们再补充下拉格朗日乘子法的知识：

[算法学习—拉格朗日乘子法](https://www.cnblogs.com/robohou/p/13511714.html)

**1）最小二乘法——求方差的平方和为极小值时的参数。**

要尽全力让这条直线最接近这些点，那么问题来了，怎么才叫做最接近呢？直觉告诉我们，这条直线在所有数据点中间穿过，让这些点到这条直线的误差之和越小越好。这里我们用**方差**来算更客观。也就是说，把每个点到直线的误差平方加起来；接下来的问题就是，如何让这个S变得最小。这里有一个概念，就是求偏导数，通过偏导求方差极小值。比如导数就是求变化率，而偏导数则是当变量超过一个的时候，对其中一个变量求变化率。我们都是在做不到完美的情况下，求那个最接近完美的解。

**2）朗格朗日乘子法——求带有拉格朗日算子的目标复合函数极小值时的参数**

举个例子：某工厂在生产过程中用到两类原材料，其中一种单价为2万/公斤，另一种为3万/公斤，而工厂每个月预算刚好是6万。就像下面的公式：

![img](https://img2020.cnblogs.com/blog/721894/202008/721894-20200816102520868-2132145223.png)

经过分析，工厂的产量f跟两种原材料（x1，x2）具有如下关系（我们暂且不管它是如何来的，而且假定产品可以按任意比例生产）

![img](https://img2020.cnblogs.com/blog/721894/202008/721894-20200816102552921-338418293.png)

请问该工厂每个月最少能生产多少？

其实现实生活中我们会经常遇到类似的问题：在某个或某几个限制条件存在的情况下，求另一个函数的极值（极大或极小值）。就好比你要在北京买房，肯定不是想买什么房子就买什么房子，想买多大就买多大，而是跟你手头的金额，是否有北京户口，纳税有没有满五年，家庭开支/负担重不重，工作单位稳不稳定都有关系。

回到工厂的例子，其实就是求函数f的极值。上面我们提到，极值点可以通过求偏导（变化率为0的地方为极值点）来实现，函数f（x1，x2）对x1，x2分别求偏导，那么得出的结论是：x1，x2都为0的时候最小，单独看这个函数，这个结论对的，很显然这个函数的最小值是0（任何数的平方都是大于或等于0），而且只有x1和x2同时为0的时候，取得最小值。但问题是它不满足上面的限制条件。

怎么办呢？拉格朗日想到了一个很妙的办法，既然h（x1，x2）为0，那函数f（x1，x2）是否可以加上这个h（x1，x2）再乘以一个系数呢？任何数乘以0当然是0，f（x1，x2）加上0当然保持不变。所以其实就可以等同于求下面这个函数的极值：

![img](https://img2020.cnblogs.com/blog/721894/202008/721894-20200816102711019-535905523.png)

我们对x1，x2以及λ分别求偏导（极值点就是偏导数均为0的点）

 

![img](https://img2020.cnblogs.com/blog/721894/202008/721894-20200816102739033-1119230221.png)

 

解上面的方程组得到x1=1.071，x2=1.286 然后代入f（x1，x2）即可。

这里为什么要多加一个乘子λ呢，试想一下，如果λ是个固定的数（比如-1），我们也能通过上面的方程式1,2求解得到x1，x2，但是我们就得不到方程式3，其实也就是没有约束条件了。**所以看到没有，拉格朗日很聪明，他希望我们在求偏导（极值点）以后，还能保留原有的约束条件。我们上面提到，单独对函数求极值不能保证满足约束条件，拉格朗日这么一搞，就能把约束条件带进来，跟求其他变量的偏导结果放在一起，既能满足约束条件，又能保证是约束条件下的极值。借用金星的一句话：完美！**

当然这是一个约束条件的情况，如果有多个约束条件呢？那就要用多个不同的λ（想想为什么），正如最上面的那个定义那样，把这些加起来（这些0加起来也是0）。

摘自：https://www.zhihu.com/question/36324957/answer/255970074

**----------------------------==插播==白话来说SVD奇异值分解(2)→超定方程求解,最小奇异值特征为最优解==结束==--------------------------------**



# 2. 类构造函数::System()

## sytem类的基本介绍-system.h

System.h中包含了七个类，分别是Viewer，FrameDrawer，Map，Tracking，LocalMapping，LoopClosing的声明，和System类的定义, 这些类组成了orbslam2系统。

![img](https://img2020.cnblogs.com/blog/2025950/202006/2025950-20200618173459657-816915285.png)
----------------------------------------------------------------------插播介绍enum枚举----------------------------------------------------------------------

C enum(枚举)

枚举是 C 语言中的一种基本数据类型，它可以让数据更简洁，更易读。

枚举语法定义格式为：

```
enum　枚举名　{枚举元素1,枚举元素2,……};
```

接下来我们举个例子，比如：一星期有 7 天，如果不用枚举，我们需要使用 #define 来为每个整数定义一个别名：

\#define MON  1 #define TUE  2 #define WED  3 #define THU  4 #define FRI  5 #define SAT  6 #define SUN  7

这个看起来代码量就比较多，接下来我们看看使用枚举的方式：

```
enum DAY
{
      MON=1, TUE, WED, THU, FRI, SAT, SUN
};
```

这样看起来是不是更简洁了。

**注意：**第一个枚举成员的默认值为整型的 0，后续枚举成员的值在前一个成员上加 1。我们在这个实例中把第一个枚举成员的值定义为 1，第二个就为 2，以此类推。

> 可以在定义枚举类型时改变枚举元素的值：
>
> ```
> enum season {spring, summer=3, autumn, winter};
> ```
>
> 没有指定值的枚举元素，其值为前一元素加 1。也就说 spring 的值为 0，summer 的值为 3，autumn 的值为 4，winter 的值为 5

枚举变量的定义

前面我们只是声明了枚举类型，接下来我们看看如何定义枚举变量。

我们可以通过以下三种方式来定义枚举变量

**1、先定义枚举类型，再定义枚举变量**

```
enum DAY
{
      MON=1, TUE, WED, THU, FRI, SAT, SUN
};
enum DAY day;
```

**2、定义枚举类型的同时定义枚举变量**

```
enum DAY
{
      MON=1, TUE, WED, THU, FRI, SAT, SUN
} day;
```

**3、省略枚举名称，直接定义枚举变量**

```
enum
{
      MON=1, TUE, WED, THU, FRI, SAT, SUN
} day;
```

实例

\#include <stdio.h>  enum DAY {      MON=1, TUE, WED, THU, FRI, SAT, SUN };  int main() {    enum DAY day;    day = WED;    printf("%d",day);    return 0; }

以上实例输出结果为：

```
3
```

----------------------------------------------------------------------插播介绍enum枚举结束-----------------------------------------------------------------------------------------------------

**System.h源程序**：

```text
namespace ORB_SLAM2
{

class Viewer;
class FrameDrawer;
class Map;
class Tracking;
class LocalMapping;
class LoopClosing;

class System //定义System类
{
public:
    // Input sensor 输入传感器类型
    enum eSensor{ // 采用枚举型确定传感器类型
        MONOCULAR=0,
        STEREO=1,
        RGBD=2
    };

public:

    // Initialize the SLAM system. It launches the Local Mapping, Loop Closing and Viewer threads.
    // 初始化SLAM系统，将启动局部建图、回环以及可视化线程
    // 可以看到构造函数中，构建SLAM系统需要输入词典路径、YAML配置文件路径、设置传感器类型，是否启用viewer线程。
    // ORB_SLAM2::System SLAM(
    //    argv[1],                            // path_to_vocabulary
    //    argv[2],                            // path_to_settings
    //    ORB_SLAM2::System::MONOCULAR,       // 单目模式
    //    true);                              // 启用可视化查看器
    // 参数对应，构建系统。const eSensor sensor=ORB_SLAM2::System::MONOCULAR，const bool bUseViewer：是否可视化，true使用。
	System(const string &strVocFile, const string &strSettingsFile, const eSensor sensor, const bool bUseViewer = true);
	
    // 以下依次构建了双目、RGB、单目的图像跟踪矩阵
    // Proccess the given stereo frame. Images must be synchronized and rectified.
    // Input images: RGB (CV_8UC3) or grayscale (CV_8U). RGB is converted to grayscale.
    // Returns the camera pose (empty if tracking fails).
    cv::Mat TrackStereo(const cv::Mat &imLeft, const cv::Mat &imRight, const double &timestamp);

    // Process the given rgbd frame. Depthmap must be registered to the RGB frame.
    // Input image: RGB (CV_8UC3) or grayscale (CV_8U). RGB is converted to grayscale.
    // Input depthmap: Float (CV_32F).
    // Returns the camera pose (empty if tracking fails).
    cv::Mat TrackRGBD(const cv::Mat &im, const cv::Mat &depthmap, const double &timestamp);

    // Proccess the given monocular frame
    // Input images: RGB (CV_8UC3) or grayscale (CV_8U). RGB is converted to grayscale.
    // Returns the camera pose (empty if tracking fails).
    cv::Mat TrackMonocular(const cv::Mat &im, const double &timestamp);

    // This stops local mapping thread (map building) and performs only camera tracking.
    // 使能定位模式，即停止局部建图线程，仅进行相机跟踪
    void ActivateLocalizationMode();
    // This resumes local mapping thread and performs SLAM again.
    // 关闭定位模式，即继续局部建图线程，并又运行SLAM系统
    void DeactivateLocalizationMode();

    // Returns true if there have been a big map change (loop closure, global BA)
    // since last call to this function
    // 判断图像是否有重大改变
    bool MapChanged();

    // Reset the system (clear map) // 复位系统，清理地图
    void Reset();

    // All threads will be requested to finish.
    // It waits until all threads have finished.
    // This function must be called before saving the trajectory.
    // 关闭系统
    void Shutdown();

    // Save camera trajectory in the TUM RGB-D dataset format.
    // Only for stereo and RGB-D. This method does not work for monocular.
    // Call first Shutdown()
    // See format details at: http://vision.in.tum.de/data/datasets/rgbd-dataset
    // 存储TUM格式的相机轨迹到指定路径下
    void SaveTrajectoryTUM(const string &filename);

    // Save keyframe poses in the TUM RGB-D dataset format.
    // This method works for all sensor input.
    // Call first Shutdown()
    // See format details at: http://vision.in.tum.de/data/datasets/rgbd-dataset
    // 仅存储TUM格式的关键帧轨迹到指定路径下
    // 为什么还需要存储关键帧轨迹？个人认为：关键帧轨迹没有冗余信息干扰，这样
    void SaveKeyFrameTrajectoryTUM(const string &filename);

    // Save camera trajectory in the KITTI dataset format.
    // Only for stereo and RGB-D. This method does not work for monocular.
    // Call first Shutdown()
    // See format details at: http://www.cvlibs.net/datasets/kitti/eval_odometry.php
    // 存储KITTI格式的相机轨迹到指定路径下
    void SaveTrajectoryKITTI(const string &filename);

    // TODO: Save/Load functions
    // SaveMap(const string &filename);
    // LoadMap(const string &filename);

    // Information from most recent processed frame
    // You can call this right after TrackMonocular (or stereo or RGBD)
    int GetTrackingState();
    std::vector<MapPoint*> GetTrackedMapPoints();
    std::vector<cv::KeyPoint> GetTrackedKeyPointsUn();

private:

    // 以下都是必要的变量声明
    // Input sensor 传感器类型
    eSensor mSensor;

    // ORB vocabulary used for place recognition and feature matching.
    // ORB词典指针，用于位置识别和特征匹配
    ORBVocabulary* mpVocabulary;

    // KeyFrame database for place recognition (relocalization and loop detection).
    // 关键帧数据库指针，用于位置识别（重定位和回环检测）
    KeyFrameDatabase* mpKeyFrameDatabase;

    // Map structure that stores the pointers to all KeyFrames and MapPoints.
    // 地图指针，该地图结构存储所有指向关键帧和地图点的指针
    Map* mpMap;

    // Tracker. It receives a frame and computes the associated camera pose.
    // 跟踪器，接收每幅图像帧，并计算其关联的相机位姿
    // It also decides when to insert a new keyframe, create some new MapPoints and
    // performs relocalization if tracking fails.
    // 跟踪器同时还决定新的关键帧插入时间；并创建一些新的地图点；如果跟踪失败就进行重定位
    Tracking* mpTracker;

    // Local Mapper. It manages the local map and performs local bundle adjustment.
    // 指向局部地图的指针，管理局部地图并完成局部BA。
    LocalMapping* mpLocalMapper;

    // Loop Closer. It searches loops with every new keyframe. If there is a loop it performs
    // a pose graph optimization and full bundle adjustment (in a new thread) afterwards.
    // 回环器，对每一帧新的关键帧都检测是否发生回环。如果回环发生，就进行位姿优化，并在新的线程里进行全部BA
    LoopClosing* mpLoopCloser;

    // The viewer draws the map and the current camera pose. It uses Pangolin.
    // 可视化器指针，绘制地图和当前的相机位姿，用到了Pangolin模块
    Viewer* mpViewer;

    FrameDrawer* mpFrameDrawer; // 图像帧绘制器
    MapDrawer* mpMapDrawer; //地图绘制器

    // System threads: Local Mapping, Loop Closing, Viewer.
    // The Tracking thread "lives" in the main execution thread that creates the System object.
    // 创建了系统目标的跟踪线程是主要线程
    std::thread* mptLocalMapping;
    std::thread* mptLoopClosing;
    std::thread* mptViewer;

    // Reset flag 复位标志
    std::mutex mMutexReset;
    bool mbReset;

    // Change mode flags 模式改变标志
    std::mutex mMutexMode;
    bool mbActivateLocalizationMode;
    bool mbDeactivateLocalizationMode;

    // Tracking state 跟踪状态
    int mTrackingState;
    std::vector<MapPoint*> mTrackedMapPoints;
    std::vector<cv::KeyPoint> mTrackedKeyPointsUn;
    std::mutex mMutexState;
};

}// namespace ORB_SLAM

#endif // SYSTEM_H
```

**重要变量**

```ORBVocabulary* mpVocabulary;
    ORBVocabulary* mpVocabulary             //ORB字典,保存ORB描述子聚类结果
    KeyFrameDatabase* mpKeyFrameDatabase;   //关键帧数据库,保存ORB描述子倒排索引
    Map* mpMap;                             //地图     
    Tracking* mpTracker;                    //追踪器
    LocalMapping* mpLocalMapper;            //局部建图器 
    LoopClosing* mpLoopCloser;              //回环检测器
    Viewer* mpViewer;                       //可视化器
    FrameDrawer* mpFrameDrawer;             //帧绘制器
    MapDrawer* mpMapDrawer;                 //地图绘制器
```

**启动线程：**

```
    std::thread* mptLocalMapping;            //局部建图线程
    std::thread* mptLoopClosing;            //回环检测线程
    std::thread* mptViewer;                 //可视化线程
```

## ORB_SLAM2::System构造函数

**前言**

下面我们看下src/System.cc中的ORB_SLAM2::System构造函数，mono_tum.cc中main函数里有创建System对象的语句：

```
ORB_SLAM2::System SLAM(argv[1],argv[2],ORB_SLAM2::System::MONOCULAR,true);
```

ORB_SLAM2::System() 构造函数是非常重要的。并且其中的函数：

    cv::Mat TrackMonocular(const cv::Mat &im, const double &timestamp);

是当中核心的核心，所以我们必须去了解 ORB_SLAM2::System 的实现以及构建过程，针对于 ORB_SLAM2::System 的[构造函数](https://so.csdn.net/so/search?q=构造函数&spm=1001.2101.3001.7020)主要流程如下:

- 加载ORB词汇表，构建Vocabulary，以及关键帧数据集库；

- 初始化追踪主线程，但是未运行；

- 初始化局部建图，回环闭合线程，且运行；

- 创建可视化线程，并且与追踪主线程关联起来。

  `其中初始化指的是构造了XX线程运行需要的XX器，也就是XX线程运行需要的XX载体，例如，初始化追踪线程指构造了追踪线程运行需要的追踪器载体;`

  `运行是指构造了XX线程，并将该线程在对应的XX载体上运行；`

  `创建指的是初始化该线程且运行。`

 **src/System.cc** 

首先我们找到工程下面的 src/System.cc 文件，并且找到其中的构建函数 System::System()，其代码注释如下:

```//系统的构造函数，将会启动其他的线程
System::System(const string &strVocFile,					//词典文件路径
			   const string &strSettingsFile,				//配置文件路径
			   const eSensor sensor,						//传感器类型
               const bool bUseViewer):						//是否使用可视化界面
					 mSensor(sensor), 							//初始化传感器类型
					 mpViewer(static_cast<Viewer*>(NULL)),		//空。。。对象指针？  TODO 
					 mbReset(false),							//无复位标志
					 mbActivateLocalizationMode(false),			//没有这个模式转换标志
        			 mbDeactivateLocalizationMode(false)		//没有这个模式转换标志
{
    // Output welcome message
    cout << endl <<
    "ORB-SLAM2 Copyright (C) 2014-2016 Raul Mur-Artal, University of Zaragoza." << endl <<
    "This program comes with ABSOLUTELY NO WARRANTY;" << endl  <<
    "This is free software, and you are welcome to redistribute it" << endl <<
    "under certain conditions. See LICENSE.txt." << endl << endl;

    // 输出当前传感器类型
    cout << "Input sensor was set to: ";
    
    if(mSensor==MONOCULAR)
        cout << "Monocular" << endl;
    else if(mSensor==STEREO)
        cout << "Stereo" << endl;
    else if(mSensor==RGBD)
        cout << "RGB-D" << endl;
    
    //Check settings file
    cv::FileStorage fsSettings(strSettingsFile.c_str(), 	//将配置文件名转换成为字符串
    						   cv::FileStorage::READ);		//只读
    //如果打开失败，就输出调试信息
    if(!fsSettings.isOpened())
    {
       cerr << "Failed to open settings file at: " << strSettingsFile << endl;
       //然后退出
       exit(-1);
    }
    
    //Load ORB Vocabulary
    cout << endl << "Loading ORB Vocabulary. This could take a while..." << endl;
    
    //建立一个新的ORB字典
    mpVocabulary = new ORBVocabulary();
    //获取字典加载状态
    bool bVocLoad = mpVocabulary->loadFromTextFile(strVocFile);
    //如果加载失败，就输出调试信息
    if(!bVocLoad)
    {
        cerr << "Wrong path to vocabulary. " << endl;
        cerr << "Falied to open at: " << strVocFile << endl;
        //然后退出
        exit(-1);
    }
    //否则则说明加载成功
    cout << "Vocabulary loaded!" << endl << endl;
    
    //Create KeyFrame Database
    mpKeyFrameDatabase = new KeyFrameDatabase(*mpVocabulary);
    
    //Create the Map
    mpMap = new Map();
    
    //Create Drawers. These are used by the Viewer
    //这里的帧绘制器和地图绘制器将会被可视化的Viewer所使用
    mpFrameDrawer = new FrameDrawer(mpMap);
    mpMapDrawer = new MapDrawer(mpMap, strSettingsFile);
    
    //初始化追踪线程，构造了追踪线程需要的追踪器mpTracker = new Tracking
    //Initialize the Tracking thread
    //(it will live in the main thread of execution, the one that called this constructor)
    mpTracker = new Tracking(this,						//现在还不是很明白为什么这里还需要一个this指针  TODO 
    						 mpVocabulary,				//字典
    						 mpFrameDrawer, 			//帧绘制器
    						 mpMapDrawer,				//地图绘制器
                             mpMap, 					//地图
                             mpKeyFrameDatabase, 		//关键帧地图
                             strSettingsFile, 			//设置文件路径
                             mSensor);					//传感器类型
    
//初始化并运行局部建图线程new LocalMapping & new thread
//Initialize the Local Mapping thread and launch
//初始化局部建图线程，构造了建图需要的局部建图器mpLocalMapper = new LocalMapping
mpLocalMapper = new LocalMapping(mpMap, 				// 指定地图指针，该地图结构存储所有指向关键帧和地图点的指针                                                   // Map structure that stores the pointers to all KeyFrames and MapPoints.                            // 称为指定存储所有指向关键帧和地图点指针的地图指针
								 mSensor==MONOCULAR);	// 判断mSensor是不是MONOCULAR
//构造并运行局部建图线程
mptLocalMapping = new thread(&ORB_SLAM2::LocalMapping::Run,	//这个线程会调用的函数
							 mpLocalMapper);				//这个调用函数的参数(调用局部建图器)
//初始化并运行回环检测线程
//Initialize the Loop Closing thread and launch
//构造了回环检测需要的回环检测器mpLoopCloser = new LoopClosing
mpLoopCloser = new LoopClosing(mpMap, 						//地图
							   mpKeyFrameDatabase, 			//关键帧数据库
							   mpVocabulary, 				//ORB字典
							   mSensor!=MONOCULAR);			//当前的传感器是否是单目
//构造并运行回环检测线程
mptLoopClosing = new thread(&ORB_SLAM2::LoopClosing::Run,	//线程的主函数
							mpLoopCloser);					//该函数的参数
    
    //Initialize the Viewer thread and launch
    if(bUseViewer)
    {
    	//如果指定了，程序的运行过程中需要运行可视化部分
    	//构造可视化器viewer
        mpViewer = new Viewer(this, 			//又是这个
        					  mpFrameDrawer,	//帧绘制器
        					  mpMapDrawer,		//地图绘制器
        					  mpTracker,		//追踪器
        					  strSettingsFile);	//配置文件的访问路径
        //构造并运行可视化线程
        mptViewer = new thread(&Viewer::Run, mpViewer);
        //给运动追踪器设置其查看器
        mpTracker->SetViewer(mpViewer);
    }
    
    //Set pointers between threads
    //设置进程间的指针
    mpTracker->SetLocalMapper(mpLocalMapper);
    mpTracker->SetLoopClosing(mpLoopCloser);
    
    mpLocalMapper->SetTracker(mpTracker);
    mpLocalMapper->SetLoopCloser(mpLoopCloser);
    
    mpLoopCloser->SetTracker(mpTracker);
    mpLoopCloser->SetLocalMapper(mpLocalMapper);
}
```

通过上面的代码，我们可以看到有几个核心部分:

### (1) mpVocabulary

        //建立一个新的ORB字典
        mpVocabulary = new ORBVocabulary();
        //获取字典加载状态
        bool bVocLoad = mpVocabulary->loadFromTextFile(strVocFile);
       	//Create KeyFrame Database
        mpKeyFrameDatabase = new KeyFrameDatabase(*mpVocabulary);
这里的 strVocFile 就是命令行传入的参数 Vocabulary/ORBvoc.txt，  这个呢是属于比较大的一块，主要和特征匹配相关，所以暂时就不进行详细讲解了，后续会专属章节进行讲解。 Vocabulary/ORBvoc.txt  是离线训练而来的文件，主要流程如下:

	首先图像提取ORB 特征点，将描述子通过 k-means 进行聚类，根据设定的树的分支数和深度，
	从叶子节点开始聚类一直到根节点，最后得到一个非常大的 vocabulary tree
	
	1、遍历所有的训练图像，对每幅图像提取ORB特征点。
	2、设定vocabulary tree的分支数K和深度L。将特征点的每个描述子用 K-means聚类，变成 K个集合，作为vocabulary tree 的第1层级，然后对每个集合重复该聚类操作，就得到了vocabulary tree的第2层级，继续迭代最后得到满足条件的vocabulary tree，它的规模通常比较大，比如ORB-SLAM2使用的离线字典就有108万+ 个节点。
	3、离根节点最远的一层节点称为叶子或者单词 Word。根据每个Word 在训练集中的相关程度给定一个权重weight，训练集里出现的次数越多，说明辨别力越差，给与的权重越低。
上面只是描述了一下 离线训练 vocabulary tree（也称为字典），具体讲解我们后续再进行。**其主要的作用是: 特征匹配 ， 关键帧辨别。**

### (2) mpTracker

    //Create Drawers. These are used by the Viewer
    //这里的帧绘制器和地图绘制器将会被可视化的Viewer所使用
    mpFrameDrawer = new FrameDrawer(mpMap);
    mpMapDrawer = new MapDrawer(mpMap, strSettingsFile);
    //初始化追踪线程，构造了追踪线程需要的追踪器mpTracker = new Tracking
    //Initialize the Tracking thread
    //(it will live in the main thread of execution, the one that called this constructor)
    mpTracker = new Tracking(this,						//现在还不是很明白为什么这里还需要一个this指针  TODO  
    						 mpVocabulary,				//字典
    						 mpFrameDrawer, 			//帧绘制器
    						 mpMapDrawer,				//地图绘制器
                             mpMap, 					//地图
                             mpKeyFrameDatabase, 		//关键帧地图
                             strSettingsFile, 			//设置文件路径
                             mSensor);					//传感器类型iomanip
其上主要初始化了追踪线程（或者说主线程），同时我们可以看到其初始化参数中包含了 mpFrameDrawer ， mpMapDrawer 。这两个对象主要是负责对帧与地图的绘画。

### (3) mptLocalMapping，mptLoopClosing

    //初始化并运行局部建图线程new LocalMapping & new thread
    //Initialize the Local Mapping thread and launch
    //初始化局部建图线程，构造了建图需要的局部建图器mpLocalMapper = new LocalMapping
    mpLocalMapper = new LocalMapping(mpMap, 				// 指定地图指针，该地图结构存储所有指向关键帧和地图点的指针                                                   // Map structure that stores the pointers to all KeyFrames and MapPoints.                            // 称为指定存储所有指向关键帧和地图点指针的地图指针
    								 mSensor==MONOCULAR);	// 判断mSensor是不是MONOCULAR
    //构造并运行局部建图线程
    mptLocalMapping = new thread(&ORB_SLAM2::LocalMapping::Run,	//这个线程会调用的函数
    							 mpLocalMapper);				//这个调用函数的参数(调用局部建图器)
    //初始化并运行回环检测线程
    //Initialize the Loop Closing thread and launch
    //构造了回环检测需要的回环检测器mpLoopCloser = new LoopClosing
    mpLoopCloser = new LoopClosing(mpMap, 						//地图
    							   mpKeyFrameDatabase, 			//关键帧数据库
    							   mpVocabulary, 				//ORB字典
    							   mSensor!=MONOCULAR);			//当前的传感器是否是单目
    //构造并运行回环检测线程
    mptLoopClosing = new thread(&ORB_SLAM2::LoopClosing::Run,	//线程的主函数
    							mpLoopCloser);					//该函数的参数
其上我们可以知道，其主要运行了两个线程ORB_SLAM2::LocalMapping::Run 以及  ORB_SLAM2::LoopClosing::Run，分别是局部建图以及闭环线程。这里已经启动了，这两个线程是后续我们需要重点讲解的大块，所以这里也不做详细的介绍了。

### (4) mptViewer

```
    	//如果指定了，程序的运行过程中需要运行可视化部分
    	//构造可视化器viewer
        mpViewer = new Viewer(this, 			//又是这个
        					  mpFrameDrawer,	//帧绘制器
        					  mpMapDrawer,		//地图绘制器
        					  mpTracker,		//追踪器
        					  strSettingsFile);	//配置文件的访问路径
        //构造并运行可视化线程
        mptViewer = new thread(&Viewer::Run, mpViewer);
        //给运动追踪器设置其查看器
        mpTracker->SetViewer(mpViewer);
```

这里主要创建了一个可视化的线程，可视化的线程与追踪主线程是息息相关的。可视化的操作，主要根据追踪线程传递的信息来执行界面的绘画。

# 3. track追踪_总体框架

## 一、总体框架**TrackMonocular→GrabImageMonocular**

运行单目摄像头的指令图下:

	cd /my_work/01.ORB-SLAM2源码解析/ORB_SLAM2
	
	#执行指令./Examples/Monocular/mono_tum Vocabulary/ORBvoc.txt Examples/Monocular/TUMX.yaml PATH_TO_SEQUENCE_FOLDER
	# 注意，此处的TUMX.yaml⽂件要对应于你下载的数据集类型, PATH_TO_SEQUENCE_FOLDER要对应于你的数据集⽂件夹路径，所以本人修改为:
	./Examples/Monocular/mono_tum Vocabulary/ORBvoc.txt Examples/Monocular/TUM1.yaml /my_work/01.ORB-SLAM2源码解析/Datasets/rgbd_dataset_freiburg1_xyz
执行如上指令之后，代码从 ./Examples/Monocular/mono_tum.cc 中的 main 函数开始，其中的主要核心代码如下（只粘贴重要代码）:

	//循环,读取图像进行追踪
	for(int ni=0; ni<nImages; ni++)
		//读取图像获得像素
		im = cv::imread(string(argv[3])+"/"+vstrImageFilenames[ni],CV_LOAD_IMAGE_UNCHANGED);
		//根据输入的图像，进行单目追踪
	    SLAM.TrackMonocular(im,tframe);
	// 停止所有线程
	SLAM.Shutdown();
其实可以看到，其上核心为 SLAM.TrackMonocular(im,tframe)，所有接下来的好些章节，都是围绕该函数进行讲解。

## 二、TrackMonocular

在 src\System.cc 文件中，我们可以看到 TrackMonocular 函数实现的具体过程。在讲解该函数时，我们先回顾一下上一节我们讲解的 ORB_SLAM2::System 构造函数，其可以看到如下代码：

	//追踪器，负责追踪的一些相关操作
	mpTracker = new Tracking(this,mpVocabulary,mpFrameDrawer,mpMapDrawer,mpMap,mpKeyFrameDatabase,strSettingsFile,mSensor);
	//局部建图器,负责局部地图的构建			
	mpLocalMapper = new LocalMapping(mpMap,mSensor==MONOCULAR);	
	//闭环器,闭环检测以及闭环操作
	mpLoopCloser = new LoopClosing(mpMap,mpKeyFrameDatabase,mpVocabulary,mSensor!=MONOCULAR);		
其上的类对象mpTracker、mpLocalMapper、mpLoopCloser都是十分重要的，是整个系统最最核心的3个类对象。回忆起我们上一章节讲解过的内容，那么我们再来看看 TrackMonocular 这个函数：

```//同理，输入为单目图像时的追踪器接口
cv::Mat System::TrackMonocular(const cv::Mat &im, const double &timestamp)
{
    if(mSensor!=MONOCULAR)
    {
        cerr << "ERROR: you called TrackMonocular but input sensor was not set to Monocular." << endl;
        exit(-1);
    }

    // Check mode change
    {
        // 独占锁，主要是为了mbActivateLocalizationMode和mbDeactivateLocalizationMode不会发生混乱
        unique_lock<mutex> lock(mMutexMode);
        // mbActivateLocalizationMode为true会关闭局部地图线程
        if(mbActivateLocalizationMode)
        {
            mpLocalMapper->RequestStop();
    
            // Wait until Local Mapping has effectively stopped
            while(!mpLocalMapper->isStopped())
            {
                usleep(1000);
            }
    
            // 局部地图关闭以后，只进行追踪的线程，只计算相机的位姿，没有对局部地图进行更新
            // 设置mbOnlyTracking为真
            mpTracker->InformOnlyTracking(true);
            // 关闭线程可以使得别的线程得到更多的资源
            mbActivateLocalizationMode = false;
        }
        // 如果mbDeactivateLocalizationMode是true，局部地图线程就被释放, 关键帧从局部地图中删除.
        if(mbDeactivateLocalizationMode)
        {
            mpTracker->InformOnlyTracking(false);
            mpLocalMapper->Release();
            mbDeactivateLocalizationMode = false;
        }
    }
    
    // Check reset
    {
    unique_lock<mutex> lock(mMutexReset);
    if(mbReset)
    {
        mpTracker->Reset();
        mbReset = false;
    }
    }
    
    //获取相机位姿的估计结果
    cv::Mat Tcw = mpTracker->GrabImageMonocular(im,timestamp);
    
    unique_lock<mutex> lock2(mMutexState);
    mTrackingState = mpTracker->mState;
    mTrackedMapPoints = mpTracker->mCurrentFrame.mvpMapPoints;
    mTrackedKeyPointsUn = mpTracker->mCurrentFrame.mvKeysUn;
    
    return Tcw;
}
```

其上的操作总体来说还是比较简单的，主要流程如下:

	1、判断传感器的类型是否为单目模式，如果不是，则表示设置错误，函数直接返回
	
	2、上锁 模式锁(mMutexMode):
		(1)如果目前需要激活定位模式，则请求停止局部建图，并且等待局部建图线程停止，设置为仅追踪模式。
		(2)如果目前需要取消定位模式，则通知局部建图可以工作了，关闭仅追踪模式
		
	3、上锁 复位锁(mMutexReset): 检查是否存在复位请求，如果有，则进行复位操作
	
	4、核心部分: 根据输入的图像获得相机位姿态（其中包含了特征提取匹配，地图初始化，关键帧查询等操作）
	
	5、进行数据更新，如追踪状态、当前帧的地图点、当前帧矫正之后的关键点等。
其上的核心部分为 根据输入的图像获得相机位姿态，也就是函数 GrabImageMonocular()。

## 三、GrabImageMonocular

该函数的调用代码为

```
	 cv::Mat Tcw = mpTracker->GrabImageMonocular(im,timestamp);
```

其主要进行了操作:

	1、如果输入的图像不为灰度图，则转换为灰度图。
	
	2、根据是否为第一帧或者或者是否进行初始化，使用不同的参数(提取的特征点数目)进行Frame类的创建
	
	3、Track(); 进行追踪
其代码注释如下

```
cv::Mat Tracking::GrabImageMonocular(const cv::Mat &im,const double &timestamp)
{
    mImGray = im;

    // Step 1 ：将彩色图像转为灰度图像
    //若图片是3、4通道的，还需要转化成灰度图
    if(mImGray.channels()==3)
    {
        if(mbRGB)
            cvtColor(mImGray,mImGray,CV_RGB2GRAY);
        else
            cvtColor(mImGray,mImGray,CV_BGR2GRAY);
    }
    else if(mImGray.channels()==4)
    {
        if(mbRGB)
            cvtColor(mImGray,mImGray,CV_RGBA2GRAY);
        else
            cvtColor(mImGray,mImGray,CV_BGRA2GRAY);
    }
    
    // Step 2 ：构造Frame
    //判断当前是否进行了初始化，如果当前为第一帧，则 mState==NO_IMAGES_YET，表示没有进行初始化
    if(mState==NOT_INITIALIZED || mState==NO_IMAGES_YET) //没有成功初始化的前一个状态就是NO_IMAGES_YET
        mCurrentFrame = Frame(
            mImGray,
            timestamp,
            mpIniORBextractor,      //初始化ORB特征点提取器会提取2倍的指定特征点数目
            mpORBVocabulary,
            mK,
            mDistCoef,
            mbf,
            mThDepth);
    else
        mCurrentFrame = Frame(
            mImGray,
            timestamp,
            mpORBextractorLeft,     //正常运行的时的ORB特征点提取器，提取指定数目特征点
            mpORBVocabulary,
            mK,
            mDistCoef,
            mbf,
            mThDepth);
    
    // Step 3 ：跟踪
    Track();
    
    //返回当前帧的位姿
    return mCurrentFrame.mTcw.clone();
}
```

四、结语

从 GrabImageMonocular 函数中，我们可以看到其最核心的部分应该存在于 Track() 函数之中，但是其上的 Frame 创建也是十分重要的，其中做了很多追踪需要的预备工作，如图像金字塔、特征提取，关键点矫正、特征点均匀分布等操作，其也是我们接下来学习的主要对象。

## 四、Tracking::Tracking()与 Frame::Frame()

### 一、前言

上一节我们说到，[Frame](https://so.csdn.net/so/search?q=Frame&spm=1001.2101.3001.7020) 的构建是比较重要的一个环节，但是再讲解其之前，我们再补充一点内容，那就是 src\Tracking.cc 中 Tracking 的构造函数，该类对象被是在 src\System.cc 的初始化函数中被创建的，如下；

	//追踪器，负责追踪的一些相关操作
	mpTracker = new Tracking(this,mpVocabulary,mpFrameDrawer,mpMapDrawer,mpMap,mpKeyFrameDatabase,strSettingsFile,mSensor);
### 二、tracking构造函数

Tracking 的构造函数主要进行了如下操作；

	1、根据配置文件(如"Examples/RGB-D/TUM1.yaml")中获得相机参数:
		(1)内参矩阵，矫正系数，帧率等信息
		(2)如果为双目，则还需要获得 Camera.bf参数
	
	2、根据配置文件获得特征提取的相关参数:
		(1)每帧图像提取关键点总数目
		(2)金字塔层数与缩放尺度
		(3)提取fast特征点的相关参数
	
	3、根据特征提取的相关配置创建特征提取类ORBextractor对象
		(1)所有传感器都是需要创建左目特征提取器 mpORBextractorLeft
		(2)如果为双目则需要创建右目特征提取器 mpORBextractorRight
		(3)如果为单目则需要额外创建一个初始化特征提取器 mpIniORBextractor
其上提到的特征提取器，是比较重要的一部分，我们再下一个章节进行讲解，关于 Tracking::Tracking 构造函数的注释代码如下:
```
///构造函数
Tracking::Tracking(
    System *pSys,                       //系统实例
    ORBVocabulary* pVoc,                //BOW字典
    FrameDrawer *pFrameDrawer,          //帧绘制器
    MapDrawer *pMapDrawer,              //地图点绘制器
    Map *pMap,                          //地图句柄
    KeyFrameDatabase* pKFDB,            //关键帧产生的词袋数据库
    const string &strSettingPath,       //配置文件路径
    const int sensor):                  //传感器类型
        mState(NO_IMAGES_YET),                              //当前系统还没有准备好
        mSensor(sensor),                                
        mbOnlyTracking(false),                              //处于SLAM模式
        mbVO(false),                                        //当处于纯跟踪模式的时候，这个变量表示了当前跟踪状态的好坏
        mpORBVocabulary(pVoc),          
        mpKeyFrameDB(pKFDB), 
        mpInitializer(static_cast<Initializer*>(NULL)),     //暂时给地图初始化器设置为空指针
        mpSystem(pSys), 
        mpViewer(NULL),                                     //注意可视化的查看器是可选的，因为ORB-SLAM2最后是被编译成为一个库，所以对方人拿过来用的时候也应该有权力说我不要可视化界面（何况可视化界面也要占用不少的CPU资源）
        mpFrameDrawer(pFrameDrawer),
        mpMapDrawer(pMapDrawer), 
        mpMap(pMap), 
        mnLastRelocFrameId(0)                               //恢复为0,没有进行这个过程的时候的默认值
{
    // Load camera parameters from settings file
    // Step 1 从配置文件中加载相机参数
    cv::FileStorage fSettings(strSettingPath, cv::FileStorage::READ);
    float fx = fSettings["Camera.fx"];
    float fy = fSettings["Camera.fy"];
    float cx = fSettings["Camera.cx"];
    float cy = fSettings["Camera.cy"];

    //     |fx  0   cx|
    // K = |0   fy  cy|
    //     |0   0   1 |
    //构造相机内参矩阵
    cv::Mat K = cv::Mat::eye(3,3,CV_32F);
    K.at<float>(0,0) = fx;
    K.at<float>(1,1) = fy;
    K.at<float>(0,2) = cx;
    K.at<float>(1,2) = cy;
    K.copyTo(mK);
    
    // 图像矫正系数
    // [k1 k2 p1 p2 k3]
    cv::Mat DistCoef(4,1,CV_32F);
    DistCoef.at<float>(0) = fSettings["Camera.k1"];
    DistCoef.at<float>(1) = fSettings["Camera.k2"];
    DistCoef.at<float>(2) = fSettings["Camera.p1"];
    DistCoef.at<float>(3) = fSettings["Camera.p2"];
    const float k3 = fSettings["Camera.k3"];
    //有些相机的畸变系数中会没有k3项
    if(k3!=0)
    {
        DistCoef.resize(5);
        DistCoef.at<float>(4) = k3;
    }
    DistCoef.copyTo(mDistCoef);
    
    // 双目摄像头baseline * fx 50
    mbf = fSettings["Camera.bf"];
    
    float fps = fSettings["Camera.fps"];
    if(fps==0)
        fps=30;
    
    // Max/Min Frames to insert keyframes and to check relocalisation
    mMinFrames = 0;
    mMaxFrames = fps;
    
    //输出
    cout << endl << "Camera Parameters: " << endl;
    cout << "- fx: " << fx << endl;
    cout << "- fy: " << fy << endl;
    cout << "- cx: " << cx << endl;
    cout << "- cy: " << cy << endl;
    cout << "- k1: " << DistCoef.at<float>(0) << endl;
    cout << "- k2: " << DistCoef.at<float>(1) << endl;
    if(DistCoef.rows==5)
        cout << "- k3: " << DistCoef.at<float>(4) << endl;
    cout << "- p1: " << DistCoef.at<float>(2) << endl;
    cout << "- p2: " << DistCoef.at<float>(3) << endl;
    cout << "- fps: " << fps << endl;
    
    // 1:RGB 0:BGR
    int nRGB = fSettings["Camera.RGB"];
    mbRGB = nRGB;
    
    if(mbRGB)
        cout << "- color order: RGB (ignored if grayscale)" << endl;
    else
        cout << "- color order: BGR (ignored if grayscale)" << endl;
    
    // Load ORB parameters
    
    // Step 2 加载ORB特征点有关的参数,并新建特征点提取器
    
    // 每一帧提取的特征点数 1000
    int nFeatures = fSettings["ORBextractor.nFeatures"];
    // 图像建立金字塔时的变化尺度 1.2
    float fScaleFactor = fSettings["ORBextractor.scaleFactor"];
    // 尺度金字塔的层数 8
    int nLevels = fSettings["ORBextractor.nLevels"];
    // 提取fast特征点的默认阈值 20
    int fIniThFAST = fSettings["ORBextractor.iniThFAST"];
    // 如果默认阈值提取不出足够fast特征点，则使用最小阈值 8
    int fMinThFAST = fSettings["ORBextractor.minThFAST"];
    
    // tracking过程都会用到mpORBextractorLeft作为特征点提取器
    mpORBextractorLeft = new ORBextractor(
        nFeatures,      //参数的含义还是看上面的注释吧
        fScaleFactor,
        nLevels,
        fIniThFAST,
        fMinThFAST);
    
    // 如果是双目，tracking过程中还会用用到mpORBextractorRight作为右目特征点提取器
    if(sensor==System::STEREO)
        mpORBextractorRight = new ORBextractor(nFeatures,fScaleFactor,nLevels,fIniThFAST,fMinThFAST);
    
    // 在单目初始化的时候，会用mpIniORBextractor来作为特征点提取器
    if(sensor==System::MONOCULAR)
        mpIniORBextractor = new ORBextractor(2*nFeatures,fScaleFactor,nLevels,fIniThFAST,fMinThFAST);
    
    cout << endl  << "ORB Extractor Parameters: " << endl;
    cout << "- Number of Features: " << nFeatures << endl;
    cout << "- Scale Levels: " << nLevels << endl;
    cout << "- Scale Factor: " << fScaleFactor << endl;
    cout << "- Initial Fast Threshold: " << fIniThFAST << endl;
    cout << "- Minimum Fast Threshold: " << fMinThFAST << endl;
    
    if(sensor==System::STEREO || sensor==System::RGBD)
    {
        // 判断一个3D点远/近的阈值 mbf * 35 / fx
        //ThDepth其实就是表示基线长度的多少倍
        mThDepth = mbf*(float)fSettings["ThDepth"]/fx;
        cout << endl << "Depth Threshold (Close/Far Points): " << mThDepth << endl;
    }
    
    if(sensor==System::RGBD)
    {
        // 深度相机disparity转化为depth时的因子
        mDepthMapFactor = fSettings["DepthMapFactor"];
        if(fabs(mDepthMapFactor)<1e-5)
            mDepthMapFactor=1;
        else
            mDepthMapFactor = 1.0f/mDepthMapFactor;
    }

}
```

mpIniORBextractor 与 mpORBextractorLeft、mpORBextractorRight 的不同，主要在于 _nfeatures 参数的不同，也就是特征提取的数目不一样。

### 三、Frame 构造函数

我们已经对 Tracking 的构造函数进行了简单的补充，那么我们回到之前的代码src\Tracking.cc 中的 GrabImageMonocular() 函数，上一节我们说到其会根据输入的图像进行 Frame 的创建，如下:

    // Step 2 ：构造Frame
    //判断当前是否进行了初始化，如果当前为第一帧，则 mState==NO_IMAGES_YET，表示没有进行初始化
    if(mState==NOT_INITIALIZED || mState==NO_IMAGES_YET) //没有成功初始化的前一个状态就是NO_IMAGES_YET
        mCurrentFrame = Frame(mImGray,timestamp,mpIniORBextractor,mpORBVocabulary,mK,mDistCoef,mbf,mThDepth);
    else
        mCurrentFrame = Frame(mImGray,timestamp,mpORBextractorLeft,mpORBVocabulary,mK,mDistCoef,mbf,mThDepth);
其上我们可以看到，当前是否初始化创建出来的 Frame 对象是一样的，其主要区别为传入的特征提取器为 mpIniORBextractor 或者 mpORBextractorLeft。 该两个类对象都是再 Tracking::Tracking() 中被创建来的。我们暂且不去理会。我们先来看看 Frame 的构造函数。其主要进行了如下操作:

	根据特征提取器，获得图像金字塔的相关参数
	进行特征提取 → ExtractORB(0,imGray);
	关键点畸变矫正 → UndistortKeyPoints();
	如果为初始化帧，则还需要将特征点分配到网络之中。 
其主要代码注释如下:
```
Frame::Frame(const cv::Mat &imGray, const double &timeStamp, ORBextractor* extractor,ORBVocabulary* voc, cv::Mat &K, cv::Mat &distCoef, const float &bf, const float &thDepth)
    :mpORBvocabulary(voc),mpORBextractorLeft(extractor),mpORBextractorRight(static_cast<ORBextractor*>(NULL)),
     mTimeStamp(timeStamp), mK(K.clone()), mDistCoef(distCoef.clone()), mbf(bf), mThDepth(thDepth)
{
    // Frame ID
	// Step 1 帧的ID 自增
    mnId=nNextId++;

    // Step 2 计算图像金字塔的参数 
    // Scale Level Info
    //获取图像金字塔的层数
    mnScaleLevels = mpORBextractorLeft->GetLevels();
    //获取每层的缩放因子
    mfScaleFactor = mpORBextractorLeft->GetScaleFactor();
    //计算每层缩放因子的自然对数
    mfLogScaleFactor = log(mfScaleFactor);
    //获取各层图像的缩放因子
    mvScaleFactors = mpORBextractorLeft->GetScaleFactors();
    //获取各层图像的缩放因子的倒数
    mvInvScaleFactors = mpORBextractorLeft->GetInverseScaleFactors();
    //获取sigma^2
    mvLevelSigma2 = mpORBextractorLeft->GetScaleSigmaSquares();
    //获取sigma^2的倒数
    mvInvLevelSigma2 = mpORBextractorLeft->GetInverseScaleSigmaSquares();
    
    // ORB extraction
    // Step 3 对这个单目图像进行提取特征点, 第一个参数0-左图， 1-右图
    ExtractORB(0,imGray);
    
    //求出特征点的个数
    N = mvKeys.size();
    
    //如果没有能够成功提取出特征点，那么就直接返回了
    if(mvKeys.empty())
        return;
    
    // Step 4 用OpenCV的矫正函数、内参对提取到的特征点进行矫正 
    UndistortKeyPoints();
    
    // Set no stereo information
    // 由于单目相机无法直接获得立体信息，所以这里要给右图像对应点和深度赋值-1表示没有相关信息
    mvuRight = vector<float>(N,-1);
    mvDepth = vector<float>(N,-1);


    // 初始化本帧的地图点
    mvpMapPoints = vector<MapPoint*>(N,static_cast<MapPoint*>(NULL));
    // 记录地图点是否为外点，初始化均为外点false
    mvbOutlier = vector<bool>(N,false);
    
    // This is done only for the first Frame (or after a change in the calibration)
    //  Step 5 计算去畸变后图像边界，将特征点分配到网格中。这个过程一般是在第一帧或者是相机标定参数发生变化之后进行
    if(mbInitialComputations)
    {
    	// 计算去畸变后图像的边界
        ComputeImageBounds(imGray);
    
    	// 表示一个图像像素相当于多少个图像网格列（宽）
        mfGridElementWidthInv=static_cast<float>(FRAME_GRID_COLS)/static_cast<float>(mnMaxX-mnMinX);
    	// 表示一个图像像素相当于多少个图像网格行（高）
        mfGridElementHeightInv=static_cast<float>(FRAME_GRID_ROWS)/static_cast<float>(mnMaxY-mnMinY);
    
    	//给类的静态成员变量复制
        fx = K.at<float>(0,0);
        fy = K.at<float>(1,1);
        cx = K.at<float>(0,2);
        cy = K.at<float>(1,2);
    	// 猜测是因为这种除法计算需要的时间略长，所以这里直接存储了这个中间计算结果
        invfx = 1.0f/fx;
        invfy = 1.0f/fy;
    
    	//特殊的初始化过程完成，标志复位
        mbInitialComputations=false;
    }
    
    //计算 basline
    mb = mbf/fx;
    
    // 将特征点分配到图像网格中 
    AssignFeaturesToGrid();
}
```

----------------------------------插播介绍内联函数inline-------------------------------------------------------------

为什么讲到这个函数呢？是因为在上面代码中存在很多小函数来直接获取类的成员变量的，GetLevels(),GetScaleFactor(),GetScaleFactors(),GetInverseScaleFactors(),比如说下面

```
mnScaleLevels = mpORBextractorLeft->GetLevels();
```

我们打开该函数可以看到

<img src="C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220923101142582.png" alt="image-20220923101142582" style="zoom:50%;" />

所有的函数都使用了inline进行了修饰，为什么呢？这是因为inline函数在此处起到更快的代码执行速度，inline就是将函数的代码直接放在内联函数的位置上，而调用一般函数的时候，是指令跳转到被调用函数的入口地址，执行完被调用函数后，指令再跳转回主函数上继续执行后面的代码；如果函数本身内容比较少，代码比较短，函数功能相对简单，使用内链函数避免了指令的来回跳转，加快程序执行速度。下面详细讲解内联函数：

**1.什么是内联函数**

用关键字inline修饰的函数就是内联函数。关键字在函数声明和定义的时候都要加上，不写系统还是会当成常规函数

**2.内联函数与一般函数的区别**

1）内联含函数比一般函数在前面多一个inline修饰符

2）内联函数是直接复制“镶嵌”到主函数中去的，就是将内联函数的代码直接放在内联函数的位置上，这与一般函数不同，主函数在调用一般函数的时候，是指令跳转到被调用函数的入口地址，执行完被调用函数后，指令再跳转回主函数上继续执行后面的代码；而由于内联函数是将函数的代码直接放在了函数的位置上，所以没有指令跳转，指令按顺序执行

3）一般函数的代码段只有一份，放在内存中的某个位置上，当程序调用它是，指令就跳转过来；当下一次程序调用它是，指令又跳转过来；而内联函数是程序中调用几次内联函数，内联函数的代码就会复制几份放在对应的位置上

4）内联函数一般在头文件中定义，而一般函数在头文件中声明，在cpp中定义

**3.利与弊**

利：避免了指令的来回跳转，加快程序执行速度

弊：代码被多次复制，增加了代码量，占用更多的内存空间

**4.什么时候使用内联函数**

1）函数本身内容比较少，代码比较短，函数功能相对简单

2）函数被调用得频繁，不如循环中的函数

**5.什么时候不能使用内联函数**

1）函数代码量多，功能复杂，体积庞大。对于这种函数，就算加上inline修饰符，系统也不一定会相应，可能还是会当成一般函数处理

2）递归函数不能使用内联函数


6.内联函数比宏更强大

看一段代码：
```
#include <iostream>
using namespace std;

#define SUM(x) x*x

inline int fun(int x)
{
	return x * x;
}

int main()
{
	int a = SUM(2 + 3);
	int b = fun(2 + 3);

	cout << "a = " << a << endl;
	cout << "b = " << b << endl;
	 
	system("pause");
	return 0;
}
```
执行结果：

![img](https://img-blog.csdn.net/201808031754259?watermark/2/text/aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzMzNzU3Mzk4/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70)

为什么通过宏执行的结果是11呢，宏比较机械和简单，只是将传入的参数直接放上去就执行，所以int a = SUM(2  + 3);就相当于int a = 2 + 3 * 2  +3;由于乘法优先级更高，所以得到a的值为11；而在内联函数中，传入的参数是5，所以得到25

为了得到正确的结果，我们应该将宏改变为：

```
#define SUM(x) ((x)*(x))
```

**7.类与内联函数**

1）类内定义的函数都是内联函数，不管是否有inline修饰符

2）[函数声明](https://so.csdn.net/so/search?q=函数声明&spm=1001.2101.3001.7020)在类内，但定义在类外的看是否有inline修饰符，如果有就是内联函数，否则不是

----------------------------------插播介绍内联函数结束-------------------------------------------------------------

### 四、结语

通过前面的介绍，大家对于追踪线程有了大概了解，其中最重要最复杂的函数是 Tracking::GrabImageMonocular() 中调用的 Track() 函数，但是在他的前面的进行了 Frame 对象的创建，我们说其也是非常重要的，主要是因为他的构造函数之中做了图像金字塔，以及ORB特征提取等操作。这是追踪过程中必不可少的预备工作。那么从下篇博客开始，我们将介绍金字塔以及ORB特征的相关知识。

# 4. ORBextractor()

## 一、前言

上一节我们说到了 src/[Frame](https://so.csdn.net/so/search?q=Frame&spm=1001.2101.3001.7020).cc 中的 Frame::Frame() 函数，调用了比较重要的两个函数:

    // ORB extraction
    // Step 3 对这个单目图像进行提取特征点, 第一个参数0-左图， 1-右图
    ExtractORB(0,imGray);
    // Step 4 用OpenCV的矫正函数、内参对提取到的特征点进行矫正 
    UndistortKeyPoints();
我们首先对 ExtractORB() 进行讲解，UndistortKeyPoints() 放到后面的章节。进入 ExtractORB 函数，可以看到其实现如下：

```
void Frame::ExtractORB(int flag, const cv::Mat &im)
{
    // 判断是左图还是右图
    if(flag==0)
        // 左图的话就套使用左图指定的特征点提取器，并将提取结果保存到对应的变量中 
        // 这里使用了仿函数来完成，重载了括号运算符 ORBextractor::operator() 
        (*mpORBextractorLeft)(im,				//待提取特征点的图像
							  cv::Mat(),		//掩摸图像, 实际没有用到
							  mvKeys,			//输出变量，用于保存提取后的特征点
							  mDescriptors);	//输出变量，用于保存特征点的描述子
    else
        // 右图的话就需要使用右图指定的特征点提取器，并将提取结果保存到对应的变量中 
        (*mpORBextractorRight)(im,cv::Mat(),mvKeysRight,mDescriptorsRight);
}
```

![image-20220925211922703](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220925211922703.png)

![image-20220925211334193](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220925211334193.png)

------

插播介绍仿函数：

functor（[仿函数](https://so.csdn.net/so/search?q=仿函数&spm=1001.2101.3001.7020)）, 或者称之为function object(函数对象)， 是STL的四大组件之一。

什么是仿函数呢？ 

一个函数对象是封装在类中， 从而看起来更像是一个对象。 这个类只有一个[成员函数](https://so.csdn.net/so/search?q=成员函数&spm=1001.2101.3001.7020)， 即重载了（） (括号)的运算符。 它没有任何数据。 该类被模板化了， 从而可以应付多种数据类型。

看一个例子：

![img](https://img-blog.csdn.net/20140726170711374?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvYTEzMDczNw==/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/SouthEast)
 上例中， 我们定义了一个类X， 然后我们在类中重载了一个[运算符](https://so.csdn.net/so/search?q=运算符&spm=1001.2101.3001.7020)， 即括号，（）,  该运算符吃一个string类型的参数。

在主程序中， 我们声明了一个类X的对象foo, 然后我们调用仿函数（函数对象）foo ，传进参数“Hi”， 注意我们定义了（） 运算符。

 上面中， foo is an instance of X,  but we can use it as if it is a function。

这就是functor的设计思想。 仿函数推广了函数的定义。 也就是说任何表现出函数的特征的都是函数。  例如上例中， X是一个class， 但是表现的像是函数， 我们就称其为函数。 STL中提供了许多预定义的函数对象， 如果要使用，  应包含头文件<functional>。

Q : 人们为啥要用functor, 而不用普通的函数（Motivation）？

A : 这源于仿函数的优点： 你可以将仿函数想象成为一个smart function， 可以做许多普通函数无法完成的事情。

例如我们可以在上面的类X中定义成员函数。 这样我们可以声明多个不同的functor, 数据不同。 （回忆一下， 对于类对象， 不同类对象的成员函数是共享的， 数据是属于类对象本身的）。

第二点， 一个functor可以由其自己的type。 regular functions can only be  differentiate by their signitures。  If 2 functiions have the same  signature, then they are the same function.  Howere, 2 functors can be  of different type, even if thry have the exact same signature.


![img](https://img-blog.csdn.net/20140726181046439?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvYTEzMDczNw==/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/SouthEast)

 正是因为仿函数的存在， 才有parametriized function:

![img](https://img-blog.csdn.net/20140726181956418?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvYTEzMDczNw==/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/SouthEast)
上例中， 我们在类X中添加了一个构造函数（constructor）X(int i)， 在main 函数中， X（8）（参数化函数， X of 8）调用了普通的参数“Hi”。

Q： 为什么要这样做（上例）， 即一个参数是8， 一个参数是“Hi”？

A： 为了说明我们为啥这样做， 下面举一个例子：

![img](https://img-blog.csdn.net/20140726182153984?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvYTEzMDczNw==/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/SouthEast)
 显然， 上述中， 我们定义了一个函数， 将输出i 加2

在主函数中， 我们定义了ＳＴＬ，　 不难看出， 是输出向量vec 所有的元素都加上2的情况。 所以输出是：

4

5

6

7

然而上述的add2 函数， 从名字上看是加2， 但是当加3 的时候， 如果我们还有add2, 就会出现问题了， 显然not extensible。  

下面说说如何让我们的code 更加的flexible：

一个很明显的做法就是定义一个全局变量（灵活的实现加几 显示）：

![img](https://img-blog.csdn.net/20140726191237061?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvYTEzMDczNw==/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/SouthEast)
 这样， 我们就可以加上任何val 显示了。

但是， 使用全局变量is a nasty coding practice， and we donot want to do this。

那么我们还有其他的解决办法吗？

是的， 有的， 我们可以定义模板函数， 将val 定义为模板的参数。 如下图， 主函数中， 给出模板的参数。

![img](https://img-blog.csdn.net/20140726192654813?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvYTEzMDczNw==/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/SouthEast)
 使用模板更加的flexible 了。 我们可以通过改变VAL实现加上任何值在输出的效果。

but, there is still a problem. a template variable is resolved at  compile time。 so it has to be a compile time constant。 所以， 下面的程序是错误的，  无法通过编译， 因为x是变量：

![img](https://img-blog.csdn.net/20140726193222570?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvYTEzMDczNw==/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/SouthEast)
 那么， 还有别的解决办法吗？

有的， the best solution: 使用仿函数（functor）。

![img](https://img-blog.csdn.net/20140726193348796?watermark/2/text/aHR0cDovL2Jsb2cuY3Nkbi5uZXQvYTEzMDczNw==/font/5a6L5L2T/fontsize/400/fill/I0JBQkFCMA==/dissolve/70/gravity/SouthEast)

------

可以看到，其主要是通过 mpORBextractorLeft，或者 mpORBextractorRight 调用函数，通过前面的章节我们可以知道这两个类对象是在 Tracking::Tracking 构造函数中创建的：

    // tracking过程都会用到mpORBextractorLeft作为特征点提取器
    mpORBextractorLeft = new ORBextractor(
        nFeatures,      //参数的含义还是看上面的注释吧
        fScaleFactor,
        nLevels,
        fIniThFAST,
        fMinThFAST);
    
    // 如果是双目，tracking过程中还会用用到mpORBextractorRight作为右目特征点提取器
    if(sensor==System::STEREO)
        mpORBextractorRight = new ORBextractor(nFeatures,fScaleFactor,nLevels,fIniThFAST,fMinThFAST);
    
    // 在单目初始化的时候，会用mpIniORBextractor来作为特征点提取器
    if(sensor==System::MONOCULAR)
        mpIniORBextractor = new ORBextractor(2*nFeatures,fScaleFactor,nLevels,fIniThFAST,fMinThFAST);
可以看到，其核心关键在于类 ORBextractor，前面的:

        (*mpORBextractorLeft)(im,cv::Mat(),mvKeys,mDescriptors);
        # 或者
        (*mpORBextractorRight)(im,cv::Mat(),mvKeysRight,mDescriptorsRight);
其本质上调用的是 src\ORBextractor.cc 文件中的 ORBextractor::operator() 函数。暂且不论，我们想来了解一下 [图像金字塔](https://so.csdn.net/so/search?q=图像金字塔&spm=1001.2101.3001.7020) 与 特征点的相关知识。

## 二、图像金字塔

在讨论构造函数 ORBextractor::ORBextractor() 之前，我们先了解一下什么是图像金字塔，这个东西呢，我们不用想得太复杂了，简单的说就是：

> ​	对一张图像进行连续的等比缩放(一般是缩小)，把多次缩放之后的图像加上原图，我们统称为图像金字塔

如下图所示:

![在这里插入图片描述](https://img-blog.csdnimg.cn/69b15216016642f09c0788654aa7a86e.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_20,color_FFFFFF,t_70,g_se,x_16#pic_center)

其上的 Level 0 表示原图， Level 1 则为 按照缩放因子 f 进行第一次缩放的结果，Level 2 则是在 Level 1 的基础上，按照放因子 f 再次进行缩放之后的结果。这样一次循环叠加，形成了上面的图像金字塔。使用图像金字塔，我们可以提取到图像各个尺寸的关键点，这样增加了算法的鲁棒性。了解图像金字塔之后，我们在来看看fast特征是什么。

## 三、ORB特征点

说到 fast特征 之前，我们先来说说ORB特征，特征点一般具备如下性质（SIFT、SURF、ORB等特征点）：

	可重复性：即相同的“区域“可以在不同的图像中找到（比如将特征点比作一只猫，在图一和图二中都能找到这只猫）。
	可区别性：即不同的”区域“有不同的表达。
	高效率：在同一副图像中，特征点的数量应该远小于像素的数量。
	本地性：特征仅与一小片图像区域相关。
特征点主要由关键点与描述子两个部分组成:
     **关键点:** 通常是指该特征点在图像中的位置，有的特征点还具有朝向、大小等信息。
     **描述子:** 通常是一个向量，按照认为设计的方式，描述了该关键点周围像素的信息。描述子的设计原则是外观相似的特征应该有相似的描述子。

ORB特征:特征也是由关键点和描述子组成。正如其英文全名一样，这种特征使用的特征点是”Oriented FAST“，描述子是”Rotated BRIEF“。其实这两种关键点与描述子都是在ORB特征出现之前就已经存在了，ORB特征的作者将二者进行了一定程度的改进，并将这两者巧妙地结合在一起，得出一种可以快速提取的特征－－ORB特征。ORB特征在速度方面相较于SIFT、SURF已经有明显的提升的同时，保持了特征子具有旋转与尺度不变性。

对于 Oriented FAST 关键点与 Rotated BRIEF 描述子，我们在下篇博客进行详细的讲解这篇博客我们先来看看代码。

## 四、代码实现

前言部分我们提到了 Frame::ExtractORB() 函数，本质上调用的是 src\ORBextractor.cc 文件中的 ORBextractor::operator() 函数，在讲解该函数之前，我们先来看看 ORBextractor::ORBextractor() 构造函数，其主要执行了以下流程:

	# ORBextractor.scaleFactor参数默认为1.2  
	ORBextractor.nLevels默认为8，表示8层金字塔
	1、获取每层金字塔的缩放因子，以及缩放因子的方平(主要用于面积计算)，缩放因子来自yaml配置文件中的 ORBextractor.scaleFactor 参数。
		(1)mvScaleFactor，mvInvScaleFactor = 每层金字塔缩放因子，缩放因子的倒数
	    (2)mvLevelSigma2，mvInvLevelSigma2 = 每层金字塔缩放因子平方，缩放因子平方的倒数
	
	# ORBextractor.nFeatures: 1000， 表示所有金字塔一共需要提取1000个特征点
	2、mnFeaturesPerLevel:用于存储每层图像金字塔应该提取的特征点数目，其分配方式主要根据面积进行计算。面积越大，提取的特征数目越多。如果按按面积分配特征点出现多余，未分配的特征点，默认分配给最后一层金字塔(最小的那一层)
	
	3、pattern0:其主要和描述子相关，暂时不做详细讲解
	   umax:其主要和描述子相关主要用于记录X的坐标的最大值，暂时不用理会即可
**也就是说其构造函数ORBextractor::ORBextractor() 完成了特征点提取的参数配置工作，包括设置每层图像金字塔需要提取的特征点数目，并为灰度质心法的运用计算所涉及到的坐标值umax，设置描述子计算的采样模板，以为特征点提取做准备，**其详细代码注释如下:

```
//特征点提取器的构造函数

ORBextractor::ORBextractor(int _nfeatures,		//指定要提取的特征点数目
                           float _scaleFactor,	//指定图像金字塔的缩放系数
                           int _nlevels,		//指定图像金字塔的层数
                           int _iniThFAST,		//指定初始的FAST特征点提取参数，可以提取出最明显的角点
                           int _minThFAST):		//如果初始阈值没有检测到角点，降低到这个阈值提取出弱一点的角点
    nfeatures(_nfeatures), scaleFactor(_scaleFactor), nlevels(_nlevels),
    iniThFAST(_iniThFAST), minThFAST(_minThFAST)//设置这些参数
{
    //存储每层图像缩放系数的vector调整为符合图层数目的大小
    mvScaleFactor.resize(nlevels);  
    //存储这个sigma^2，其实就是每层图像相对初始图像缩放因子的平方
    mvLevelSigma2.resize(nlevels);
    //对于初始图像，这两个参数都是1
    mvScaleFactor[0]=1.0f;
    mvLevelSigma2[0]=1.0f;
    //然后逐层计算图像金字塔中图像相当于初始图像的缩放系数 
    for(int i=1; i<nlevels; i++)  
    {
        //其实就是这样累乘计算得出来的
        mvScaleFactor[i]=mvScaleFactor[i-1]*scaleFactor;
        //原来这里的sigma^2就是每层图像相对于初始图像缩放因子的平方
        mvLevelSigma2[i]=mvScaleFactor[i]*mvScaleFactor[i];
    }

    //接下来的两个向量保存上面的参数的倒数
    mvInvScaleFactor.resize(nlevels);
    mvInvLevelSigma2.resize(nlevels);
    for(int i=0; i<nlevels; i++)
    {
        mvInvScaleFactor[i]=1.0f/mvScaleFactor[i];
        mvInvLevelSigma2[i]=1.0f/mvLevelSigma2[i];
    }
    
    //调整图像金字塔vector以使得其符合设定的图像层数
    mvImagePyramid.resize(nlevels);
    
    //每层需要提取出来的特征点个数，这个向量也要根据图像金字塔设定的层数进行调整
    mnFeaturesPerLevel.resize(nlevels);
    
    //图片降采样缩放系数的倒数
    float factor = 1.0f / scaleFactor;

    //第0层图像应该分配的特征点数量
    float nDesiredFeaturesPerScale = nfeatures*(1 - factor)/(1 - (float)pow((double)factor, (double)nlevels));
    
    //用于在特征点个数分配的，特征点的累计计数清空
    int sumFeatures = 0;
    //开始逐层计算要分配的特征点个数，顶层图像除外（看循环后面）
    for( int level = 0; level < nlevels-1; level++ )
    {
        //分配 cvRound : 返回个参数最接近的整数值
        mnFeaturesPerLevel[level] = cvRound(nDesiredFeaturesPerScale);
        //累计
        sumFeatures += mnFeaturesPerLevel[level];
        //乘系数
        nDesiredFeaturesPerScale *= factor;
    }
    //由于前面的特征点个数取整操作，可能会导致剩余一些特征点个数没有被分配，所以这里就将这个余出来的特征点分配到最高的图层中
    mnFeaturesPerLevel[nlevels-1] = std::max(nfeatures - sumFeatures, 0);
    
    //成员变量pattern的长度，也就是点的个数，这里的512表示512个点（上面的数组中是存储的坐标所以是256*2*2）
    const int npoints = 512;
    //获取用于计算BRIEF描述子的随机采样点点集头指针
    //注意到pattern0数据类型为Points*,bit_pattern_31_是int[]型，所以这里需要进行强制类型转换
    const Point* pattern0 = (const Point*)bit_pattern_31_;	
    //使用std::back_inserter的目的是可以快覆盖掉这个容器pattern之前的数据
    //其实这里的操作就是，将在全局变量区域的、int格式的随机采样点以cv::point格式复制到当前类对象中的成员变量中
    std::copy(pattern0, pattern0 + npoints, std::back_inserter(pattern));
    
    //This is for orientation
    //下面的内容是和特征点的旋转计算有关的
    // pre-compute the end of a row in a circular patch
    //预先计算圆形patch中行的结束位置
    //+1中的1表示那个圆的中间行
    umax.resize(HALF_PATCH_SIZE + 1);
    
    //cvFloor返回不大于参数的最大整数值，cvCeil返回不小于参数的最小整数值，cvRound则是四舍五入
    int v,		//循环辅助变量
        v0,		//辅助变量
        vmax = cvFloor(HALF_PATCH_SIZE * sqrt(2.f) / 2 + 1);	//计算圆的最大行号，+1应该是把中间行也给考虑进去了
                //NOTICE 注意这里的最大行号指的是计算的时候的最大行号，此行的和圆的角点在45°圆心角的一边上，之所以这样选择
                //是因为圆周上的对称特性
                
    //这里的二分之根2就是对应那个45°圆心角
    
    int vmin = cvCeil(HALF_PATCH_SIZE * sqrt(2.f) / 2);
    //半径的平方
    const double hp2 = HALF_PATCH_SIZE*HALF_PATCH_SIZE;
    
    //利用圆的方程计算每行像素的u坐标边界（max）
    for (v = 0; v <= vmax; ++v)
        umax[v] = cvRound(sqrt(hp2 - v * v));		//结果都是大于0的结果，表示x坐标在这一行的边界
    
    // Make sure we are symmetric
    //这里其实是使用了对称的方式计算上四分之一的圆周上的umax，目的也是为了保持严格的对称（如果按照常规的想法做，由于cvRound就会很容易出现不对称的情况，
    //同时这些随机采样的特征点集也不能够满足旋转之后的采样不变性了）
    for (v = HALF_PATCH_SIZE, v0 = 0; v >= vmin; --v)
    {
        while (umax[v0] == umax[v0 + 1])
            ++v0;
        umax[v] = v0;
        DEBUG("%d=%d", v, v0);
        ++v0;
    }
}
```

其中，最重要的代码为：

```cpp
    //第0层图像应该分配的特征点数量
    float nDesiredFeaturesPerScale = nfeatures*(1 - factor)/(1 - (float)pow((double)factor, (double)nlevels));
```

如何分配每一层提取的特征点数量？

金字塔层数越高，图像的面积越小，所能提取到的特征数量就越小。基于这个原理，我们可以按照面积将特征点均摊到金字塔每层的图像上。我们假设第0层图像的宽为W，长为L，缩放因子为s（这里的0<s<1)。那么整个金字塔总的面积为
$$
S=L×W+L×s×W×s+...+L×s^{n-1}×W×s^{n-1}
$$
![image-20220922104159876](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220922104159876.png)

那么，单位面积的特征点数量为

![image-20220922104236675](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220922104236675.png)

那么，第0层应分配的特征点数量为

![image-20220922104304962](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220922104304962.png)

接着推出了第$\alpha$层应分配的特征点数量为

![image-20220922104546537](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220922104546537.png)

实际上，opencv里的代码不是按照面积算的，而是按照边长来算的。也就是上面公式中的$[s^{2}]$换成s。

## 五、结语

该章节主要讲解什么金字塔，以及特征点。并且提及到特征点包含了关键点以及描述子两个部分，但是没有做详细的介绍，再下一篇博客中会进行具体的讲解。最后我们还了解了ORBextractor 的构造函数，主要为接下来的内容做铺垫。

# 5. ORBextractor::operator()→构建图像金字塔

## 一、前言

在上一篇博客中，简单的介绍了以下特征带点，以及 ORBextractor::ORBextractor() 构造函数。我们已经知道Frame::Frame()构建函数中会调用到 ORBextractor::operator() 函数，该张博客我们就来看看其函数的具体实现，其代码位于 src/ORBextractor.cc文件中。

## 二、代码流程

	根据输入的灰度图像，构建特征金字塔: ComputePyramid(image)
	使用四叉树的方式计算每层图像的关键点并且进行分配（均匀化）: ComputeKeyPointsOctTree(allKeypoints); 计算四叉树的特征点，函数名字后面的OctTree只是说明了在均匀化特征点时所使用的方式
	经过高斯模糊之后，生成关键点对应的描述子，并且计算出关键点的方向: computeDescriptors(workingMat,keypoints,esc,pattern);
代码的流程是十分简单的，其上的三个部分都是都是十分重要的，下面是 [operator](https://so.csdn.net/so/search?q=operator&spm=1001.2101.3001.7020)() 函数的代码注释。

## 三、源码注释
```
/**
 * @brief 用仿函数（重载括号运算符）方法来计算图像特征点
 * 
 * @param[in] _image                    输入原始图的图像
 * @param[in] _mask                     掩膜mask
 * @param[in & out] _keypoints                存储特征点关键点的向量
 * @param[in & out] _descriptors              存储特征点描述子的矩阵
 */
void ORBextractor::operator()( InputArray _image, InputArray _mask, vector<KeyPoint>& _keypoints,
                      OutputArray _descriptors)
{ 
    // Step 1 检查图像有效性。如果图像为空，那么就直接返回
    if(_image.empty())
        return;

    //获取图像的大小
    Mat image = _image.getMat();
    // UNDONE: 
    cv::imshow("src", image);
    system("mkdir -p result_images");
    cv::imwrite("result_images/src.jpg", image);
    //判断图像的格式是否正确，要求是单通道灰度值
    assert(image.type() == CV_8UC1 );

    // Pre-compute the scale pyramid
    // Step 2 构建图像金字塔
    ComputePyramid(image);

    // Step 3 计算图像的特征点，并且将特征点进行均匀化。均匀的特征点可以提高位姿计算精度
    // 存储所有的特征点，注意此处为二维的vector，第一维存储的是金字塔的层数，第二维存储的是那一层金字塔图像里提取的所有特征点
    vector < vector<KeyPoint> > allKeypoints; 
    //使用四叉树的方式计算每层图像的特征点并进行分配
    ComputeKeyPointsOctTree(allKeypoints);

    //使用传统的方法提取并平均分配图像的特征点，作者并未使用
    //ComputeKeyPointsOld(allKeypoints);

    
    // Step 4 拷贝图像描述子到新的矩阵descriptors
    Mat descriptors;

    //统计整个图像金字塔中的特征点
    int nkeypoints = 0;
    //开始遍历每层图像金字塔，并且累加每层的特征点个数
    for (int level = 0; level < nlevels; ++level)
        nkeypoints += (int)allKeypoints[level].size();
    
    //如果本图像金字塔中没有任何的特征点
    if( nkeypoints == 0 )
        //通过调用cv::mat类的.realse方法，强制清空矩阵的引用计数，这样就可以强制释放矩阵的数据了
        //参考[https://blog.csdn.net/giantchen547792075/article/details/9107877]
        _descriptors.release();
    else
    {
        //如果图像金字塔中有特征点，那么就创建这个存储描述子的矩阵，注意这个矩阵是存储整个图像金字塔中特征点的描述子的
        _descriptors.create(nkeypoints,		//矩阵的行数，对应为特征点的总个数
                            32, 			//矩阵的列数，对应为使用32*8=256位描述子
                            CV_8U);			//矩阵元素的格式
        //获取这个描述子的矩阵信息
        // ?为什么不是直接在参数_descriptors上对矩阵内容进行修改，而是重新新建了一个变量，复制矩阵后，在这个新建变量的基础上进行修改？
        descriptors = _descriptors.getMat();
    }

    //清空用作返回特征点提取结果的vector容器
    _keypoints.clear();
    //并预分配正确大小的空间
    _keypoints.reserve(nkeypoints);

    //因为遍历是一层一层进行的，但是描述子那个矩阵是存储整个图像金字塔中特征点的描述子，所以在这里设置了Offset变量来保存“寻址”时的偏移量，
    //辅助进行在总描述子mat中的定位
    int offset = 0;
    //开始遍历每一层图像
    for (int level = 0; level < nlevels; ++level)
    {
        //获取在allKeypoints中当前层特征点容器的句柄
        vector<KeyPoint>& keypoints = allKeypoints[level];
        //本层的特征点数
        int nkeypointsLevel = (int)keypoints.size();

        //如果特征点数目为0，跳出本次循环，继续下一层金字塔
        if(nkeypointsLevel==0)
            continue;
    
        // preprocess the resized image 
        //  Step 5 对图像进行高斯模糊
        // 深拷贝当前金字塔所在层级的图像
        Mat workingMat = mvImagePyramid[level].clone();
    
        // 注意：提取特征点的时候，使用的是清晰的原图像；这里计算描述子的时候，为了避免图像噪声的影响，使用了高斯模糊
        GaussianBlur(workingMat, 		//源图像
                     workingMat, 		//输出图像
                     Size(7, 7), 		//高斯滤波器kernel大小，必须为正的奇数
                     2, 				//高斯滤波在x方向的标准差
                     2, 				//高斯滤波在y方向的标准差
                     BORDER_REFLECT_101);//边缘拓展点插值类型
    
        // Compute the descriptors 计算描述子
        // desc存储当前图层的描述子
        Mat desc = descriptors.rowRange(offset, offset + nkeypointsLevel);
        // Step 6 计算高斯模糊后图像的描述子
        computeDescriptors(workingMat, 	//高斯模糊之后的图层图像
                           keypoints, 	//当前图层中的特征点集合
                           desc, 		//存储计算之后的描述子
                           pattern);	//随机采样模板
    
        // 更新偏移量的值 
        offset += nkeypointsLevel;
    
        // Scale keypoint coordinates
        // Step 6 对非第0层图像中的特征点的坐标恢复到第0层图像（原图像）的坐标系下
        // ? 得到所有层特征点在第0层里的坐标放到_keypoints里面
        // 对于第0层的图像特征点，他们的坐标就不需要再进行恢复了
        if (level != 0)
        {
            // 获取当前图层上的缩放系数
            float scale = mvScaleFactor[level];
            // 遍历本层所有的特征点
            for (vector<KeyPoint>::iterator keypoint = keypoints.begin(),
                 keypointEnd = keypoints.end(); keypoint != keypointEnd; ++keypoint)
                // 特征点本身直接乘缩放倍数就可以了
                keypoint->pt *= scale;
        }
        
        // And add the keypoints to the output
        // 将keypoints中内容插入到_keypoints 的末尾
        // keypoint其实是对allkeypoints中每层图像中特征点的引用，这样allkeypoints中的所有特征点在这里被转存到输出的_keypoints
        _keypoints.insert(_keypoints.end(), keypoints.begin(), keypoints.end());
    }
}
```

本人这里保存的金字塔的图像，展示如下:

ComputePyramid()的代码注释如下
```
@param image 输入图像，这个输入图像所有像素都是有效的，也就是说都是可以在其上提取出FAST角点的
void ORBextractor::ComputePyramid(cv::Mat image)
{
    //开始遍历所有的图层
    for (int level = 0; level < nlevels; ++level)
    {
        //获取本层图像的缩放系数
        float scale = mvInvScaleFactor[level];
        //计算本层图像的像素尺寸大小 
        Size sz(cvRound((float)image.cols*scale), cvRound((float)image.rows*scale));
        //全尺寸图像，包括无效图像区域的大小。实际上作者这样做是将图像进行“裁边”，EDGE_THRESHOLD区域内的图像不进行FAST角点检测，全尺寸图像示意图在代码下方
        Size wholeSize(sz.width + EDGE_THRESHOLD*2, sz.height + EDGE_THRESHOLD*2);
        // 定义了两个变量：temp是扩展了边界的图像，masktemp看上去是要作为掩膜，但后者其实在这个程序中并没有被使用到
        Mat temp(wholeSize, image.type()), masktemp; 
        // mvImagePyramid 刚开始时是个空的vector<Mat>
        // 把图像金字塔该图层的图像复制给temp，或者说把图像金字塔该图层的图像指针mvImagePyramid指向temp的中间部分（这里为浅拷贝，内存相同），关于深拷贝、浅拷贝的讲解见下方.
        mvImagePyramid[level] = temp(Rect(EDGE_THRESHOLD, EDGE_THRESHOLD, sz.width, sz.height));

#ifdef _DEBUG 
        // UNDONE: 
        ostringstream buffer;
        buffer << "EDGE_mvImagePyramid_" << level << ".jpg";
        string imageFile = buffer.str();
        string imagePath = "result_images/" + imageFile;
        //cv::imshow(imageFile, mvImagePyramid[level]);
        system("mkdir -p result_images");
        cv::imwrite(imagePath, mvImagePyramid[level]);
        //cv::waitKey();
#endif
        // Compute the resized image
        //计算第0层以上resize后的图像
        if( level != 0 )
        {
            //resize将上一层金字塔图像根据设定sz缩放到当前层级
            resize(mvImagePyramid[level-1],	//输入图像
                   mvImagePyramid[level], 	//输出图像
                   sz, 						//输出图像的尺寸
                   0, 						//水平方向上的缩放系数，留0表示自动计算
                   0,  						//垂直方向上的缩放系数，留0表示自动计算
                   cv::INTER_LINEAR);		//图像缩放的差值算法类型，这里的是线性插值算法
    
            //copyMakeBorder是opencv中的库函数，该函数定义是把源图像拷贝到目的图像的中央，四面填充指定的像素。图片如果已经拷贝到中间，只填充边界，见下图
            //这样做是为了能够正确提取边界的FAST角点
            //EDGE_THRESHOLD指的这个边界的宽度，由于这个边界之外的像素不是原图像素而是算法生成出来的，所以不能够在EDGE_THRESHOLD之外提取特征点			
            copyMakeBorder(mvImagePyramid[level], 					//源图像
                           temp, 									//目标图像（此时其实就已经有大了一圈的尺寸了）
                           EDGE_THRESHOLD, EDGE_THRESHOLD, 			//top & bottom 需要扩展的border大小
                           EDGE_THRESHOLD, EDGE_THRESHOLD,			//left & right 需要扩展的border大小
                           BORDER_REFLECT_101+BORDER_ISOLATED);     //扩充方式，opencv给出的解释，见下图：
            
            /*Various border types, image boundaries are denoted with '|' 
            * BORDER_REPLICATE:     aaaaaa|abcdefgh|hhhhhhh
            * BORDER_REFLECT:       fedcba|abcdefgh|hgfedcb
            * BORDER_REFLECT_101:   gfedcb|abcdefgh|gfedcba
            * BORDER_WRAP:          cdefgh|abcdefgh|abcdefg
            * BORDER_CONSTANT:      iiiiii|abcdefgh|iiiiiii  with some specified 'i'
            */
            
            //BORDER_ISOLATED	表示对整个图像进行操作
            // https://docs.opencv.org/3.4.4/d2/de8/group__core__array.html#ga2ac1049c2c3dd25c2b41bffe17658a36
    
        }
        else
        {
       //对于第0层未缩放图像（底层图像），直接就扩充边界了，如下图；
       //temp输出并没有拷贝给mvImagePyramid；
       //直接将图像深拷贝到temp的中间，并且对其周围进行边界扩展。此时temp就是对原图扩展后的图像；
            copyMakeBorder(image,			//这里是货真价实的原图像啊
                           temp, EDGE_THRESHOLD, EDGE_THRESHOLD, EDGE_THRESHOLD, EDGE_THRESHOLD,
                           BORDER_REFLECT_101);           
        }
        // ! 原代码mvImagePyramid 并未扩充，应该添加下面一行代码？实际上不需要，因为已经经历了浅拷贝
        //mvImagePyramid[level] = temp;
    }
}
```

![image-20220926102116064](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220926102116064.png)

![image-20220926192251881](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220926192251881.png)

![image-20220926192450664](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220926192450664.png)

-------------------------------------插播介绍C++深拷贝和浅拷贝-----------------------------------------------------------------------

**浅拷贝：简单的赋值拷贝操作（这个是系统默认提供的）**

**深拷贝：在堆区重新开辟空间，进行拷贝操作（要自己写的）**

**简单来说，只要类属性里有指针等就必须利用深拷贝操作**

浅拷贝是调用默认拷贝构造函数，简单把把一个对象中的值传递给了另外一个，两个对象具有同一个地址，可以理解为这两个对象就是同一个对象，改变其中一个的值另外一个的值也改变；而深拷贝是自己写一个拷贝构造函数，重新开辟一个内存空间存放数据（可以理解为临时增加一个new对象），只对对象的值进行传递，因此两个对象具有不同的地址，而具有相同的值，这样在堆空间中new出来的数据不会因为二次释放同一块内存会被编译器视为非法操作；可知，浅拷贝带来的问题就是堆区的内存被重复释放。因此说，只要类属性里有指针等就必须利用深拷贝操作。

为便于深刻理解，先看一个实例：
```
#include<iostream>
using namespace std;
class Person                                  ******Person类分隔符*******
{
public:
	Person(){
		cout<<"默认构造函数的调用\n";
	}
	Person(int age)
	{
		cout<<"有参构造函数的调用\n";
		m_age=age;
	}	
	~Person()
	{
		cout<<"析构函数的调用\n";
	}

    int m_age;
};                                           ******Person类分隔符*******
int main()
{
	Person a(10);
	Person b(a);
	
	cout<<"a的年龄为："<<a.m_age<<endl;
	cout<<"b的年龄为："<<b.m_age<<endl; 
}
```
运行结果：

![img](https://img-blog.csdnimg.cn/e8fa17eecb3441feb5d8e4e95ecf016e.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5Y2O54Gv5Yid5LiKYA==,size_20,color_FFFFFF,t_70,g_se,x_16)

运行结果显然没有问题，我们在初始化类b的时候，系统调用了默认拷贝构造函数，把a中的值传递给了b；下面，我们增加类Person的一个指针属性m_height，并且把身高属性放置到堆区：

**tip：堆区由程序员手动开创与释放，new出来的数据就位于堆区**

```
#include<iostream>
using namespace std;
class Person                                 ******Person类分隔符*******
{
public:
	Person(){
		cout<<"默认构造函数的调用\n";
	}
	Person(int age,int height)
	{
		cout<<"有参构造函数的调用\n";
		m_age=age;
        m_height=new int(height);    //**
	}	
	~Person()
	{
		cout<<"析构函数的调用\n";
	}

    int m_age;
    int* m_height;
};                                           ******Person类分隔符*******
int main()
{
	Person a(19,170);
	Person b(a);
	
	cout<<"a的年龄为："<<a.m_age<<"  a的身高为："<<*a.m_height<<endl;
	cout<<"b的年龄为："<<b.m_age<<"  b的身高为："<<*b.m_height<<endl; 
}
```
运行结果为：

![img](https://img-blog.csdnimg.cn/a83e114c7acf4bb69b0d7247ee225ff4.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5Y2O54Gv5Yid5LiKYA==,size_20,color_FFFFFF,t_70,g_se,x_16)

 运行结果看起来没问题，但是代码实际上是不规范的；堆区的数据需要程序员手动开创与释放，需要用到析构函数，我们进行析构代码的完善：

```
~Person()
	{
        //析构代码，将堆区开辟的数据释放干净
        if(m_height!=NULL)
        {
            delete m_height;
            m_height=NULL;    //防止指针滞空变成野指针
        }
		cout<<"析构函数的调用\n";
	}
```

但当继续运行程序的时候会发现代码已经崩了（部分编译器现象不明显，如Dev，这里不再截图），什么原因导致的？我们分析一下：

![img](https://img-blog.csdnimg.cn/749cb5ae7186485abc40edb6f8360f83.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5Y2O54Gv5Yid5LiKYA==,size_20,color_FFFFFF,t_70,g_se,x_16)

**二次释放同一块内存会被编译器视为非法操作；可知，浅拷贝带来的问题就是堆区的内存被重复释放。**解决这个问题就需要利用[深拷贝](https://so.csdn.net/so/search?q=深拷贝&spm=1001.2101.3001.7020)，具体做法便是重新开辟一个内存空间存放数据，利用深拷贝构造函数使b的指针指向新开辟的空间，这样，a与b各指向一块堆区的数据，两块空间存放的数据是一样的，这样就在拷贝的同时避免了对同一空间的重复释放问题。

![img](https://img-blog.csdnimg.cn/30246c6e21d84fadb05e6fdd77fe645a.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5Y2O54Gv5Yid5LiKYA==,size_20,color_FFFFFF,t_70,g_se,x_16)具体代码实现如下：

```
#include<iostream>
using namespace std;
class Person                                             *****类Person***** 
{
public:
	Person(){
		cout<<"默认构造函数的调用\n";
	}
	Person(int age,int height)
	{
		cout<<"有参构造函数的调用\n";
		m_age=age;
        m_height=new int(height);
	}	
	
	Person(const Person& p)
	{
		cout<<"拷贝构造函数的调用"<<endl;
		m_age=p.m_age;
		
		m_height=new int(*p.m_height);//                <-深拷贝操作 
		//编译器提供的拷贝函数是:m_height=p.m_height;     <-浅拷贝
	    //由此可见浅拷贝中a，b指向的是同一块内存
	}
	
	~Person()
	{
		if(m_height!=NULL)
	    {
	        delete m_height;
	        m_height=NULL;    //防止指针滞空变成野指针
	    }
		cout<<"析构函数的调用\n";
	}
	 
	int m_age;
	int* m_height;
};                                                       *****类Person*****
int main()
{
	Person a(19,170);
	Person b(a);
	
	cout<<"a的年龄为："<<a.m_age<<"  a的身高为："<<*a.m_height<<endl;
	cout<<"b的年龄为："<<b.m_age<<"  b的身高为："<<*b.m_height<<endl; 
}
```

运行结果为：

![img](https://img-blog.csdnimg.cn/80d471eba72844ba91e15c869aa175e9.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5Y2O54Gv5Yid5LiKYA==,size_20,color_FFFFFF,t_70,g_se,x_16)

下面附两张截图：

[浅拷贝](https://so.csdn.net/so/search?q=浅拷贝&spm=1001.2101.3001.7020)时两个对象height属性的地址：

<img src="https://img-blog.csdnimg.cn/758c10fdcbbe41c5b7786c3ffb230db1.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5Y2O54Gv5Yid5LiKYA==,size_20,color_FFFFFF,t_70,g_se,x_16" alt="img" style="zoom:50%;" />

 深拷贝时两个对象height的地址：

<img src="https://img-blog.csdnimg.cn/e0600de7aeaf4675b6c118246c03ba5a.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5Y2O54Gv5Yid5LiKYA==,size_20,color_FFFFFF,t_70,g_se,x_16" alt="img" style="zoom:50%;" />

-------------------------------------插播介绍C++深拷贝和浅拷贝**结束**-----------------------------------------------------------------------

本人对 mvImagePyramid 图像金字塔进行了保存(由于屏幕大小，本人只保存了6层,实际共8成)，展示如下：

![在这里插入图片描述](https://img-blog.csdnimg.cn/78ace807c4884d5fadba4bf22746c48f.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_20,color_FFFFFF,t_70,g_se,x_16#pic_center)

在上面的代码中，我们可以看到一个参数 EDGE_THRESHOLD = 19, 这里是为了对图片进行填充，后续进行特征提取时，尽量利用到图像的每个像素，本人取消 ORBextractor::ComputePyramid()函数中代码

```
	mvImagePyramid[level] = temp;
```

![在这里插入图片描述](https://img-blog.csdnimg.cn/173366172fff42eea2adb743e114e202.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_13,color_FFFFFF,t_70,g_se,x_16#pic_center)

可以很明显的看到，该图像的周边填充了16行列像素，其填充结果是对称于红线的。

## 四、结语

这篇博客主要讲解了图像金字塔的具体构建过程，并且知道图像金字塔存储于变量 mvImagePyramid 之后，后续我们会基于金字塔进行特征点提取。也就是接下来需要讲解的核心内容了

# 6. ORBextractor::operator()→FAST关键点提取

## 一、前言

上一篇博客，我们对 ORBextractor::operator() 中调用的ComputePyramid(image)函数进行了讲解，那么接下来我们就是要分析紧跟在其后面的 ComputeKeyPointsOctTree(allKeypoints) 函数进行讲解。其代码实现位于src/ORBextractor.cc文件中。在讲解代码之前，我们先来具体的了解一下ORC特征，前面的博客我们提及到ORB特征主要由 Oriented FAST 关键点与 Rotated BRIEF 描述子两个部分组成。

## 二、FAST关键点

我们先来了解一下什么四FAST关键。判断一个像素点是否为FAST关键点，主要流程如下：

	1、选取像素p，假设它的亮度为Ip；
	
	2、设置一个阈值T（比如Ip的20%）；
	
	3、以像素p为中心，选取半径为3的圆上的16个像素点；
	
	4、假如选取的圆上，有连续的N个点的亮度大于Ip+T或小于Ip-T，那么像素p可以被认为是关键点；
	通常取12，即为FAST-12。其他常用的N取值为9和11，它们分别被称为FAST-9，FAST-11)。
	
	5、循环以上4步，对每一个像素执行相同操作。
简单的图示如下:

![在这里插入图片描述](https://img-blog.csdnimg.cn/548c8f43782f4b1dbf11bb01632524c3.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_20,color_FFFFFF,t_70,g_se,x_16#pic_center)

FAST是一种角点，主要检测局部像素灰度变化明显的地方，以速度快著称。它的思想是:如果一个像素与它邻域的像素差别较大(过亮或过暗),那它更可能是角点。

在FAST-12算法中，为了更高效，可以添加一项预测试操作，以快速地排除绝大多数不是角点的像素。具体操作为，对于每个像素，直接检测邻域圆上的第1，5，9，13个像素的亮度。只有当这四个像素中有三个同时大于Ip+T或小于Ip-T时，当前像素才有可能是一个角点，否则应该直接排除。这样的预测试操作大大加速了角点检测。此外，原始的FAST角点经常出现“扎堆”的现象。所以在第一遍检测之后，还需要用非极大值抑制(Non-maximal suppression)，在一定区域内仅保留响应极大值的角点， 避免角点集中的问题。

**FAST特征点的计算仅仅是比较像素间亮度的差异，速度非常快，**但它也有一些问题如下:

F A S T 特征点数量很大且不确定 →而我们往往希望对图像提取固定数量的特征。因此，在ORB中，对原始的FAST算法进行了改进。 我们可以指定最终要提取的角点数量N，对原始FAST角点分别计算Harris响应值，然后选取前N 个具有最大响应值的角点，作为最终的角点集合。

F A S T 角点不具有方向信息 → 由于它固定取半径为3的圆，存在尺度问题 :远处看着像是角点的地方，接近后看可能就不是角点了。针对FAST角点不具有方向性和尺度的弱点，ORB添加了尺度和旋转的描述。尺度不变性由构建图像金字塔，并在金字塔的每一层上检测角点来实现。而特征的旋转是由灰度质心法(Intensity Centroid)实现的

金字塔我们已经在前面进行了讲解，对于灰度质心法(Intensity Centroid)稍后会进行讲解

## 三、代码流程

	 //用于存储所有的关键点信息
	allKeypoints.resize(nlevels);
	
	//循环对图像金字塔进行处理
	for (int level = 0; level < nlevels; ++level)
		1、把图像分割成栅格，栅格为正方形，边长像素为W=30
		
		2、循环遍历每个网格
		for(int i=0; i<nRows; i++)
			for(int j=0; j<nCols; j++)
			
			3、设定初始对单个栅格进行FAST关键点提取
			FAST(mvImagePyramid[level].rowRange(iniY,maxY).colRange(iniX,maxX),vKeysCell,iniThFAST,true);
			
			4、如果没有检测到任何关键点，则降低阈值再进行检测
			FAST(mvImagePyramid[level].rowRange(iniY,maxY).colRange(iniX,maxX),vKeysCell,minThFAST,true);

其上大家要注意一个点，就是 为什么要划分成栅格的形式进行关键点提取，而不是直接送入整张图像进行检测。因为一张图像的各个区域的可区分度是不一样的，有的区域像素值差距比较大，直接使用 iniThFAST 作为检测阈值即可，但是有的区域的像素值差距比较小，这时可区分度是比较小的，所以需要设定比较小的阈值。该是一个比较细节的地方，大家需要多思考一下。

## 四、代码实现
```
代码实现位于src/ORBextractor.cc文件中，该函数没有全部粘贴，只给出了关于FAST的相关部分

//计算四叉树的特征点，函数名字后面的OctTree只是说明了在过滤和分配特征点时所使用的方式
void ORBextractor::ComputeKeyPointsOctTree(
    vector<vector<KeyPoint> >& allKeypoints)	//所有的特征点，这里第一层vector存储的是某图层里面的所有特征点，
                                                //第二层存储的是整个图像金字塔中的所有图层里面的所有特征点
{
    //重新调整图像层数
    allKeypoints.resize(nlevels);

    //图像cell的尺寸，是个正方形，可以理解为边长in像素坐标
    const float W = 30;
    
    // 对每一层图像做处理
    //遍历所有图像
    for (int level = 0; level < nlevels; ++level)
    {

#ifdef _DEBUG 
        // UNDONE: 
        ostringstream buffer;
        buffer << "mvImagePyramid_" << level << ".jpg";
        string imageFile = buffer.str();
        string imagePath = "result_images/" + imageFile;
        //cv::imshow(imageFile, mvImagePyramid[level]);
        system("mkdir -p result_images");
        cv::imwrite(imagePath, mvImagePyramid[level]);
        //cv::waitKey();

#endif
        //计算这层图像的坐标边界， NOTICE 注意这里是坐标边界，EDGE_THRESHOLD指的应该是可以提取特征点的有效图像边界，后面会一直使用“有效图像边界“这个自创名词
        const int minBorderX = EDGE_THRESHOLD-3;			//这里的3是因为在计算FAST特征点的时候，需要建立一个半径为3的圆
        const int minBorderY = minBorderX;					//minY的计算就可以直接拷贝上面的计算结果了
        const int maxBorderX = mvImagePyramid[level].cols-EDGE_THRESHOLD+3;
        const int maxBorderY = mvImagePyramid[level].rows-EDGE_THRESHOLD+3;

        //存储需要进行平均分配的特征点
        vector<cv::KeyPoint> vToDistributeKeys;
        //一般地都是过量采集，所以这里预分配的空间大小是nfeatures*10
        vToDistributeKeys.reserve(nfeatures*10);
    
        //计算进行特征点提取的图像区域尺寸
        const float width = (maxBorderX-minBorderX);
        const float height = (maxBorderY-minBorderY);
    
        //计算网格在当前层的图像有的行数和列数
        const int nCols = width/W;
        const int nRows = height/W;
        //计算每个图像网格所占的像素行数和列数
        const int wCell = ceil(width/nCols);
        const int hCell = ceil(height/nRows);
    
        //开始遍历图像网格，还是以行开始遍历的
        for(int i=0; i<nRows; i++)
        {
            //计算当前网格初始行坐标
            const float iniY =minBorderY+i*hCell;
            //计算当前网格最大的行坐标，这里的+6=+3+3，即考虑到了多出来3是为了cell边界像素进行FAST特征点提取用
            //前面的EDGE_THRESHOLD指的应该是提取后的特征点所在的边界，所以minBorderY是考虑了计算半径时候的图像边界
            //目测一个图像网格的大小是25*25啊
            float maxY = iniY+hCell+6;
    
            //如果初始的行坐标就已经超过了有效的图像边界了，这里的“有效图像”是指原始的、可以提取FAST特征点的图像区域
            if(iniY>=maxBorderY-3)
                //那么就跳过这一行
                continue;
            //如果图像的大小导致不能够正好划分出来整齐的图像网格，那么就要委屈最后一行了
            if(maxY>maxBorderY)
                maxY = maxBorderY;
    
            //开始列的遍历
            for(int j=0; j<nCols; j++)
            {
                //计算初始的列坐标
                const float iniX =minBorderX+j*wCell;
                //计算这列网格的最大列坐标，+6的含义和前面相同
                float maxX = iniX+wCell+6;
                //判断坐标是否在图像中
                //如果初始的列坐标就已经超过了有效的图像边界了，这里的“有效图像”是指原始的、可以提取FAST特征点的图像区域。
                //并且应该同前面行坐标的边界对应，都为-3
                //!BUG  正确应该是maxBorderX-3
                if(iniX>=maxBorderX-6)
                    continue;
                //如果最大坐标越界那么委屈一下
                if(maxX>maxBorderX)
                    maxX = maxBorderX;
    
                // FAST提取兴趣点, 自适应阈值
                //这个向量存储这个cell中的特征点
                vector<cv::KeyPoint> vKeysCell;
                //调用opencv的库函数来检测FAST角点
                FAST(mvImagePyramid[level].rowRange(iniY,maxY).colRange(iniX,maxX),	//待检测的图像，这里就是当前遍历到的图像块
                     vKeysCell,			//存储角点位置的容器
                     iniThFAST,			//检测阈值
                     true);				//使能非极大值抑制
    
                //如果这个图像块中使用默认的FAST检测阈值没有能够检测到角点
                if(vKeysCell.empty())
                {
                    //那么就使用更低的阈值来进行重新检测
                    FAST(mvImagePyramid[level].rowRange(iniY,maxY).colRange(iniX,maxX),	//待检测的图像
                         vKeysCell,		//存储角点位置的容器
                         minThFAST,		//更低的检测阈值
                         true);			//使能非极大值抑制
                }
    
                //当图像cell中检测到FAST角点的时候执行下面的语句
                if(!vKeysCell.empty())
                {
                    //遍历其中的所有FAST角点
                    for(vector<cv::KeyPoint>::iterator vit=vKeysCell.begin(); vit!=vKeysCell.end();vit++)
                    {
                        //NOTICE 到目前为止，这些角点的坐标都是基于图像cell的，现在我们要先将其恢复到当前的【坐标边界】下的坐标
                        //这样做是因为在下面使用八叉树法整理特征点的时候将会使用得到这个坐标
                        //在后面将会被继续转换成为在当前图层的扩充图像坐标系下的坐标
                        (*vit).pt.x+=j*wCell;
                        (*vit).pt.y+=i*hCell;
                        //然后将其加入到”等待被分配“的特征点容器中
                        vToDistributeKeys.push_back(*vit);
                    }//遍历图像cell中的所有的提取出来的FAST角点，并且恢复其在整个金字塔当前层图像下的坐标
                }//当图像cell中检测到FAST角点的时候执行下面的语句
            }//开始遍历图像cell的列
        }//开始遍历图像cell的行
        //声明一个对当前图层的特征点的容器的引用
        vector<KeyPoint> & keypoints = allKeypoints[level];
        //并且调整其大小为欲提取出来的特征点个数（当然这里也是扩大了的，因为不可能所有的特征点都是在这一个图层中提取出来的）
        keypoints.reserve(nfeatures);
    
        // 根据mnFeatuvector<KeyPoint> & keypoints = allKeypoints[level];resPerLevel,即该层的兴趣点数,对特征点进行剔除
        //返回值是一个保存有特征点的vector容器，含有剔除后的保留下来的特征点
        //得到的特征点的坐标，依旧是在当前图层下来讲的
        keypoints = DistributeOctTree(vToDistributeKeys, 			//当前图层提取出来的特征点，也即是等待剔除的特征点
                                                                    //NOTICE 注意此时特征点所使用的坐标都是在“半径扩充图像”下的
                                      minBorderX, maxBorderX,		//当前图层图像的边界，而这里的坐标却都是在“边缘扩充图像”下的
                                      minBorderY, maxBorderY,
                                      mnFeaturesPerLevel[level], 	//希望保留下来的当前层图像的特征点个数
                                      level);						//当前层图像所在的图层
    
        //PATCH_SIZE是对于底层的初始图像来说的，现在要根据当前图层的尺度缩放倍数进行缩放得到缩放后的PATCH大小 和特征点的方向计算有关
        const int scaledPatchSize = PATCH_SIZE*mvScaleFactor[level];
    
        // Add border to coordinates and scale information
        //获取剔除过程后保留下来的特征点数目
        const int nkps = keypoints.size();
        //然后开始遍历这些特征点，恢复其在当前图层图像坐标系下的坐标
        for(int i=0; i<nkps ; i++)
        {
            //对每一个保留下来的特征点，恢复到相对于当前图层“边缘扩充图像下”的坐标系的坐标
            keypoints[i].pt.x+=minBorderX;
            keypoints[i].pt.y+=minBorderY;
            //记录特征点来源的图像金字塔图层
            keypoints[i].octave=level;
            //记录计算方向的patch，缩放后对应的大小， 又被称作为特征点半径
            keypoints[i].size = scaledPatchSize;
        }
    }
    
    // compute orientations
    //然后计算这些特征点的方向信息，注意这里还是分层计算的
    for (int level = 0; level < nlevels; ++level)
        computeOrientation(mvImagePyramid[level],	//对应的图层的图像
                           allKeypoints[level], 	//这个图层中提取并保留下来的特征点容器
                           umax);					//以及PATCH的横坐标边界		

}
```

在代码中还有一个比较细节的问题，我们可以看到其进行了两次 FAST关键点提取，第一次使用 iniThFAST 为检测阈值，第二次使用 minThFAST 为检测阈值。他的用意也是十分简单的，单使用 iniThFAST 为检测阈值的时候，如果没有检测到任何关键点，即 vKeysCell.empty() 成立，说明当前图像的区分度比较小，我们需要降低 iniThFAST，使用minThFAST 为阈值来进行检测。

另外我们可以看到代码中的 mvImagePyramid[level].rowRange(iniY,maxY).colRange(iniX,maxX) 的操作，该操作就是先类似于图片剪切的效果。本人保存了一些截图之后的图像如下:

![在这里插入图片描述](https://img-blog.csdnimg.cn/2dffba4b804842c3b4715a02f4eff618.png#pic_center)

可以很明显的知道其是下图左上角的一部分:

![请添加图片描述](https://img-blog.csdnimg.cn/b46c6f37f65a4fd2885ddeb60f9fa6f1.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_13,color_FFFFFF,t_70,g_se,x_16)

四、结语

上面是关于 FAST关键点提取的讲解，但是这里存在一个问题，就是一张图像提取出来的特征是分布不均匀的，有的可能特别密集，有的可能很稀疏。比如一共需要提取300个关键点，如果很多关键点都集中在一个区域，其他区域关键点很少。那么我们这300个关键点，肯定大部分都来自于密集区域吗，那么这种时候我们应该如何处理呢?

其上有个地方，我们可以看到，提取出来的关键点都存储在变量 vToDistributeKeys 变量之中，从命令可以看到这些关键点还需要进行重新分配(均匀分配)。也就是随后调用的 DistributeOctTree() 函数，在下一篇博客我们会对齐进行详细的讲解。

# 7. ORBextractor::operator()→FAST关键点均匀化

## 一、前言

在上一篇博客中的末尾，我们提到:一般来说一张图像提取出来的关键点是非常多的，且分布不均匀，有的区域很密集，有的区域很稀疏。那么我们就需要进行均匀化处理，处理之前与处理之后的结果对比如下:

**原始关键点**

![在这里插入图片描述](https://img-blog.csdnimg.cn/dba34575172540d88ea227e843752997.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_11,color_FFFFFF,t_70,g_se,x_16#pic_center)

**均匀化关键点**

![在这里插入图片描述](https://img-blog.csdnimg.cn/0f632ff10a40455ca381312c0e37812b.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_11,color_FFFFFF,t_70,g_se,x_16#pic_center)

从上面可以看到，原始密集区域的关键点已经被清除了好些，下面我们就再来看看其具体实现过程

## 二、实现逻辑
```
其主要函数为 src/ORBextractor.cc 中的 ORBextractor::DistributeOctTree() 函数，其主要使用四叉数的方式实现。主要逻辑如下:

	1、如果图片的宽度比较宽，就先把分成左右w/h份。一般的640×480的图像开始的时候只有一个
node。
	2、如果node里面的点数>1，把每个node分成四个node，如果node里面的特征点为空，就不要了，
删掉。
	3、新分的node的点数>1，就再分裂成4个node。如此，一直分裂。
	4、终止条件为：node的总数量> [公式] ，或者无法再进行分裂。
	5、然后从每个node里面选择一个质量最好的FAST点。
```

对应的步骤如下:

![在这里插入图片描述](https://img-blog.csdnimg.cn/6426e5e6ef19443f8c449327c4e3e4c5.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_20,color_FFFFFF,t_70,g_se,x_16#pic_center)

![在这里插入图片描述](https://img-blog.csdnimg.cn/fc3840eb42e74c22bec73785645f42b6.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_20,color_FFFFFF,t_70,g_se,x_16#pic_center)

![在这里插入图片描述](https://img-blog.csdnimg.cn/0c14cfe8a3934092960a3945958b7e3e.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_20,color_FFFFFF,t_70,g_se,x_16)

![在这里插入图片描述](https://img-blog.csdnimg.cn/6ecae93f177a4e8a9d3abf147ecdd915.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_20,color_FFFFFF,t_70,g_se,x_16)



![在这里插入图片描述](https://img-blog.csdnimg.cn/35eef9db83cb4664b525494f2a41edec.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_20,color_FFFFFF,t_70,g_se,x_16)简单的来说，就是把图像分成四个区域，判断每个区域关键点数，如果该区域关键点数目等于1或者0，则该区域下次则不在进行重分，其他的区域再划分层四个区域，判断其中每个区域关键点数目是否等于1或者0，依次重复。直到每个区域的关键点数目等于1或者0。或者重分次数达到指定次数也会停止。

## 三、代码实现
```
vector<cv::KeyPoint> ORBextractor::DistributeOctTree(const vector<cv::KeyPoint>& vToDistributeKeys, const int &minX,
                                       const int &maxX, const int &minY, const int &maxY, const int &N, const int &level)
{
    // Compute how many initial nodes
    // Step 1 根据宽高比确定初始节点数目
    //计算应该生成的初始节点个数，根节点的数量nIni是根据边界的宽高比值确定的，一般是1或者2
    // ! bug: 如果宽高比小于0.5，nIni=0, 后面hx会报错
    const int nIni = round(static_cast<float>(maxX-minX)/(maxY-minY));

    //一个初始的节点的x方向有多少个像素
    const float hX = static_cast<float>(maxX-minX)/nIni;
    
    //存储有提取器节点的链表
    list<ExtractorNode> lNodes;
    
    //存储初始提取器节点指针的vector
    vector<ExtractorNode*> vpIniNodes;
    
    //重新设置其大小
    vpIniNodes.resize(nIni);
    
    // Step 2 生成初始提取器节点
    for(int i=0; i<nIni; i++)
    {      
        //生成一个提取器节点
        ExtractorNode ni;
    
        //设置提取器节点的图像边界
        //注意这里和提取FAST角点区域相同，都是“半径扩充图像”，特征点坐标从0 开始 
        ni.UL = cv::Point2i(hX*static_cast<float>(i),0);    //UpLeft
        ni.UR = cv::Point2i(hX*static_cast<float>(i+1),0);  //UpRight
        ni.BL = cv::Point2i(ni.UL.x,maxY-minY);		        //BottomLeft
        ni.BR = cv::Point2i(ni.UR.x,maxY-minY);             //BottomRight
    
        //重设vkeys大小
        ni.vKeys.reserve(vToDistributeKeys.size());
    
        //将刚才生成的提取节点添加到链表中
        //虽然这里的ni是局部变量，但是由于这里的push_back()是拷贝参数的内容到一个新的对象中然后再添加到列表中
        //所以当本函数退出之后这里的内存不会成为“野指针”
        lNodes.push_back(ni);
        //存储这个初始的提取器节点句柄
        vpIniNodes[i] = &lNodes.back();
    }
    
    //Associate points to childs
    // Step 3 将特征点分配到子提取器节点中
    for(size_t i=0;i<vToDistributeKeys.size();i++)
    {
        //获取这个特征点对象
        const cv::KeyPoint &kp = vToDistributeKeys[i];
        //按特征点的横轴位置，分配给属于那个图像区域的提取器节点（最初的提取器节点）
        vpIniNodes[kp.pt.x/hX]->vKeys.push_back(kp);
    }
    
    // Step 4 遍历此提取器节点列表，标记那些不可再分裂的节点，删除那些没有分配到特征点的节点
    // ? 这个步骤是必要的吗？感觉可以省略，通过判断nIni个数和vKeys.size() 就可以吧
    list<ExtractorNode>::iterator lit = lNodes.begin();
    while(lit!=lNodes.end())
    {
        //如果初始的提取器节点所分配到的特征点个数为1
        if(lit->vKeys.size()==1)
        {
            //那么就标志位置位，表示此节点不可再分
            lit->bNoMore=true;
            //更新迭代器
            lit++;
        }
        ///如果一个提取器节点没有被分配到特征点，那么就从列表中直接删除它
        else if(lit->vKeys.empty())
            //注意，由于是直接删除了它，所以这里的迭代器没有必要更新；否则反而会造成跳过元素的情况
            lit = lNodes.erase(lit);			
        else
            //如果上面的这些情况和当前的特征点提取器节点无关，那么就只是更新迭代器 
            lit++;
    }
    
    //结束标志位清空
    bool bFinish = false;
    
    //记录迭代次数，只是记录，并未起到作用
    int iteration = 0;
    
    //声明一个vector用于存储节点的vSize和句柄对
    //这个变量记录了在一次分裂循环中，那些可以再继续进行分裂的节点中包含的特征点数目和其句柄
    vector<pair<int,ExtractorNode*> > vSizeAndPointerToNode;
    
    //调整大小，这里的意思是一个初始化节点将“分裂”成为四个
    vSizeAndPointerToNode.reserve(lNodes.size()*4);
    
    // Step 5 利用四叉树方法对图像进行划分区域，均匀分配特征点
    while(!bFinish)
    {
        //更新迭代次数计数器，只是记录，并未起到作用
        iteration++;
    
        //保存当前节点个数，prev在这里理解为“保留”比较好
        int prevSize = lNodes.size();
    
        //重新定位迭代器指向列表头部
        lit = lNodes.begin();
    
        //需要展开的节点计数，这个一直保持累计，不清零
        int nToExpand = 0;
    
        //因为是在循环中，前面的循环体中可能污染了这个变量，所以清空
        //这个变量也只是统计了某一个循环中的点
        //这个变量记录了在一次分裂循环中，那些可以再继续进行分裂的节点中包含的特征点数目和其句柄
        vSizeAndPointerToNode.clear();
    
        // 将目前的子区域进行划分
        //开始遍历列表中所有的提取器节点，并进行分解或者保留
        while(lit!=lNodes.end())
        {
            //如果提取器节点只有一个特征点，
            if(lit->bNoMore)
            {
                // If node only contains one point do not subdivide and continue
                //那么就没有必要再进行细分了
                lit++;
                //跳过当前节点，继续下一个
                continue;
            }
            else
            {
                // If more than one point, subdivide
                //如果当前的提取器节点具有超过一个的特征点，那么就要进行继续分裂
                ExtractorNode n1,n2,n3,n4;
    
                //再细分成四个子区域
                lit->DivideNode(n1,n2,n3,n4); 
    
                // Add childs if they contain points
                //如果这里分出来的子区域中有特征点，那么就将这个子区域的节点添加到提取器节点的列表中
                //注意这里的条件是，有特征点即可
                if(n1.vKeys.size()>0)
                {
                    //注意这里也是添加到列表前面的
                    lNodes.push_front(n1);   
    
                    //再判断其中子提取器节点中的特征点数目是否大于1
                    if(n1.vKeys.size()>1)
                    {
                        //如果有超过一个的特征点，那么待展开的节点计数加1
                        nToExpand++;
    
                        //保存这个特征点数目和节点指针的信息
                        vSizeAndPointerToNode.push_back(make_pair(n1.vKeys.size(),&lNodes.front()));
    
                        //?这个访问用的句柄貌似并没有用到？
                        // lNodes.front().lit 和前面的迭代的lit 不同，只是名字相同而已
                        // lNodes.front().lit是node结构体里的一个指针用来记录节点的位置
                        // 迭代的lit 是while循环里作者命名的遍历的指针名称
                        lNodes.front().lit = lNodes.begin();
                    }
                }
                //后面的操作都是相同的，这里不再赘述
                if(n2.vKeys.size()>0)
                {
                    lNodes.push_front(n2);
                    if(n2.vKeys.size()>1)
                    {
                        nToExpand++;
                        vSizeAndPointerToNode.push_back(make_pair(n2.vKeys.size(),&lNodes.front()));
                        lNodes.front().lit = lNodes.begin();
                    }
                }
                if(n3.vKeys.size()>0)
                {
                    lNodes.push_front(n3);
                    if(n3.vKeys.size()>1)
                    {
                        nToExpand++;
                        vSizeAndPointerToNode.push_back(make_pair(n3.vKeys.size(),&lNodes.front()));
                        lNodes.front().lit = lNodes.begin();
                    }
                }
                if(n4.vKeys.size()>0)
                {
                    lNodes.push_front(n4);
                    if(n4.vKeys.size()>1)
                    {
                        nToExpand++;
                        vSizeAndPointerToNode.push_back(make_pair(n4.vKeys.size(),&lNodes.front()));
                        lNodes.front().lit = lNodes.begin();
                    }
                }
    
                //当这个母节点expand之后就从列表中删除它了，能够进行分裂操作说明至少有一个子节点的区域中特征点的数量是>1的
                // 分裂方式是后加的节点先分裂，先加的后分裂
                lit=lNodes.erase(lit);
    
                //继续下一次循环，其实这里加不加这句话的作用都是一样的
                continue;
            }//判断当前遍历到的节点中是否有超过一个的特征点
        }//遍历列表中的所有提取器节点
    
        // Finish if there are more nodes than required features or all nodes contain just one point
        //停止这个过程的条件有两个，满足其中一个即可：
        //1、当前的节点数已经超过了要求的特征点数
        //2、当前所有的节点中都只包含一个特征点
        if((int)lNodes.size()>=N 				//判断是否超过了要求的特征点数
            || (int)lNodes.size()==prevSize)	//prevSize中保存的是分裂之前的节点个数，如果分裂之前和分裂之后的总节点个数一样，说明当前所有的
                                                //节点区域中只有一个特征点，已经不能够再细分了
        {
            //停止标志置位
            bFinish = true;
        }
    
        // Step 6 当再划分之后所有的Node数大于要求数目时,就慢慢划分直到使其刚刚达到或者超过要求的特征点个数
        //可以展开的子节点个数nToExpand x3，是因为一分四之后，会删除原来的主节点，所以乘以3
        /**
         * //?BUG 但是我觉得这里有BUG，虽然最终作者也给误打误撞、稀里糊涂地修复了
         * 注意到，这里的nToExpand变量在前面的执行过程中是一直处于累计状态的，如果因为特征点个数太少，跳过了下面的else-if，又进行了一次上面的遍历
         * list的操作之后，lNodes.size()增加了，但是nToExpand也增加了，尤其是在很多次操作之后，下面的表达式：
         * ((int)lNodes.size()+nToExpand*3)>N
         * 会很快就被满足，但是此时只进行一次对vSizeAndPointerToNode中点进行分裂的操作是肯定不够的；
         * 理想中，作者下面的for理论上只要执行一次就能满足，不过作者所考虑的“不理想情况”应该是分裂后出现的节点所在区域可能没有特征点，因此将for
         * 循环放在了一个while循环里面，通过再次进行for循环、再分裂一次解决这个问题。而我所考虑的“不理想情况”则是因为前面的一次对vSizeAndPointerToNode
         * 中的特征点进行for循环不够，需要将其放在另外一个循环（也就是作者所写的while循环）中不断尝试直到达到退出条件。 
         * */
        else if(((int)lNodes.size()+nToExpand*3)>N)
        {
            //如果再分裂一次那么数目就要超了，这里想办法尽可能使其刚刚达到或者超过要求的特征点个数时就退出
            //这里的nToExpand和vSizeAndPointerToNode不是一次循环对一次循环的关系，而是前者是累计计数，后者只保存某一个循环的
            //一直循环，直到结束标志位被置位
            while(!bFinish)
            {
                //获取当前的list中的节点个数
                prevSize = lNodes.size();
    
                //保留那些还可以分裂的节点的信息, 这里是深拷贝
                vector<pair<int,ExtractorNode*> > vPrevSizeAndPointerToNode = vSizeAndPointerToNode;
                //清空
                vSizeAndPointerToNode.clear();
    
                // 对需要划分的节点进行排序，对pair对的第一个元素进行排序，默认是从小到大排序
                // 优先分裂特征点多的节点，使得特征点密集的区域保留更少的特征点
                //! 注意这里的排序规则非常重要！会导致每次最后产生的特征点都不一样。建议使用 stable_sort
                sort(vPrevSizeAndPointerToNode.begin(),vPrevSizeAndPointerToNode.end());
    
                //遍历这个存储了pair对的vector，注意是从后往前遍历
                for(int j=vPrevSizeAndPointerToNode.size()-1;j>=0;j--)
                {
                    ExtractorNode n1,n2,n3,n4;
                    //对每个需要进行分裂的节点进行分裂
                    vPrevSizeAndPointerToNode[j].second->DivideNode(n1,n2,n3,n4);
    
                    // Add childs if they contain points
                    //其实这里的节点可以说是二级子节点了，执行和前面一样的操作
                    if(n1.vKeys.size()>0)
                    {
                        lNodes.push_front(n1);
                        if(n1.vKeys.size()>1)
                        {
                            //因为这里还有对于vSizeAndPointerToNode的操作，所以前面才会备份vSizeAndPointerToNode中的数据
                            //为可能的、后续的又一次for循环做准备
                            vSizeAndPointerToNode.push_back(make_pair(n1.vKeys.size(),&lNodes.front()));
                            lNodes.front().lit = lNodes.begin();
                        }
                    }
                    if(n2.vKeys.size()>0)
                    {
                        lNodes.push_front(n2);
                        if(n2.vKeys.size()>1)
                        {
                            vSizeAndPointerToNode.push_back(make_pair(n2.vKeys.size(),&lNodes.front()));
                            lNodes.front().lit = lNodes.begin();
                        }
                    }
                    if(n3.vKeys.size()>0)
                    {
                        lNodes.push_front(n3);
                        if(n3.vKeys.size()>1)
                        {
                            vSizeAndPointerToNode.push_back(make_pair(n3.vKeys.size(),&lNodes.front()));
                            lNodes.front().lit = lNodes.begin();
                        }
                    }
                    if(n4.vKeys.size()>0)
                    {
                        lNodes.push_front(n4);
                        if(n4.vKeys.size()>1)
                        {
                            vSizeAndPointerToNode.push_back(make_pair(n4.vKeys.size(),&lNodes.front()));
                            lNodes.front().lit = lNodes.begin();
                        }
                    }
    
                    //删除母节点，在这里其实应该是一级子节点
                    lNodes.erase(vPrevSizeAndPointerToNode[j].second->lit);
    
                    //判断是是否超过了需要的特征点数？是的话就退出，不是的话就继续这个分裂过程，直到刚刚达到或者超过要求的特征点个数
                    //作者的思想其实就是这样的，再分裂了一次之后判断下一次分裂是否会超过N，如果不是那么就放心大胆地全部进行分裂（因为少了一个判断因此
                    //其运算速度会稍微快一些），如果会那么就引导到这里进行最后一次分裂
                    if((int)lNodes.size()>=N)
                        break;
                }//遍历vPrevSizeAndPointerToNode并对其中指定的node进行分裂，直到刚刚达到或者超过要求的特征点个数
    
                //这里理想中应该是一个for循环就能够达成结束条件了，但是作者想的可能是，有些子节点所在的区域会没有特征点，因此很有可能一次for循环之后
                //的数目还是不能够满足要求，所以还是需要判断结束条件并且再来一次
                //判断是否达到了停止条件
                if((int)lNodes.size()>=N || (int)lNodes.size()==prevSize)
                    bFinish = true;				
            }//一直进行nToExpand累加的节点分裂过程，直到分裂后的nodes数目刚刚达到或者超过要求的特征点数目
        }//当本次分裂后达不到结束条件但是再进行一次完整的分裂之后就可以达到结束条件时
    }// 根据兴趣点分布,利用4叉树方法对图像进行划分区域
    
    // Retain the best point in each node
    // Step 7 保留每个区域响应值最大的一个兴趣点
    //使用这个vector来存储我们感兴趣的特征点的过滤结果
    vector<cv::KeyPoint> vResultKeys;
    
    //调整容器大小为要提取的特征点数目
    vResultKeys.reserve(nfeatures);
    
    //遍历这个节点链表
    for(list<ExtractorNode>::iterator lit=lNodes.begin(); lit!=lNodes.end(); lit++)
    {
        //得到这个节点区域中的特征点容器句柄
        vector<cv::KeyPoint> &vNodeKeys = lit->vKeys;
    
        //得到指向第一个特征点的指针，后面作为最大响应值对应的关键点
        cv::KeyPoint* pKP = &vNodeKeys[0];
    
        //用第1个关键点响应值初始化最大响应值
        float maxResponse = pKP->response;
    
        //开始遍历这个节点区域中的特征点容器中的特征点，注意是从1开始哟，0已经用过了
        for(size_t k=1;k<vNodeKeys.size();k++)
        {
            //更新最大响应值
            if(vNodeKeys[k].response>maxResponse)
            {
                //更新pKP指向具有最大响应值的keypoints
                pKP = &vNodeKeys[k];
                maxResponse = vNodeKeys[k].response;
            }
        }
    
        //将这个节点区域中的响应值最大的特征点加入最终结果容器
        vResultKeys.push_back(*pKP);
    }
    
    //返回最终结果容器，其中保存有分裂出来的区域中，我们最感兴趣、响应值最大的特征点
    return vResultKeys;
}
```

## 四、结语

通过这篇博客的讲解我们已经知道了关键点如何进行均匀化，但是前面提到过，FAST关键点关键点是没有方向信息的，所以后续我们还需要使用 灰度质心法 为关键点添加上方向或者说角度信息。

# 8. ORBextractor::operator()→灰度质心法-orientated FAST

## 一、前言

原始的FAST关键点没有方向信息，这样当图像发生旋转后，brief描述子也会发生变化，使得特征点对旋转不鲁棒。

解决方法：orientated FAST，使用灰度质心法计算特征点的方向。

什么是灰度质心法？

## 二、理论介绍

**第一步:**
 我们定义该区域图像的矩

![image-20220922154127751](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220922154127751.png)

式中 p , q 取0或者1； I(x,y)表示在像素坐标  (x,y) 处图像的灰度值l; mpq表示图像的矩。在半径为 R 的圆形图像区域，沿两个坐标轴 (x,y)方向的图像矩分别为:
![image-20220922154338946](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220922154338946.png)

圆形区域内所有像素的灰度值总和为:

![image-20220922154408538](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220922154408538.png)

**第二步:**
 图像的质心为:

![image-20220922154438840](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220922154438840.png)

第三步:
关键点的"主方向"就可以表示为从圆形图像形心 O 指向质心 C的方向向量 O C → ，于是关键点的旋转角度记为：

![image-20220922154522615](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220922154522615.png)

以上就是灰度质心法求关键点旋转角度的问题。下图 P 为几何中心， Q 为灰度质心：

![在这里插入图片描述](https://img-blog.csdnimg.cn/2646f5f35f4f489f9e5530eb92eed8be.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_20,color_FFFFFF,t_70,g_se,x_16#pic_center)

思考:
为什么计算灰度质心的时候，是选择一个圆中的像素，而不是一个正方形。这是因为圆具备旋转不变的性质。比如说，一张图像中确定一个像素的质心之后，图像发生了旋转。这个时候，如果我们选择以该像素为中心，计算正方形区域内的像素的矩 m 01, m 01, m 00就会发生变化，这样质心就发生改变。但是如果计算圆内的像素。只要半径保持不变，那么他的像素就不会发生变化。如下图所示：
![在这里插入图片描述](https://img-blog.csdnimg.cn/4a765582200e414b88a99b5b58e79566.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_12,color_FFFFFF,t_70,g_se,x_16#pic_center)

可以看到正方形旋转的时候，蓝色与黄色区域的像素是不一样的。

## 三、代码流程

代码中计算灰度质心的时候，还采用了一些其他的技巧，下面是一个简单的图示:
 ![在这里插入图片描述](https://img-blog.csdnimg.cn/80b0e44f2f27486daf78d1d2ffdd9ea4.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_18,color_FFFFFF,t_70,g_se,x_16#pic_center)

比如说源码求 m 00 的时候，其需要计算圆内所有像素值的总和，其是先求红色行像素值总和(一列)，然后在求黄色列像素值总和(两列)，再接着求绿色列总和(两列)。依次递推下去。然后把所有列的和相加起来，就是圆内像素值的总和。

使用这种方式进行计算，在代码实现的时候，就需要一些已知量，比如黄色列的坐标索引，或者说知道黄色列这一列一共有多少个像素。代码中是如何实现计算的呢，我们先来看下图：

![在这里插入图片描述](https://img-blog.csdnimg.cn/50ec60e54cba4e159a194ad0be86c106.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_19,color_FFFFFF,t_70,g_se,x_16)

上图的 V , U  分别表示坐标轴，以及圆的半径，然后使用勾股定理进行计算。但是代码中用到了一个技巧，就是只用对称的方式进行计算，以 AB 为对称轴进行计算。该坐标的计算代码位于 src/ORBextractor.cc 的 ORBextractor::ORBextractor() 函数中，该函数我们在前面的博客中有进行讲解过,我们下面只把圆内灰度值计算的相关代码贴出来讲解，下面求园内的坐标范围

```
ORBextractor::ORBextractor(int _nfeatures,		//指定要提取的特征点数目
                           float _scaleFactor,	//指定图像金字塔的缩放系数
                           int _nlevels,		//指定图像金字塔的层数
                           int _iniThFAST,		//指定初始的FAST特征点提取参数，可以提取出最明显的角点
                           int _minThFAST):		//如果初始阈值没有检测到角点，降低到这个阈值提取出弱一点的角点
    nfeatures(_nfeatures), scaleFactor(_scaleFactor), nlevels(_nlevels),
    iniThFAST(_iniThFAST), minThFAST(_minThFAST)//设置这些参数
{
	......
	......
	......
   //This is for orientation
    //下面的内容是和特征点的旋转计算有关的
    // pre-compute the end of a row in a circular patch
    //预先计算圆形patch中行的结束位置
    //+1中的1表示那个圆的中间行
    umax.resize(HALF_PATCH_SIZE + 1);
    

    //cvFloor返回不大于参数的最大整数值，cvCeil返回不小于参数的最小整数值，cvRound则是四舍五入
    int v,		//循环辅助变量
        v0,		//辅助变量
        vmax = cvFloor(HALF_PATCH_SIZE * sqrt(2.f) / 2 + 1);	//计算圆的最大行号，+1应该是把中间行也给考虑进去了
                //NOTICE 注意这里的最大行号指的是计算的时候的最大行号，此行的和圆的角点在45°圆心角的一边上，之所以这样选择
                //是因为圆周上的对称特性
                
    //这里的二分之根2就是对应那个45°圆心角
    
    int vmin = cvCeil(HALF_PATCH_SIZE * sqrt(2.f) / 2);
    //半径的平方
    const double hp2 = HALF_PATCH_SIZE*HALF_PATCH_SIZE;
    
    //利用圆的方程计算每行像素的u坐标边界（max）
    for (v = 0; v <= vmax; ++v)
        umax[v] = cvRound(sqrt(hp2 - v * v));		//结果都是大于0的结果，表示x坐标在这一行的边界
    
    // Make sure we are symmetric
    //这里其实是使用了对称的方式计算上四分之一的圆周上的umax，目的也是为了保持严格的对称（如果按照常规的想法做，由于cvRound就会很容易出现不对称的情况，
    //同时这些随机采样的特征点集也不能够满足旋转之后的采样不变性了）
    for (v = HALF_PATCH_SIZE, v0 = 0; v >= vmin; --v)
    {
        while (umax[v0] == umax[v0 + 1])
            ++v0;
        umax[v] = v0;
        DEBUG("%d=%d", v, v0);
        ++v0;
    }
```

umax: 1/4圆的每一行的u轴坐标边界（例如图中橙色线段FG）

对应从D到B的红色弧线，umax坐标从D到C

    for (v = 0; v <= vmax; ++v)
        umax[v] = cvRound(sqrt(hp2 - v * v));		//结果都是大于0的结果，表示x坐标在这一行的边界
对应从B到E的蓝色弧线，umax坐标从C到A

    for (v = HALF_PATCH_SIZE, v0 = 0; v >= vmin; --v)
    {
        while (umax[v0] == umax[v0 + 1])
            ++v0;
        umax[v] = v0;
        DEBUG("%d=%d", v, v0);
        ++v0;
    }
比如这里的半径 HALF_PATCH_SIZE = 15, 其计算出来的 umax 数值为:

	umax[0] == 15
	umax[1] == 15
	umax[2] == 15
	umax[3] == 15
	umax[4] == 14
	umax[5] == 14
	umax[6] == 14
	umax[7] == 13
	umax[8] == 13
	umax[9] == 12
	umax[10] == 11
	umax[11] == 10
	umax[12] == 9
	umax[13] == 8
	umax[14] == 6
	umax[15] == 3
其上 u m a x [ 0 ] = = 15意思表示，当 V = 0 ，在该行参数计算的像素个数为15个( U > 0 方向上)。 u m a x [ 1 ] = = 15意思表示，当 V = 1，在该行参与计算的像素个数为15个( U > 0 方向上 ), 依次递推。四、源码注释

## 四、源码注释

灰度质心角度计算于 src/ORBextractor.cpp 中被 ORBextractor::ComputeKeyPointsOctTree() 调用，其函数名为 computeOrientation(), 实现具体过程图下:
```
 * /**
  * @brief 计算特征点的方向
 * @param[in] image                 特征点所在当前金字塔的图像
 * @param[in & out] keypoints       特征点向量
     * @param[in] umax                  每个特征点所在图像区块的每行的边界 u_max 组成的vector
     */
static void computeOrientation(const Mat& image, vector<KeyPoint>& keypoints, const vector<int>& umax)
    {
        // 遍历所有的特征点
    for (vector<KeyPoint>::iterator keypoint = keypoints.begin(),
             keypointEnd = keypoints.end(); keypoint != keypointEnd; ++keypoint)
        {
            // 调用IC_Angle 函数计算这个特征点的方向
            keypoint->angle = IC_Angle(image, 			//特征点所在的图层的图像
                                       keypoint->pt, 	//特征点在这张图像中的坐标
                                       umax);			//每个特征点所在图像区块的每行的边界 u_max 组成的vector
    }
    }
    
    /**
     * @brief 这个函数用于计算特征点的方向，这里是返回角度作为方向。
    
     * 计算特征点方向是为了使得提取的特征点具有旋转不变性。
    
     * 方法是灰度质心法：以几何中心和灰度质心的连线作为该特征点方向
    
     * @param[in] image     要进行操作的某层金字塔图像
    
     * @param[in] pt        当前特征点的坐标
    
     * @param[in] u_max     图像块的每一行的坐标边界 u_max
    
     * @return float        返回特征点的角度，范围为[0,360)角度，精度为0.3°
     */
    static float IC_Angle(const Mat& image, Point2f pt,  const vector<int> & u_max)
    {
        //图像的矩，前者是按照图像块的y坐标加权，后者是按照图像块的x坐标加权
        int m_01 = 0, m_10 = 0;
    
        //获得这个特征点所在的图像块的中心点坐标灰度值的指针center
        const uchar* center = &image.at<uchar> (cvRound(pt.y), cvRound(pt.x));
    
        // Treat the center line differently, v=0
        //这条v=0中心线的计算需要特殊对待
        //后面是以中心行为对称轴，成对遍历行数，所以PATCH_SIZE必须是奇数
        for (int u = -HALF_PATCH_SIZE; u <= HALF_PATCH_SIZE; ++u)
        //注意这里的center下标u可以是负的！中心水平线上的像素按x坐标（也就是u坐标）加权
            m_10 += u * center[u];
    
    // Go line by line in the circular patch  
     //这里的step1表示这个图像一行包含的字节总数。参考[https://blog.csdn.net/qianqing13579/article/details/45318279]
     int step = (int)image.step1();
     //注意这里是以v=0中心线为对称轴，然后对称地每成对的两行之间进行遍历，这样处理加快了计算速度
     for (int v = 1; v <= HALF_PATCH_SIZE; ++v)
     {
         // Proceed over the two lines
         //本来m_01应该是一列一列地计算的，但是由于对称以及坐标x,y正负的原因，可以一次计算两行
         int v_sum = 0;
         // 获取某行像素横坐标的最大范围，注意这里的图像块是圆形的！
         int d = u_max[v];
         //在坐标范围内挨个像素遍历，实际是一次遍历2个
         // 假设每次处理的两个点坐标，中心线下方为(x,y),中心线上方为(x,-y) 
         // 对于某次待处理的两个点：m_10 = Σ x*I(x,y) =  x*I(x,y) + x*I(x,-y) = x*(I(x,y) + I(x,-y))
         // 对于某次待处理的两个点：m_01 = Σ y*I(x,y) =  y*I(x,y) - y*I(x,-y) = y*(I(x,y) - I(x,-y))
         for (int u = -d; u <= d; ++u)
         {
             //得到需要进行加运算和减运算的像素灰度值
             //val_plus：在中心线下方x=u时的的像素灰度值
             //val_minus：在中心线上方x=u时的像素灰度值
             int val_plus = center[u + v*step], val_minus = center[u - v*step];
             //在v（y轴）上，2行所有像素灰度值之差
             v_sum += (val_plus - val_minus);
             //u轴（也就是x轴）方向上用u坐标加权和（u坐标也有正负符号），相当于同时计算两行
             m_10 += u * (val_plus + val_minus);
         }
         //将这一行上的和按照y坐标加权
         m_01 += v * v_sum;
     }
 
     //为了加快速度还使用了fastAtan2()函数，输出为[0,360)角度，精度为0.3°
     return fastAtan2((float)m_01, (float)m_10);
 }
```

fastAtan2 函数求解出来的结果是一个角度，该结果会复制给关键点对应的 keypoint->angle 参数。

## 五、结语

该篇博客我们了解了如何使用灰度质心法求关键点的角度方向，针对于FAST关键点的相关内容已经讲解得差不多了。前面我们提到ORB特征点由 FAST 关键点以及 BRIEF 描述子组成，下面就是对 BRIEF 的相关内容进行讲解了。

# ORBextractor::operator()→BRIEF描述子

## 一、前言

通过前面的博客，我们已经对FAST关键点做了详细的讲解。我们先来回顾一下之前的代码，在 ORBextractor::[operator](https://so.csdn.net/so/search?q=operator&spm=1001.2101.3001.7020)() 函数中，调用了比较重要的几个函数：

```
void ORBextractor::operator()( InputArray _image, InputArray _mask, vector<KeyPoint>& _keypoints,
                      OutputArray _descriptors){
    // Step 2 构建图像金字塔
    ComputePyramid(image);
    //使用四叉树的方式计算每层图像的特征点并进行分配
    ComputeKeyPointsOctTree(allKeypoints);
    // Step 6 计算高斯模糊后图像的描述子
    computeDescriptors(workingMat, 	//高斯模糊之后的图层图像
                       keypoints, 	//当前图层中的特征点集合
                       desc, 		//存储计算之后的描述子
                       pattern);	//随机采样模板
}
```

针对于 ComputePyramid(image); 以及 ComputeKeyPointsOctTree(allKeypoints); 已经做了比较详细的讲解，接下来我们就是就是对 computeDescriptors()进行讲解了。在代码讲解之前，我们先来了解一下BRIEF描述子的理论知识。

## 二、理论基础

论文：BRIEF: Binary Robust Independent Elementary Features，如果需要详细了解的朋友可以查看这篇论文。
简要说明: 一种对已检测到的特征点进行描述的算法，它是一种二进制编码的描述子，在图像匹配时使用BRIEF能极大的提升匹配速度。
主要实现的思想逻辑如下：

- 步骤(1)
          为减少噪声干扰，先对图像进行高斯滤波.

- 步骤 ( 2 ) 
          以关键点为中心，取SxS的邻域大窗口(仅在该区域内为该关键点生成描述子)。大窗口中随机选取一对（两个）5x5的子窗口，当然也可以是随机两个像素点。比较子窗口内的像素和（可用积分图像完成），进行二进制赋值。（一般S=31），公式如下：

![image-20220922212141518](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220922212141518.png)

其上的公式很好理解，就是说两个窗口(像素)和进行比较，第一个小于第二个则为 0否则为1。其中，p(x)， p(y) 分别取随机点 x = ( u 1 , v 1 ) x=(u2,v2)所在5x5子窗口的像素和。

- 步骤(3)
   在大窗口中随机选取N对子窗口，重复步骤2的二进制赋值，形成一个二进制编码，这个编码就是对特征点的描述，即特征描述子。（一般N=256），256对其结果就存在256位二进制，一字节8位，也就是四个字节，相当于一个32位 int 形。

其他
 以上便是BRIEF特征描述算法的步骤。关于随机对点的选择方法，原作者测试了以下五种方式，其中方式(2)的效果比较好。

![image-20220922213045652](C:\Users\YinHeSheng\AppData\Roaming\Typora\typora-user-images\image-20220922213045652.png)

这5种方法生成的256对随机点如下（一条线段的两个端点是一对）：

![在这里插入图片描述](https://img-blog.csdnimg.cn/54b7f99a8a254d82ab6218c85664c572.png?x-oss-process=image/watermark,type_d3F5LXplbmhlaQ,shadow_50,text_Q1NETiBA5rGf5Y2X5omN5bC977yM5bm05bCR5peg55-l77yB,size_10,color_FFFFFF,t_70,g_se,x_16)

经过上面的特征提取算法，对于一幅图中的每一个特征点，都得到了一个256bit的二进制编码。

## 三、代码流程

步骤 ( 1 ) 
为了每次计算的描述子的踩点是一致的，先固定采样模板，也就是使用固定的采样像素对。代码在src/ORBextractor.cc文件中的 bit_pattern_31_ 变量, 其是 256*4 的数组。一对坐标4个元素，所以这种共256对。

步骤 ( 2 ) 
获得关键点角度，单纯的 BRIEF 描述子是不具备方向信息的，所以需要与关键点的角度结合起来。

步骤 ( 3 ) 
分成32次循环，每次循环对比8对像素值，共完成 32x8=256 次对比，代码中还使用位移操作来完成计算，这样加快了代码运行速率。

## 四、pattern的构建

要提取BRIEF描述子，这里需要先明白的一个变量就是pattern，它里面具体保存的内容，以及他的作用，个人觉得与BRIEF相关的其实就是这里（貌似也没有其他地方了[捂脸]）

理解pattern之前需要先看一个变量，也就是bit_pattern_31_，也就是那个256*4的变量，这里只摘抄两行出来：

```
static int bit_pattern_31_[256*4] =
{
    8,-3, 9,5/*mean (0), correlation (0)*/,
    4,2, 7,-12/*mean (1.12461e-05), correlation (0.0437584)*/,
   ...
}
```

这个变量里的数字，在ORBSLAM的代码中总共是256行，代表了256个点对儿，也就是每一个都代表了一对点的坐标，如第一行表示点q1(8,-3) 和点 q2(9,5), 接下来就是要对比这两个坐标对应的像素值的大小；

好了，明白了bit_pattern_31_里面保存的点对就可以，接下来在ORBextractor的构造函数中，将这个数组转换成了std::vector<cv::Point> pattern; 也就是一个包含512个Point的变量；

```
const int npoints = 512;
    const Point* pattern0 = (const Point*)bit_pattern_31_;
    // copy [pattern0,pattern0+npoints] 到std::vector<cv::Point> pattern 
    std::copy(pattern0, pattern0 + npoints, std::back_inserter(pattern));
```

至此，BRIEF描述子的模板已经保存成功，将要在下面的描述子成型中使用；

## 五、描述子成型

center为中心，因为点对的数量为256，也就是512个点，这里将其分成32组，每一组包含16个点，也就是8个点对；

```
 for (int i = 0; i < 32; ++i, pattern += 16)
{
        int t0, t1, val;
        t0 = GET_VALUE(0); t1 = GET_VALUE(1);  //GET_VALUE用于获取该id对应的坐标出的像素值
        val = t0 < t1;
        t0 = GET_VALUE(2); t1 = GET_VALUE(3);
        val |= (t0 < t1) << 1;
 }
```

## 六、代码注释

该核心代码为 src/ORBextractor.cc 中的 computeDescriptors()函数，注释如下：
```
//注意这是一个不属于任何类的全局静态函数，static修饰符限定其只能够被本文件中的函数调用
/**
 * @brief 计算某层金字塔图像上特征点的描述子
 * 
 * @param[in] image                 某层金字塔图像
 * @param[in] keypoints             特征点vector容器
 * @param[out] descriptors          描述子
 * @param[in] pattern               计算描述子使用的固定随机点集
 */
static void computeDescriptors(const Mat& image, vector<KeyPoint>& keypoints, Mat& descriptors,
                               const vector<Point>& pattern)
{
    //清空保存描述子信息的容器
    descriptors = Mat::zeros((int)keypoints.size(), 32, CV_8UC1);

    //开始遍历特征点
    for (size_t i = 0; i < keypoints.size(); i++)
        //计算这个特征点的描述子
        computeOrbDescriptor(keypoints[i], 				//要计算描述子的特征点
                             image, 					//以及其图像
                             &pattern[0], 				//随机点集的首地址
                             descriptors.ptr((int)i));	//提取出来的描述子的保存位置
}

/**
 * @brief 计算ORB特征点的描述子。注意这个是全局的静态函数，只能是在本文件内被调用
 * @param[in] kpt       特征点对象
 * @param[in] img       提取特征点的图像
 * @param[in] pattern   预定义好的采样模板
 * @param[out] desc     用作输出变量，保存计算好的描述子，维度为32*8 = 256 bit
 */
static void computeOrbDescriptor(const KeyPoint& kpt, const Mat& img, const Point* pattern, uchar* desc)
{
    //得到特征点的角度，用弧度制表示。其中kpt.angle是角度制，范围为[0,360)度
    float angle = (float)kpt.angle*factorPI;
    //计算这个角度的余弦值和正弦值
    float a = (float)cos(angle), b = (float)sin(angle);

    //获得图像中心指针
    const uchar* center = &img.at<uchar>(cvRound(kpt.pt.y), cvRound(kpt.pt.x));
    //获得图像的每行的字节数
    const int step = (int)img.step;

    //原始的BRIEF描述子没有方向不变性，通过加入关键点的方向来计算描述子，称之为Steer BRIEF，具有较好旋转不变特性
    //具体地，在计算的时候需要将这里选取的采样模板中点的x轴方向旋转到特征点的方向。
    //获得采样点中某个idx所对应的点的灰度值,这里旋转前坐标为(x,y), 旋转后坐标(x',y')，他们的变换关系:
    // x'= xcos(θ) - ysin(θ),  y'= xsin(θ) + ycos(θ)
    // 下面表示 y'* step + x'
    #define GET_VALUE(idx) center[cvRound(pattern[idx].x*b + pattern[idx].y*a)*step + cvRound(pattern[idx].x*a - pattern[idx].y*b)]        
    
    //brief描述子由32*8位组成
    //其中每一位是来自于两个像素点灰度的直接比较，所以每比较出8bit结果，需要16个随机点，这也就是为什么pattern需要+=16的原因
    for (int i = 0; i < 32; ++i, pattern += 16)
    {
        
    
        int t0, 	//参与比较的第1个特征点的灰度值
            t1,		//参与比较的第2个特征点的灰度值		
            val;	//描述子这个字节的比较结果，0或1
        
        t0 = GET_VALUE(0); t1 = GET_VALUE(1);
        val = t0 < t1;							//描述子本字节的bit0
        t0 = GET_VALUE(2); t1 = GET_VALUE(3);
        val |= (t0 < t1) << 1;					//描述子本字节的bit1
        t0 = GET_VALUE(4); t1 = GET_VALUE(5);
        val |= (t0 < t1) << 2;					//描述子本字节的bit2
        t0 = GET_VALUE(6); t1 = GET_VALUE(7);
        val |= (t0 < t1) << 3;					//描述子本字节的bit3
        t0 = GET_VALUE(8); t1 = GET_VALUE(9);
        val |= (t0 < t1) << 4;					//描述子本字节的bit4
        t0 = GET_VALUE(10); t1 = GET_VALUE(11);
        val |= (t0 < t1) << 5;					//描述子本字节的bit5
        t0 = GET_VALUE(12); t1 = GET_VALUE(13);
        val |= (t0 < t1) << 6;					//描述子本字节的bit6
        t0 = GET_VALUE(14); t1 = GET_VALUE(15);
    val |= (t0 < t1) << 7;					//描述子本字节的bit7
        
        //保存当前比较的出来的描述子的这个字节
        desc[i] = (uchar)val;
}
    
    //为了避免和程序中的其他部分冲突在，在使用完成之后就取消这个宏定义
#undef GET_VALUE
 }
```

----------------------------插播介绍uchar--------------------------------------------------

unsigned char，uchar ，UCHAR，这几个都是表示的无符号的char，其实都是unsigned char的宏定义，所以就是一个东西。
char占一个字节，能表示 -128 到127
1，计算机里面所有的数都是用补码表示的，正数的补码是其本身，负数的补码是原码的反码（符号为以外，其余的全部求反）加一。
2，数字在计算机中是以二进制来存储的，最高位是符号位，0为正1为负
3，表示正数时：0~127 0000 0000 ~ 0111 1111
表示负数时： 1111 1111 ~ 1000 0000 表示的是 -127 ~ -0
此时的补码是 1000 0001 ~ 0000 0000 ，截断后变为0000 0000，这与+0所表示的数是一样的，为了不能浪费1000 0000这个数值，我们规定用1000 0000这个位来表示-128。
所以char表示的范围是-128 ~ 127.
----------------------------插播介绍uchar--------------------------------------------------

---------------------------插播指针变量为什么可以作为“数组名”？----------------

```
//输入输出数组
#include <bits/stdc++.h>
using namespace std;
void f(int *p1)
{
    int i;
    for(i=0;i<5;i++)
    {
        if(i!=4)
            cout<<*p1<<" ";
        else 
            cout<<*p1<<endl;
        p1++;
    }
    /*
    也可以当数组使用
    for(i=0;i<5;i++)
    {
        if(i!=4)
            cout<<p1[i]<<" ";//也可以用 *(p1+i);
        else 
            cout<<p1[i]<<endl;
    }
    */
}
int main()
{
    int a[10],i;
    for(i=0;i<5;i++)
        cin>>a[i];
    f(a);//也可以用 f(&a[0]);
    return 0;
}
输入：
1 2 3 4 5
输出：
1 2 3 4 5
————————————————
```

数组a[i]在编译的时候会被编译器复原成*(a+i)，其中a是数组名，也是数组的首地址，也就是说a[i]和*(a+i)是完全等价的，甚至可以认为C语言没有数组，a[i]只是方便表达，实际上程序是按照*(a+i)操作的。因此，数组在一段连续的内存中存储的，这也是可以用数组来描述数据结构中的顺序存储结构的原因。

综上，我们对数组这种变量类型有了更本质的理解。

---------------------------插播指针变量为什么可以作为“数组名”？完毕---------------

## 五、结语

到该章节为止，对于ORB特征提取的相关内容可以说是结束了。也就是 ORBextractor::operator() 函数已经完成了。该函数是在 Frame.cc 中的 Frame::Frame 中被调用的。如下：

```
Frame::Frame(const cv::Mat &imGray, const double &timeStamp, ORBextractor* extractor,ORBVocabulary* voc, cv::Mat &K, cv::Mat &distCoef, const float &bf, const float &thDepth)
    :mpORBvocabulary(voc),mpORBextractorLeft(extractor),mpORBextractorRight(static_cast<ORBextractor*>(NULL)),
     mTimeStamp(timeStamp), mK(K.clone()), mDistCoef(distCoef.clone()), mbf(bf), mThDepth(thDepth)
{
    // ORB extraction
	// Step 3 对图像进行提取特征点, 第一个参数0-左图， 1-右图
    ExtractORB(0,imGray);
	......
	......
}
```

Frame 的构造函数除了提取 ORB 特征之外,还做了其他的操作，如特征点畸变矫正等等。这些内容我们会在后面的博客进行讲解。