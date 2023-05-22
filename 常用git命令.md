# branch
    $ git branch #查看本地分支
    $ git branch -r #查看远程分支
    $ git branch -a #查看所有分支

    $ git branch -vv #查看本地分支及追踪的分支

    $ git branch 新分支名 #基于当前分支创建新的本地分支

# push到远程分支

将本地的 master 分支推送到 origin 主机的 master 分支

    git push origin master

等价于
    
    git push origin master:master

# checkout

切换分支

    git checkout 分支名

基于当前本地分支新建并切换到该分支

    git checkout -b 分支名

基于指定分支创建本地分支并切换到新建的本地分支

    $ git checkout -b 要新建的分支名 已有分支名 

例子：checkout远程的dev分支，本地创建名为mydev分支，并切换到本地的mydev分支
    
    $ git checkout -b mydev origin/dev 

# 创建并切换新分支

创建新分支：git branch branchName

切换到新分支：git checkout branchName

上面两个命令也可以合成为一个命令：

git checkout -b branchName

# 删除分支

    $ git branch -d 分支名 #删除本地分支
    $ git push origin --delete 分支名 #删除远程分支


# 合并分支到master

    #合并前要先切回要并入的分支，以下表示要把dev分支合并入master分支
    $ git checkout master #切换到master分支
    $ git merge dev #将dev合并到master分支
