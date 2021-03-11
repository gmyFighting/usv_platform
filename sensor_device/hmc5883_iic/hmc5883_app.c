#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "hmc5883.h"

/*
 * sudo ./hmc5883_app /dev/hmc5883
 *
 */
int main(int argc, char **argv)
{
	int fd;
	char *filename;
	struct hmc5883_3axes mag_test;
	int ret = 0;

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

	while (1) {
		ret = read(fd, &mag_test, sizeof(mag_test));
		if(ret == 0) { 			
			printf("mag_x=%d,mag_y=%d,mag_z=%d\r\n", mag_test.x, mag_test.y, mag_test.z);
		}
		usleep(200000); /*100ms */
	}
	close(fd);	/* 关闭文件 */	
	
	return 0;
}


