# OSlab3
### 一、实现背景
本次实验是在《Orange’s ⼀个操作系统的实现》书中所带代码基础上实现的。
基础代码为书中chapter7/n中代码。
在修改makfile(在此不再赘述)之后，在本机上运行后发现其已经实现以下基本功能：
1. 回车换行
2. 空格键
3. 退格键删除(回车和TAB仍有问题)
4. 可输入a-zA-Z0-9
5. shift组合键       （教材代码已在keyboard.c中实现）
### 二、基本实现
#### 清屏
每隔20s清除屏幕，主要靠的是tty.c中的task_tty
#### TAB，回车，删除(backspace)
主要是对console.c中的out_char方法进行了改动
加上了'/n','/t'case，修改了'/b'case
#### 查找(ESC)
console.c 中新定义的find函数和stop_find函数等
还有tty.c中in_process中
#### ctrl+z
尝试实现了一下，但发现有点麻烦就搁置了。
有，但不完全
