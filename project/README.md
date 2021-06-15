# app
# 线程管理，初始化相关内容
### 2021/3/26
### 添加sensor线程，读取解析bmi088，hmc5883驱动数据,采样频率1hz

### 2021/3/29
### 在sensor线程中添加Posix定时器,需引入lrt库
### 将$(LINK)放到命令行尾，修复makefile链接-lrt库不成功问题

### 2021/3/31
### 在sensor线程中添加信号量操作

### 2021/4/13
### 在sensor线程中添加sensor_loop

### 2021/4/20
### 添加uDEU.c 修改相应makefile

### 2021/4/21
### 完善uDEU中基本操作函数, DEFINE,publish,subscribe,advertise

### 2021/4/22
### DEFINE,publish,subscribe,advertise单一节点测试,不满足条件未测试
### 修改DEFINE中错误,修改NULL指针判断,增加断言assert
### 添加互斥量设计临界段保护多线程涉及到的公共变量(代测试)

### 2021/4/27
### 添加ringbuffer

### 2021/4/28
### 添加test线程用于测试框架内软件包
### bug:主线程中创建线程只能顺序执行
### bug:test订阅话题中没有话题发布但是没有产生阻塞

### 2021/4/29
### 解决bug，deu_poll_sync还没到sem_wait就返回-1，导致不会阻塞
### 使用sleep可以正常切换线程
### 测试多线程下单线程pub,多线程deu_poll_sync

### 2021/4/30
### 添加uart api

### 2021/5/12
### 添加imu040b解析部分(待测试)

### 2021/5/13
### 解决bug:uart_open内获取的文件描述符fd没有输出，导致后面配置的是描述符0
### 发现buf:自从加上uart_set以后，开始出现input/output error
### 使用uart.c接收imu040 hex测试完成

### 2021/5/18
### uart_recv返回值改为len
### 测试usleep
### imu040b单字节解析改为多字节

### 2021//6/1
### 添加log文件，内置线程入口

### 编程规范
只有对外需要使用的函数才作为全局函数使用,其他函数名前应用static修饰
静态函数名前加_
