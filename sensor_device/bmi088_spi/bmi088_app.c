#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "bmi088.h"

/*
 * sudo ./bmi088_app /dev/bmi088
 *
 */
int main(int argc, char **argv)
{
	int fd;
	char *filename;
	int ret = 0;
    short buf[6];

	if (argc != 2) {
		printf("Error Usage!\r\n");
		return -1;
	}

	filename = argv[1];
	fd = open(filename, O_RDWR);
	if(fd < 0) {
		printf("can't open file %s\r\n", filename);
		return -1;
	}

    short data[6];
    printf("sizeof(data[6])=%d\n", sizeof(data));
	while (1) 
    {
        read(fd, buf, sizeof(buf));
        printf("ax=%d, ay=%d, az=%d, gx=%d, gy=%d, gz=%d\r\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
		usleep(100000); /*100ms */
	}
	close(fd);	/* 关闭文件 */	
	
	return 0;
}


