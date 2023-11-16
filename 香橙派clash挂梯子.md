# 以下方法针对xcfe桌面

## 1、修改 /etc/environment 文件 

原来文件内容为:

    PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin"

在后面添加：

    export http_proxy="http://127.0.0.1:7890"  # 这个根据自己的梯子修改
    export https_proxy="http://127.0.0.1:7890" # 这个根据自己的梯子修改
    export no_proxy="http://localhost, http://127.0.0.1"

## 2、安装clash，配置梯子

## 3、安装proxychains工具

sudo apt-get install proxychains

修改配置文件/etc/poxychains4 

最后面改为

    # socks4 	127.0.0.1 9050  # 原来文件的这一行注释掉
    socks5 127.0.0.1 7891

## 4、启动clash 完成

# gnome桌面就直接在配置里面改就行