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
### 修改DEFINE中错误,修改NULL指针判断,增加断言assert等