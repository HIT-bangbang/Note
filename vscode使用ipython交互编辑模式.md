# 在python中使用‘# %%‘快速进入ipython交互编辑模式

vscode的python代码可以使用# %%字符，# %% 后的代码会变为一个cell单独运行，即ipython或jupyter中的cell。不过看上去比完整的jupyter cell略显简陋。
点击运行的话，vscode会启动jupyter服务来显示运行结果。也可以通过 shift + 回车运行当前cell。

测试发现，在vscode和PyCharm中通过输入# %%都能够快速进入ipython的交互编辑模式。
使用

最后通过查找vscode的官方文档发现了该命令的作用：Python Interactive window

通过 # %% 进入交互模式，文件最后保存为.py文件，不是.ipynb。

进入该模式后，使用方式同jupyter相同，可以使用ipython的魔法方法；使用markdown编辑文字；直接显示plt绘图等。
