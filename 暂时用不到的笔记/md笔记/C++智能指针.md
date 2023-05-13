# 一、动态内存管理
在C++中，动态内存的管理是用一对运算符完成的：new和delete，
new:在动态内存中为对象分配一块空间并返回一个指向该对象的指针，
delete：指向一个动态独享的指针，销毁对象，并释放与之关联的内存。
动态内存管理经常会出现两种问题：一种是忘记释放内存，会造成内存泄漏；一种是尚有指针引用内存的情况下就释放了它，就会产生引用非法内存的指针。

为了更加容易（更加安全）的使用动态内存，引入了智能指针的概念。

在C++中没有垃圾回收机制，必须自己释放分配的内存，否则就会造成内存泄露。解决这个问题最有效的方法是使用智能指针（smart pointer）。
智能指针是存储指向动态分配（堆）对象指针的类，用于生存期的控制，能够确保在离开指针所在作用域时，自动地销毁动态分配的对象，防止内存泄露。
智能指针的核心实现技术是引用计数，每使用它一次，内部引用计数加1，每析构一次内部的引用计数减1，减为0时，删除所指向的堆内存。

从比较简单的层面来看，智能指针是RAII(Resource Acquisition Is Initialization，资源获取即初始化)机制对普通指针进行的一层封装。
这样使得智能指针的行为动作像一个指针，本质上却是一个对象，这样可以方便管理一个对象的生命周期。

原始指针的问题大家都懂，就是如果忘记删除，或者删除的情况没有考虑清楚，容易造成悬挂指针(dangling pointer)或者说野指针(wild pointer)。

例子:
```c++
objtype *p = new objtype();
p -> func();
delete p;

```
可能出现的问题：

1.代码的最后，忘记执行delete p的操作。
2.第一点其实还好，比较容易发现也比较容易解决。比较麻烦的是，如果func()中有异常，delete p语句执行不到，这就很难办。有的同学说可以在func中进行删除操作，理论上是可以这么做，实际操作起来，会非常麻烦也非常复杂。
此时，智能指针就可以方便我们控制指针对象的生命周期。在智能指针中，一个对象什么情况下被析构或被删除，是由指针本身决定的，并不需要用户进行手动管理。


# 二、野指针

## 1、未初始化
造成野指针最常见的情况之一就是指针未被正确初始化。任何指针在被创建的时候，不会自动变成NULL指针，他的default值是随机的。所以一个比较好的习惯是，指针刚创建的时候，要么设置为NULL空指针，要么指向合理的内存区域。

```c++
void func() {
    int* p;
    cout<<*p<<endl;
}
```
我们在main方法中调用该函数，会得到输出

    1901647869

不难看出得到的就是一个随机值。

## 2、悬垂指针

悬垂指针也是野指针常见的一种。当我们显式地从内存中删除一个对象或者返回时通过销毁栈帧，并不会改变相关的指针的值。这个指针实际仍然指向内存中相同位置，甚至该位置仍然可以被读写，只不过这时候该内存区域完全不可控，因为你不能保证这块内存是不是被别的程序或者代码使用过。
```c++
void dangling_point() {
    char *p = (char *)malloc(10);
    strcpy(p, "abc");
    cout<<p<<endl;
    free(p);
    if (p != NULL) cout<<p<<endl;
    strcpy(p, "def"); 
}
```
我们执行free()语句时，p所指向的内存被释放，此时该内存区域变成不可用内存。但是，p此时指向的地址并没有发生改变，而且也不会为空，所以if(p != NULL)判断为真，会继续执行后面的语句。
但是strcpy(p, “def”); 这一句，虽然代码不会报错崩溃，但此时正在篡改动态内存区，会产生不可预料的结果，此操作相当危险，尤其是debug时候非常不好排查，往往会让人崩溃…
为了避免悬垂指针的问题，一般做法是在free/delete指针以后，再将p=NULL，这样就可以避免上述问题。


## 3、返回栈内存指针或引用
```c++
    int* return_ret() {
        int result = 1;
        return &result;
    }

    int main(int argc, char const *argv[])
    {
        int *p = return_ret();
        cout<<*p<<endl;
        cout<<*p<<endl;
        return 0;
    }
```
首先，这段代码能编译通过。不过IDE会提醒warning:
test_code.cc:80:13: warning: address of stack memory associated with local variable 'result' returned [-Wreturn-stack-address]
    return &result;
            ^~~~~~
1 warning generated.
什么意思？就是我们返回了一个局部变量的栈内内存地址。
代码也能正常运行：

输出：

    1
    32767

栈中的数据在函数执行之后就被释放了
我们注意看，第一个打印语句，甚至还能输出"正确"的结果1，这里是编译器为我们做了一次保留。
但是到了第二个打印语句，此时输出的结果就不可控了，因为此时该内存区域的内容，已经发生了变化。此时该内存地址已经变得不"可靠"，操作该内存区域将会相当危险。

## 3、free()的问题
malloc动态申请一块内存，使用完毕后free掉这个指针，只是将指针指向的内存空间释放掉了，并没有将指针置为NULL，指针仍指向被释放掉的内存的地址，在判断指针是否为NULL的时候，通常是通过if(pt == NULL) ,这时，导致指针成为了野指针

并且野指针和空指针不同，野指针有地址，或者说是指向内存，对野指针进行操作，会造成内存错误，并且野指针无法从if语句进行判断其是否为NULL，所以在指针释放之后要将指针置为NULL。


# 二、智能指针
智能指针的行为类似常规指针，重要的区别是它负责自动释放所指向的对象。
标准库提供的两种智能指针的区别在于管理底层指针的方法不同，
shared_ptr允许多个指针指向同一个对象，
unique_ptr则“独占”所指向的对象。
标准库还定义了一种名为weak_ptr的伴随类，它是一种弱引用，指向shared_ptr所管理的对象，
这三种智能指针都定义在memory头文件中。

# 三、shared_ptr类

我们提到的智能指针，很大程度上就是指的shared_ptr，shared_ptr也在实际应用中广泛使用。它的原理是使用引用计数实现对同一块内存的多个引用。在最后一个引用被释放时，指向的内存才释放，这也是和 unique_ptr 最大的区别。当对象的所有权需要共享(share)时，share_ptr可以进行赋值拷贝。
shared_ptr使用引用计数，每一个shared_ptr的拷贝都指向相同的内存。每使用他一次，内部的引用计数加1，每析构一次，内部的引用计数减1，减为0时，删除所指向的堆内存。

shared_ptr是可以共享所有权的指针。如果有多个shared_ptr共同管理同一个对象时，只有这些 shared_ptr全部与该对象分离之后，被管理的对象才会被释放，使用需要包含头文件：
```c++
#include <boost/shared_ptr.hpp>
```

创建智能指针时必须提供额外的信息，即指针指向的类型:

```c++
    shared_ptr<string> p1;
    shared_ptr<list<int>>p2;
```
默认初始化的智能指针中保存着一个空指针。

### 赋值
错误写法：
```c++
std::shared_ptr<int> p4 = new int(1)
```
上面这种写法是错误的，因为右边得到的是一个原始指针，前面我们讲过shared_ptr本质是一个对象，将一个指针赋值给一个对象是不行的。

正确写法：
```c++
void f2() {
    shared_ptr<int> p = make_shared<int>(1);
    shared_ptr<int> p2(p);
    shared_ptr<int> p3 = p;
}
```
以上写法都是可以的


### 获取原始指针

```c++
void f2() {
    shared_ptr<int> p = make_shared<int>(1);
    int *p2 = p.get();
    cout<<*p2<<endl;
}
```

智能指针的使用方式和普通指针类似,引用一个智能指针放回他的对象, 在一个条件判断中使用智能指针就是检测他是不是空.
```C++
if(p1 && p1->empty())
    *p1 = "hi";
```
### shared_ptr使用需要注意的点

不能将一个原始指针初始化多个shared_ptr
```c++
void f2() {
    int *p0 = new int(1);
    shared_ptr<int> p1(p0);
    shared_ptr<int> p2(p0);
    cout<<*p1<<endl;
}
```
上面代码就会报错。原因也很简单，因为p1,p2都要进行析构删除，这样会造成原始指针p0被删除两次，自然要报错。

循环引用问题
shared_ptr最大的坑就是循环引用。引用网络上的一个例子：

```c++
    struct Father
    {
        shared_ptr<Son> son_;
    };

    struct Son
    {
        shared_ptr<Father> father_;
    };

    int main()
    {
        auto father = make_shared<Father>();
        auto son = make_shared<Son>();

        father->son_ = son;
        son->father_ = father;

        return 0;
    }
```
该部分代码会有内存泄漏问题。原因是
1.main 函数退出之前，Father 和 Son 对象的引用计数都是 2。
2.son 指针销毁，这时 Son 对象的引用计数是 1。
3.father 指针销毁，这时 Father 对象的引用计数是 1。
4.由于 Father 对象和 Son 对象的引用计数都是 1，这两个对象都不会被销毁，从而发生内存泄露。

为避免循环引用导致的内存泄露，就需要使用 weak_ptr。weak_ptr 并不拥有其指向的对象，也就是说，让 weak_ptr 指向 shared_ptr 所指向对象，对象的引用计数并不会增加。
使用 weak_ptr 就能解决前面提到的循环引用的问题，方法很简单，只要让 Son 或者 Father 包含的 shared_ptr 改成 weak_ptr 就可以了。
```c++
struct Father
{
    shared_ptr<Son> son_;
};

struct Son
{
    weak_ptr<Father> father_;
};

int main()
{
    auto father = make_shared<Father>();
    auto son = make_shared<Son>();

    father->son_ = son;
    son->father_ = father;

    return 0;
}
```
1.main 函数退出前，Son 对象的引用计数是 2，而 Father 的引用计数是 1。
2.son 指针销毁，Son 对象的引用计数变成 1。
3.father 指针销毁，Father 对象的引用计数变成 0，导致 Father 对象析构，Father 对象的析构会导致它包含的 son_ 指针被销毁，这时 Son 对象的引用计数变成 0，所以 Son 对象也会被析构

## 成员函数
use_count ：返回引用计数的个数
unique ：返回是否是独占所有权( use_count 为 1)
swap ：交换两个 shared_ptr 对象(即交换所拥有的对象)
reset ：放弃内部对象的所有权或拥有对象的变更, 会引起原有对象的引用计数的减少
get ：返回内部对象(指针)
