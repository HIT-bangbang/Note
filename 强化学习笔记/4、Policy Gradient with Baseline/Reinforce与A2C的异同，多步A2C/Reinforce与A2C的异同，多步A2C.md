# 一、回顾

A2C算法:

<img src="1.png" height=100% width=100%>

REINFORCE with Baseline：

<img src="4.png" height=100% width=100%>

* 对比可以发现，这两种方法只是$u_t$和$y_t$不同

# 二、One-Step Target VS Multi-Step Target

m-step Target

<img src="2.png" height=100% width=100%>


A2C with m-step Target

<img src="3.png" height=100% width=100%>

m-step Target效果要比One-Step Target要好

# 联系

当利用一整局游戏时，m-step Target其实使用了从当前时刻到游戏结束所有的奖励，也就是回报return，那么后面的一项近似状态价值$v(s_{t+m};\mathbf{w})$就可以被丢掉了

<img src="5.png" height=100% width=100%>

算法也就变成了Reinforce




