# baseline

设baseline的为$b$，且$b$与动作A无关，下面证明
$$\mathbb{E}_{A \sim \pi} \left[b\frac{\partial \log \pi(A \mid s, \boldsymbol{\theta})}{\partial \boldsymbol{\theta}} \right] = 0$$

$$
\begin{aligned}
\mathbb{E}_{A \sim \pi} \left[b \ \frac{\partial \ln \pi(A \mid s, \boldsymbol{\theta})}{\partial \boldsymbol{\theta}} \right] 
& =b \ \mathbb{E}_{A \sim \pi} \left[\frac{\partial \ln \pi(A \mid s, \boldsymbol{\theta})}{\partial \boldsymbol{\theta}} \right] --(b与A无关)\\
& = b\ \sum_a \pi(a \mid s ; \boldsymbol{\theta}) \frac{{\partial \ln \pi(a \mid s; \boldsymbol{\theta})}}{\partial \boldsymbol{\theta}}--(展开写成连加的形式)\\
& = b\ \sum_a \pi(a \mid s ; \boldsymbol{\theta}) \frac{1}{\pi(a \mid s; \boldsymbol{\theta}) }\cdot \frac{{\partial \pi(a \mid s; \boldsymbol{\theta})}}{\partial \boldsymbol{\theta}} --(链式法则展开导数)\\
& =b\ \sum_a \frac{{\partial \pi(a \mid s; \boldsymbol{\theta})}}{\partial \boldsymbol{\theta}}\\
& =b\  \frac{{\partial \sum_a\pi(a \mid s; \boldsymbol{\theta})}}{\partial \boldsymbol{\theta}}--(连加与求导的对象不同)\\
& =b \frac{\partial1}{\partial \boldsymbol{\theta}}\\
& = 0
\end{aligned}
$$

既然这一项恒等于0，那么就可以加到我们的策略梯度中：

$$
\begin{aligned}
\frac{\partial V(s ; \boldsymbol{\theta})}{\partial \boldsymbol{\theta}}
& = \mathbb{E}_{A \sim \pi(\cdot \mid s ; \boldsymbol{\theta})}\left[\frac{\partial \log \pi(A \mid s, \boldsymbol{\theta})}{\partial \boldsymbol{\theta}} \cdot Q_\pi(s, A)\right] - \mathbb{E}_{A \sim \pi} \left[b\frac{\partial \log \pi(A \mid s, \boldsymbol{\theta})}{\partial \boldsymbol{\theta}} \right]\\
& = \mathbb{E}_{A \sim \pi(\cdot \mid s ; \boldsymbol{\theta})}\left[\frac{\partial \log \pi(A \mid s, \boldsymbol{\theta})}{\partial \boldsymbol{\theta}} \cdot (Q_\pi(s, A) - b)\right]
\end{aligned}
$$

只要$b$与动作$A$无关，那么得到的期望就不变

# 目的

* 虽然$b$不会影响期望，但是实际应用中，我们用的不是真正的期望，而是蒙特卡洛近似。$b$会影响蒙特卡洛近似。

* 如果函数$b$选取的比较好，比较接近于$Q_{\pi}$，就会让蒙特卡洛近似的方差降低，加快收敛。

为了方便计算，令$\left[\frac{\partial \log \pi(A_t \mid s, \boldsymbol{\theta})}{\partial \boldsymbol{\theta}} \cdot Q_\pi(s, A) - b\right] = g(A_t)$。它依赖于随机变量$A_t$
根据策略函数$a_t\sim \pi(\cdot \mid s ; \boldsymbol{\theta})$抽样得到动作$a_t$，并且计算$g(a_t)$。得到的$g(a_t)$就是期望的蒙特卡洛近似。显然$g(a_t)$是策略梯度的一个无偏估计，这是因为：
$$\mathbb{E}_{A \sim \pi(\cdot \mid s ; \boldsymbol{\theta})}\left[g(A_t)\right] = \frac{\partial V(s ; \boldsymbol{\theta})}{\partial \boldsymbol{\theta}} $$

$$ \frac{\partial V(s ; \boldsymbol{\theta})}{\partial \boldsymbol{\theta}} = \mathbb{E}_{A \sim \pi(\cdot \mid s ; \boldsymbol{\theta})}\left[\frac{\partial \log \pi(A \mid s, \boldsymbol{\theta})}{\partial \boldsymbol{\theta}} \cdot (Q_\pi(s, A) - b) \right]$$

$g(a_t)$其实是个随机梯度，是对策略梯度的蒙特卡洛近似，实际训练的时候，大家用但都是随机梯度$g(a_t)$

$$g(a_t) = \left[\frac{\partial \log \pi(a_t \mid s, \boldsymbol{\theta})}{\partial \boldsymbol{\theta}} \cdot (Q_\pi(s, a) - b) \right]$$

用随机梯度$g(a_t)$来进行梯度上升，更新$\theta$

$$\boldsymbol{\theta} \leftarrow \boldsymbol{\theta}+\beta \cdot g(a_t)$$

虽然$b$不用影响$\mathbb{E}_{A \sim \pi(\cdot \mid s ; \boldsymbol{\theta})}\left[g(A_t)\right]$，但是它显然会影响具体的$g(a_t)$

# $b$ 的选择

## 1. $b = 0$

标准形式：

$$
\frac{\partial V(s ; \boldsymbol{\theta})}{\partial \boldsymbol{\theta}}=\mathbb{E}_{A \sim \pi(\cdot \mid s ; \boldsymbol{\theta})}\left[\frac{\partial \log \pi(A \mid s, \boldsymbol{\theta})}{\partial \boldsymbol{\theta}} \cdot Q_\pi(s, A)\right]
$$


## 2. $b = V_{\pi}(s_t)$

* $V_{\pi}(s_t)$ 是先于动作$A_t$被观测到的，与$A_t$无关，所以可以作为baseline。
* $V_{\pi}(s_t)$就很接近于$Q_{\pi}$ ，因为：$V_{\pi}(s_t)$是$Q_{\pi}$的期望：

$$V_{\pi}(s_t) =  \mathbb{E}_{A_t}\left[Q_\pi(s_t, A_t)\right]$$

* 所以可以使方差减小，加速收敛

$$\frac{\partial V(s_t)}{\partial \boldsymbol{\theta}} = = \mathbb{E}_{A_t \sim \pi}  \underbrace{ \left[\frac{\partial \ln \pi(A_t \mid s_t, \boldsymbol{\theta})}{\partial \boldsymbol{\theta}} \cdot (Q_\pi(s_t, A_t) -  V_{\pi}(s_t) ) \right]}_{g(A_t)}$$
