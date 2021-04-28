CROSS_COMPILE ?= arm-linux-gnueabihf-
TARGET        ?= usv_plateform
LINK          := -lrt -pthread 
CC 			  := $(CROSS_COMPILE)gcc

INCDIRS := framework/include     \
			application/include  \
			driver/include 

SRCDIRS := framework/sensor      \
			framework/uDEU       \
			framework/ringbuffer \
			application          \
			driver/bmi088        \
			driver/hmc5883 

# 遍历INCDIRS，将各个变量前加-I
INCLUDE         := $(patsubst %, -I %, $(INCDIRS))

# foreach把<SRCDIRS>中的单词逐一取出放到变量<dir>中，再执行<text>表达式
# wildcard 将所有变量合并写一起
CFILES          := $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.c))
# notdir 去掉文件名的目录 *.c
CFILENDIR		:= $(notdir  $(CFILES))
# 将所有.c换成.o，然后在前面加上目录，编译出的o文件都放到/obj下 obj/*.o
COBJS			:= $(patsubst %, obj/%, $(CFILENDIR:.c=.o))

VPATH			:= $(SRCDIRS)
# -c 只编译源文件但不链接为可执行文件，编译器将输入的.c 文件编译为.o 的目标文件
# -o 用来指定编译结束以后的输出文件名
# -g 产生调试信息
# 目标 : 依赖文件集合
# $(TARGET): $(COBJS)
# 	$(CC) $(LINK) -o $(TARGET) $(OBJS)
# !$(LINK)要放在命令最后面，否则有时会出现链接不到库的问题
$(TARGET): $(COBJS)
	$(CC) -o obj/$(TARGET) $(COBJS) $(LINK)
$(COBJS): obj/%.o : %.c
	$(CC) -c $(INCLUDE) -o $@ $<

# 声明clean为伪目标,make目录下有同名文件也可以执行clean目标
.PHONY: clean
clean: 
	rm obj/*.o
	rm obj/$(TARGET)

