ubuntu 的用户主和用户组

很久很久以前，我们在Ubuntu下使用软件（如minicom、screen等）访问串口时，是不需要任何超级权限的（使用minicom时，只有使用-s选项时需要root权限）；不知道从哪个版本（12.04？）开始，我们发现原来那招就不好使了；于是很多人开始习惯无论什么时候使用minicom，都使用sudo来运行，更有甚者，直接就用root帐号来登录系统了。

　　其实只要很简单的一步，就能够实现非root权限就能访问/dev/ttyS*设备了。

　　首先我们来看看为什么普通账户会没有权限访问ttyS设备了：

	ls -l /dev/ttyS0
	crw-rw---- 1 root dialout 4, 64  8月 30 21:53 /dev/ttyS0

　　从上面的输出，我们很容易看出来，ttyS设备的用户主是root，而所属的组是dialout，并且owner和group都是有相同的rw权限的，但others是没有任何权限的。

　　使用groups命令，我们就明了了：我们在安装Ubuntu时，安装时使用的账户并不会默认加入dialout组，因此该用户就没有权限可以访问ttyS设备了。

　　解决方法也非常简单：

	sudo usermod -a -G dialout user_name

　　这样，重启系统后，用户“user_name”就会加入dialout组了，之后我们就能自由自在地访问ttyS设备了，


法二，修改规则文件.rule


删除modemmanager

Ubuntu附带一个串行调制解调器管理器，它可以严重干扰任何与机器人相关的串口（或USB串口）使用。它可以删除/卸载没有副作用：

sudo apt-get remove modemmanager
