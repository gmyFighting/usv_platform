#include <stdio.h>
#include <unistd.h>
#include <fcntl.h> // file contrl
#include "sensor_collect.h"
#include "bmi088.h"
#include "hmc5883.h"

/*
 * sudo ./bmi088_app /dev/bmi088
 * sudo ./hmc5883_app /dev/hmc5883
 */
void* sensor_collect_func(void *arg)
{
    struct sensor_file_addr *fil = (struct sensor_file_addr *)arg; /** sizeof(void*) == 8 and sizeof(int) == 4 (64 bits) */
    int n = 10;
    int fd_mems, fd_mag, fd_gps;
    short mems_buf[6];
    short mag_buf[3];
    struct bmi088_data imu_sample;
    struct hmc5883_data mag_sample;
    // 打开各个传感器驱动
    // fil->mems_file = "/dev/bmi088";
    fd_mems = open(fil->mems_file, O_RDWR);
    if (fd_mems < 0) {
		printf("can't open file %s\r\n", fil->mems_file);
		return;        
    }
    else {
        printf("open file sucess:%s \n", fil->mems_file);
    }

    fd_mag = open(fil->mag_file, O_RDWR);
    if (fd_mag < 0) {
        printf("can't open file %s\r\n", fil->mag_file);
        return;
    }
    else {
        printf("open file sucess:%s \n", fil->mag_file);
    }

    while (n--) {
        read(fd_mems, mems_buf, sizeof(mems_buf));
        bmi088_get_data(mems_buf, &imu_sample);
        printf("ax=%f, ay=%f, az=%f, gx=%f, gy=%f, gz=%f\r\n", \
            imu_sample.acc_x, imu_sample.acc_y, imu_sample.acc_z, 
            imu_sample.gyr_x, imu_sample.gyr_y, imu_sample.gyr_z);
        
        read(fd_mag, mag_buf, sizeof(mag_buf));
        hmc5883_get_data(mag_buf, &mag_sample);
        printf("mx=%f, my=%f, mz=%f\r\n", \
            mag_sample.x, mag_sample.y, mag_sample.z);
        sleep(1);// 1s
    }
    close(fd_mems);
    /*退出线程*/
    // pthread_exit(NULL);
}