# 关于stm32f10x_conf.h


该文件在stm32f10x.h被include进来（大概在8296行）
	#ifdef USE_STDPERIPH_DRIVER
    #include "stm32f10x_conf.h"
	#endif
注意，这个宏USE_STDPERIPH_DRIVER需要在c/c++选项中添加进去

# 关于stm32f10x_it.c

一般在这里面存放中断函数，其实中断函数不管写在哪个文件中都是一样的，是一种与C语言标准生命和调用格式无关的函数，只要添加在工程中就行，无需include，在启动文件startup_stm32f10x_hd.s中已经完成了对中断的初始化。