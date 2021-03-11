#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>// close read write unix标准函数定义
#include <stdio.h>
#include <string.h>
#include "gps.h"

/*
 * sudo ./gps_app /dev/ttymxc2
 */
int main(int argc, char **argv)
{
	int fd;
	char *filename;
	int ret = 0;
    char buf[6] = {};

	if (argc != 2) {
		printf("Error Usage!\r\n");
		return -1;
	}

	filename = argv[1];
    /* O_NOCTTY:表示打开的是一个终端设备，程序不会成为该端口的控制终端。如果不使用此标志，任务一个输入(eg:键盘中止信号等)都将影响进程。
         O_NDELAY:表示不关心DCD信号线所处的状态（端口的另一端是否激活或者停止）*/
	fd = open(filename, O_RDWR | O_NOCTTY | O_NDELAY);
	if(fd < 0) {
		printf("can't open file %s\r\n", filename);
		return -1;
	}
    else {
        printf("open success!\n");
    }

    short data[6];
    printf("sizeof(data[6])=%d\n", sizeof(data));

	while (1) 
    {
        read(fd, buf, 1);
        printf("buf = %c\n", buf[0]);
		usleep(1000); /*1000ms */
	}
	close(fd);	/* 关闭文件 */	
	
	return 0;
}


