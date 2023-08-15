# 

相邻两个回报之间的关系：
$$
\begin{aligned}
U_t & =R_t+\gamma \cdot R_{t+1}+\gamma^2 \cdot R_{t+2}+\gamma^3 \cdot R_{t+3}+\gamma^4 \cdot R_{t+4}+\cdots \\
& =R_t+\gamma \cdot \underbrace{\left(R_{t+1}+\gamma \cdot R_{t+2}+\gamma^2 \cdot R_{t+3}+\gamma^3 \cdot R_{t+4}+\cdots\right)}_{=U_{t+1}}
\end{aligned}
$$

$$ U_t = R_t +\gamma \cdot U_{t+1} $$

通常认为奖励 $R_t$ 依赖于 $S_t, A_T, S_{t+1}$
动作价值函数是回报的期望，是对未来所有的状态和价值求的，从而消除了未来的状态和动作的不确定性

$$ 
\begin{aligned}
Q_\pi\left(s_t, a_t\right)
&=\mathbb{E}\left[U_t \mid s_t, a_t\right] \\
&= \mathbb{E}\left[R_t +\gamma \cdot U_{t+1} \mid s_t, a_t\right]\\
&=\mathbb{E}\left[R_t \mid s_t, a_t\right]+\gamma \cdot \mathbb{E}\left[  U_{t+1} \mid s_t, a_t\right]
\end{aligned}
$$

而后面的一项是对$U_t+1$的期望，而$Q_\pi\left(S_{t+1}, A_{t+1}\right)$，本身就是对$U_t+1$的期望，所以可以这样替换为期望的期望，结果不变：

$$\mathbb{E}\left[U_{t+1} \mid s_t, a_t\right]=\mathbb{E}\left[Q_\pi\left(S_{t+1}, A_{t+1}\right) \mid s_t, a_t\right]$$

替换之后，t时刻的动作价值可以写为：

$$Q_\pi\left(s_t, a_t\right)=\mathbb{E}\left[R_t +\gamma \cdot Q_\pi\left(S_{t+1}, A_{t+1}\right)\right]$$

注意，这里的期望是对$S_{t+1}$和$a_{t+1}$求的。

期望不好求，所以通常对期望做蒙特卡洛近似：

用观测到的奖励 $r_t$ 代替随机变量 $R_t$ 的期望，用观测到的$s_{t+1}, a_{t+1}$代替$S_{t+1}, A_{t+1}$

$$
\underbrace{Q\left(s_t, a_t ; \mathbf{w}\right)}_{\text {Prediction}} \approx \underbrace{r_t+\gamma \cdot Q\left(s_{t+1}, a_{t+1} ; \mathbf{w}\right)}_{\text {TD target}}
$$

我们鼓励$Q_\pi\left(s_t, a_t\right)$去接近TD-target $y_t$

# 表格形式的Sarsa

动作和环境的状态的维度比较少

表格相当于一个$Q$函数，可以输入$s_t$和$a_t$，输出动作价值$Q$

也就是查询对应行索引为$s_t$列索引为$a_t$的格子，各自里面填的是Q

# 神经网络形式的


