
从FMUv6X（H7飞控）开始，bootloader编译在飞控固件的源代码里面进行就可以了
Boards starting with FMUv6X (STM32H7) use the in-tree PX4 bootloader. Older boards use the bootloader from the legacy PX4 bootloader (opens new window) repository. Please refer to the instructions in the README to learn how to use it.

例如：
make px4_fmu-v6x_bootloader

make matek_h743-slim_bootloader

如果是老芯片的飞控，就需要下载PX4的bootloader仓库，在那里面编译

https://docs.px4.io/main/en/advanced_config/bootloader_update_from_betaflight.html

刷写bootloader

按照官方文档来

dfu-util -a 0 --dfuse-address 0x08000000 -D  matek_h743-slim_bootloader.bin

