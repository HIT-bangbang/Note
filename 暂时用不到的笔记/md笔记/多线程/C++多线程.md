# 多线程的概念

传统的C++（C++11之前）中并没有引入线程这个概念，在C++11出来之前，如果我们想要在C++中实现多线程，需要借助操作系统平台提供的API，比如Linux的<pthread.h>，或者windows下的<windows.h> 。

C++11提供了语言层面上的多线程，包含在头文件<thread>中。它解决了跨平台的问题，提供了管理线程、保护共享数据、线程间同步操作、原子操作等类。C++11 新标准中引入了5个头文件来支持多线程编程，如下图所示：
![img](1.png)

## 多线程与多进程

### 1、进程并发
使用多进程并发是将一个应用程序划分为多个独立的进程（每个进程只有一个线程），这些独立的进程间可以互相通信，共同完成任务。由于操作系统对进程提供了大量的保护机制，以避免一个进程修改了另一个进程的数据，使用多进程比多线程更容易写出安全的代码。但是这也造就了多进程并发的两个缺点：

    1、在进程间的通信，无论是使用信号、套接字，还是文件、管道等方式，其使用要么比较复杂，要么就是速度较慢或者两者兼而有之。
    2、运行多个线程的开销很大，操作系统要分配很多的资源来对这些进程进行管理。

由于多个进程并发完成同一个任务时，不可避免的是操作同一个数据和进程间的相互通信，上述的两个缺点也就决定了多进程的并发不是一个好的选择。

### 2、多线程并发
优点：

    有操作系统相关知识的应该知道，线程是轻量级的进程，每个线程可以独立的运行不同的指令序列，但是线程不独立的拥有资源，依赖于创建它的进程而存在。也就是说，同一进程中的多个线程共享相同的地址空间，可以访问进程中的大部分数据，指针和引用可以在线程间进行传递。这样，同一进程内的多个线程能够很方便的进行数据共享以及通信，也就比进程更适用于并发操作。

缺点：

    由于缺少操作系统提供的保护机制，在多线程共享数据及通信时，就需要程序员做更多的工作以保证对共享数据段的操作是以预想的操作顺序进行的，并且要极力的避免死锁(deadlock)。

c++主线程和子线程
参考blog:

    https://blog.csdn.net/weixin_45252450/article/details/104104669
    https://blog.csdn.net/qq_24649627/article/details/112237395
    https://www.cnblogs.com/chang-yuan/p/15202084.html

# 二、thread

## 1、join()创建线程
```c++
void funa()
{
	cout << "funa()" << endl;
}
void funb(int a)
{
	cout << "funb(int a)" << endl;
}
void func(int& a)
{
	a += 10;
	cout << "func(int& a)" << endl;
}
void fund(int* p)
{
	if (p == nullptr) return;
	*p += 100;
	cout << "fund(int* p)" << endl;
}

int main()
{
	int x = 10;
	std::thread tha(funa);
	std::thread thb(funb, x);
	std::thread thc(func, std::ref(x));//需要引用不能直接给，需要ref告诉其为引用类型
	std::thread thd(fund, &x);

	tha.join(); //等待其他线程结束
	thb.join();
	thc.join();
	thd.join();
	cout << x << endl;

	return 0;
}

```

## 2、detach()创建线程
```c++
int main()
{
	std::thread tha(funa);
	tha.detach(); //主线程与开辟线程没有关系，主线程可以首先结束
	cout << "main end" << endl;
	return 0;
}
```

## 3、join与detach的区别


    detach方式: 启动的线程自主在后台运行，当前的代码继续往下执行，不等待新线程结束。
    join方式:   等待启动的线程完成，才会继续往下执行。

### (1)join举例

子线程加入到主线程中
主线程会等待子线程结束。如果子线程未结束，主线程就会一直阻塞，等待所有join的子线程结束后才能结束。

```C++
#include <iostream>
#include <thread>
using namespace std;
void thread_1()
{
    while(1)
    {
        //cout<<"子线程1111"<<endl;
    }
}
void thread_2(int x)
{
    while(1)
    {
        //cout<<"子线程2222"<<endl;
    }
}
int main()
{
    thread first ( thread_1);     // 开启线程，调用：thread_1()
    thread second (thread_2,100);  // 开启线程，调用：thread_2(100)

    first.join();                // pauses until first finishes 这个操作完了之后才能destroyed
    second.join();               // pauses until second finishes//join完了之后，才能往下执行？？
    while(1)
    {
        std::cout << "主线程\n";
    }
    return 0;
}
```
### (2)detach举例

```c++
#include <iostream>
#include <thread>
using namespace std;
void thread_1()
{
    while(1)
    {
        cout<<"子线程1111"<<endl;
    }
}
void thread_2(int x)
{
    while(1)
    {
        cout<<"子线程2222"<<endl;
    }
}
int main()
{
    thread first ( thread_1);     // 开启线程，调用：thread_1()
    thread second (thread_2,100);  // 开启线程，调用：thread_2(100)

    first.detach();                
    second.detach();            
    for(int i = 0; i < 10; i++)
    {
        std::cout << "主线程\n";
    }
    return 0;//主线程与开辟线程没有关系，主线程可以首先结束
}

```

## this_thread

this_thread是一个类，它有4个功能函数，具体如下：

|  函数   | 使用  |说明|
|  ----  | ----  |----|
| get_id  | std::this_thread::get_id()	 |获取线程id|
| yield  | std::this_thread::yield()	 |放弃线程执行，回到就绪状态|
|sleep_for|std::this_thread::sleep_for(std::chrono::seconds(1));	|暂停1秒|
|sleep_until|如下|一分钟后执行|

```c++
    using std::chrono::system_clock;
    std::time_t tt = system_clock::to_time_t(system_clock::now());

    struct std::tm * ptm = std::localtime(&tt);
    cout << "Waiting for the next minute to begin...\n";
    ++ptm->tm_min; //加一分钟
    ptm->tm_sec = 0; //秒数设置为0
    //暂停执行，到下一整分执行
    this_thread::sleep_until(system_clock::from_time_t(mktime(ptm)));
```

# 三、mutex

mutex头文件主要声明了与互斥量(mutex)相关的类。mutex提供了4种互斥类型，如下所示。
std::mutex	
std::recursive_mutex
std::time_mutex	
std::recursive_timed_mutex	

## 死锁

    死锁：是指两个或两个以上的进程在执行过程中，由于竞争资源或者由于彼此通信而造成的一种阻塞的现象，若无外力作用，它们都将无法推进下去。此时称系统处于死锁状态或系统产生了死锁，这些永远在互相等待的进程称为死锁进程。

## 1、lock与unlock

    lock()：资源上锁
    unlock()：解锁资源
    trylock()：查看是否上锁，它有下列3种类情况：
    （1）未上锁返回false，并锁住；
    （2）其他线程已经上锁，返回true；
    （3）同一个线程已经对它上锁，将会产生死锁。

同一个mutex变量上锁之后，一个时间段内，只允许一个线程访问它。
这样可以避免两个线程同时修改共同的全局变量

```C++
#include <iostream>       // std::cout
#include <thread>         // std::thread
#include <mutex>          // std::mutex
std::mutex mtx;           // mutex for critical section

void print_block (int n, char c) {
  // critical section (exclusive access to std::cout signaled by locking mtx):
  mtx.lock();
  for (int i=0; i<n; ++i) { std::cout << c; }
  std::cout << '\n';
  mtx.unlock();
}
int main ()
{
  std::thread th1 (print_block,50,'*');//线程1：打印*
  std::thread th2 (print_block,50,'$');//线程2：打印$

  th1.join();
  th2.join();

  return 0;
}
```

输出：

    **************************************************
    $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

如果是不同mutex变量，因为不涉及到同一资源的竞争，所以下列代码运行可能会出现交替打印的情况，或者另一个线程可以修改共同的全局变量

```c++
#include <iostream>       // std::cout
#include <thread>         // std::thread
#include <mutex>          // std::mutex

std::mutex mtx_1;           // mutex for critical section
std::mutex mtx_2;           // mutex for critical section

int test_num = 1;

void print_block_1 (int n, char c) {
  // critical section (exclusive access to std::cout signaled by locking mtx):
  mtx_1.lock();
  for (int i=0; i<n; ++i) {
      //std::cout << c;
      test_num = 1;
      std::cout<<test_num<<std::endl;
  }
  std::cout << '\n';
  mtx_1.unlock();
}
void print_block_2 (int n, char c) {
  // critical section (exclusive access to std::cout signaled by locking mtx):
  mtx_2.lock();
  test_num = 2;
  for (int i=0; i<n; ++i) {
      //std::cout << c;
      test_num = 2;
      std::cout<<test_num<<std::endl;
  }
  mtx_2.unlock();
}

int main ()
{
  std::thread th1 (print_block_1,10000,'*');
  std::thread th2 (print_block_2,10000,'$');

  th1.join();
  th2.join();

  return 0;
}

```

## 2、lock_guard

创建lock_guard对象时，它将尝试获取提供给它的互斥锁的所有权。当控制流离开lock_guard对象的作用域时，lock_guard析构并释放互斥量。

lock_guard的特点：

    创建的时候就加锁了，作用域结束自动析构并解锁，无需手工解锁
    不能中途解锁，必须等作用域结束才解锁
    不能复制

```C++
#include <thread>
#include <mutex>
#include <iostream>

int g_i = 0;
std::mutex g_i_mutex;  // protects g_i，用来保护g_i

/*
该程序的功能为，每经过一个线程，g_i 加1。
因为涉及到共同资源g_i ，所以需要一个共同mutex：g_i_mutex。
main线程的id为1，所以下次的线程id依次加1。
*/
void safe_increment()
{
    const std::lock_guard<std::mutex> lock(g_i_mutex);
    ++g_i;
    std::cout << std::this_thread::get_id() << ": " << g_i << '\n';
    // g_i_mutex自动解锁,不需要手动解锁
}

int main()
{
	std::cout << "main id: " <<std::this_thread::get_id()<<std::endl;
    std::cout << "main: " << g_i << '\n';

    std::thread t1(safe_increment);
    std::thread t2(safe_increment);

    t1.join();
    t2.join();

    std::cout << "main: " << g_i << '\n';
}

```

## 3、 unique_lock

简单地讲，unique_lock 是 lock_guard 的升级加强版，它具有 lock_guard 的所有功能，同时又具有其他很多方法，使用起来更强灵活方便，能够应对更复杂的锁定需要。

特点：

    创建时可以不锁定（通过指定第二个参数为std::defer_lock），而在需要时再锁定
    可以随时加锁解锁
    作用域规则同 lock_grard，析构时自动释放锁
    不可复制，可移动
    条件变量需要该类型的锁作为参数（此时必须使用unique_lock）

```c++

/*
该函数的作用是，从一个结构体中的变量减去一个num，加载到另一个结构体的变量中去。
std::mutex m;在结构体中，mutex不是共享的。但是只需要一把锁也能锁住，因为引用传递后，同一把锁传给了两个函数。
cout需要在join后面进行，要不然cout的结果不一定是最终算出来的结果。
std::ref 用于包装按引用传递的值。
std::cref 用于包装按const引用传递的值。
*/
#include <mutex>
#include <thread>
#include <iostream>
struct Box {
    explicit Box(int num) : num_things{num} {}

    int num_things;
    std::mutex m;
};

void transfer(Box &from, Box &to, int num)
{
    // defer_lock表示暂时unlock，默认自动加锁
    std::unique_lock<std::mutex> lock1(from.m, std::defer_lock);
    std::unique_lock<std::mutex> lock2(to.m, std::defer_lock);

    //两个同时加锁
    std::lock(lock1, lock2);//或者使用lock1.lock()

    from.num_things -= num;
    to.num_things += num;
    //作用域结束自动解锁,也可以使用lock1.unlock()手动解锁
}

int main()
{
    Box acc1(100);
    Box acc2(50);

    std::thread t1(transfer, std::ref(acc1), std::ref(acc2), 10);
    std::thread t2(transfer, std::ref(acc2), std::ref(acc1), 5);

    t1.join();
    t2.join();
    std::cout << "acc1 num_things: " << acc1.num_things << std::endl;
    std::cout << "acc2 num_things: " << acc2.num_things << std::endl;
}

```

# 3. condition_variable

. . . . . . . . 
 
# ss

当线程启动后，一定要在和线程相关联的thread销毁前，确定以何种方式等待线程执行结束。可以使用joinable判断是join模式还是detach模式。
```c++
    if (myThread.joinable()) foo.join();
```

