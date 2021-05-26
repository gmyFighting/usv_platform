#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>// exit()
#include "sensor_collect.h"
#include "test.h"
#include "ringbuffer.h"
#include "client.h"
#include "server.h"

// 驱动文件地址
static struct sensor_file_addr sensor_addr = {
    .mems_file = "/dev/bmi088",
    .gps_file = "/dev/gps",
    .mag_file = "/dev/hmc5883",
};
// 线程标识符
static pthread_t sensor_collect_thread;
static pthread_t test_thread;
static pthread_t tcp_thread;

int main(int argc, char **argv)
{
    int res;
    
    res = pthread_create(&sensor_collect_thread, NULL, sensor_collect_func, (void*)(&sensor_addr));
    if (res) {
        printf("create sensor thread fail\n");
        exit(res);// 退出线程
    }

    // res = pthread_create(&test_thread, NULL, user_test_func, NULL);
    // if (res) {
    //     printf("create test thread fail\n");
    //     exit(res);// 退出线程
    // }

    res = pthread_create(&tcp_thread, NULL, tcp_server_func, NULL);
    if (res) {
        printf("create tcp thread fail\n");
        exit(res);// 退出线程
    }
  
    /* 主线程具有进程行为，return后进程也会结束，进程下的所有线程都会结束
     * 使用pthread_exit后，主线程退出不会引起其创建子线程的退出
     */
    pthread_exit(0);
	return 0;
}

