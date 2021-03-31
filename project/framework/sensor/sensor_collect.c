#include <stdio.h>
#include <unistd.h>
#include <fcntl.h> // file contrl
#include <time.h>
#include <signal.h>
#include <semaphore.h> // sem
#include "sensor_collect.h"
#include "bmi088.h"
#include "hmc5883.h"

static sem_t sensor_sem;

void sig_handler(int sig_num)
{
    if (sig_num == SIGUSR1) {
        printf("Capture sign No.=SIGUSR1\n"); 
        // 发送信号量
        sem_post(&sensor_sem);
    } 
}

void* sensor_collect_func(void *arg)
{
    struct sensor_file_addr *fil = (struct sensor_file_addr *)arg; 
    int n = 10;
    int fd_mems, fd_mag, fd_gps;
    short mems_buf[6];
    short mag_buf[3];
    struct bmi088_data imu_sample;
    struct hmc5883_data mag_sample;
    // 创建定时器
    timer_t timer;
    // 异步通知
    struct sigevent evp;
    // 定时器
    struct itimerspec ts;
    evp.sigev_notify = SIGEV_SIGNAL;// 通知方法:指定时器到期时会产生信号  
    evp.sigev_signo = SIGUSR1;// 通知信号类型  
    evp.sigev_value.sival_ptr = &timer;// 发出信号的定时器,用于多个定时器发同一信号
    
    // 登记信号
    signal(evp.sigev_signo, sig_handler);// kill -l中前32个非实时信号安装
    sem_init(&sensor_sem, 0, 0);

    // 创建定时器,此时不启动
    if (timer_create(CLOCK_REALTIME, &evp, &timer) < 0) {
        printf("timer create failed\n");
    }   

    fd_mems = open(fil->mems_file, O_RDWR);
    if (fd_mems < 0) {
		printf("can't open file %s\r\n", fil->mems_file);
		return NULL;        
    }
    else {
        printf("open file sucess:%s \n", fil->mems_file);
    }

    fd_mag = open(fil->mag_file, O_RDWR);
    if (fd_mag < 0) {
        printf("can't open file %s\r\n", fil->mag_file);
        return NULL;
    }
    else {
        printf("open file sucess:%s \n", fil->mag_file);
    }
    
    ts.it_interval.tv_sec = 1; // the spacing time  
    ts.it_interval.tv_nsec = 0;  
    ts.it_value.tv_sec = 1;  // the delay time start
    ts.it_value.tv_nsec = 0;
    // 设置完定时器直接启动
    if (timer_settime(timer, 0, &ts, NULL) < 0) {
        printf("timer_settime failed\n"); 
    } 

    while (n--) {
        if (sem_wait(&sensor_sem) == 0) {
            read(fd_mems, mems_buf, sizeof(mems_buf));
            bmi088_get_data(mems_buf, &imu_sample);
            printf("ax=%f, ay=%f, az=%f, gx=%f, gy=%f, gz=%f\r\n", \
                imu_sample.acc_x, imu_sample.acc_y, imu_sample.acc_z, 
                imu_sample.gyr_x, imu_sample.gyr_y, imu_sample.gyr_z);
            
            read(fd_mag, mag_buf, sizeof(mag_buf));
            hmc5883_get_data(mag_buf, &mag_sample);
            printf("mx=%f, my=%f, mz=%f\r\n", \
                mag_sample.x, mag_sample.y, mag_sample.z);
        }
    }
    close(fd_mems);
    close(fd_mag);
    return NULL;
    /*退出线程*/
    // pthread_exit(NULL);
}