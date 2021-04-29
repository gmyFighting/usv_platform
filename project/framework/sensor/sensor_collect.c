#include <stdio.h>
#include <unistd.h>
#include <fcntl.h> // file contrl
#include <time.h>
#include <signal.h>
#include <semaphore.h> // sem
#include "sensor_collect.h"
#include "bmi088.h"
#include "hmc5883.h"
#include "uDEU.h"

#define INS_PERIOD 1
#define MAG_PERIOD 1
#define GPS_PERIOD 2

static sem_t sensor_sem;// 需要静态全局吗？
static int fd_mems, fd_mag, fd_gps;
static unsigned int ins_cnt = INS_PERIOD;
static unsigned int mag_cnt = MAG_PERIOD;
static unsigned int gps_cnt = GPS_PERIOD;

DEU_DEFINE(imu, sizeof(struct bmi088_data));

void sig_handler(int sig_num)
{
    if (sig_num == SIGUSR1) {
        // printf("Capture sign No.=SIGUSR1\n"); 
        // 发送信号量
        sem_post(&sensor_sem);
    } 
}

void sensor_loop(DeuTopic_t top, DeuNode_t node)
{
    short mems_buf[6];
    short mag_buf[3];
    struct bmi088_data imu_sample, imu_sample1;
    struct hmc5883_data mag_sample;
    int res;
    int sem_val;

    if (ins_cnt >= INS_PERIOD) {
        ins_cnt = 0;
        read(fd_mems, mems_buf, sizeof(mems_buf));
        bmi088_get_data(mems_buf, &imu_sample);

        deu_publish(top, &imu_sample);
    }

    if (mag_cnt >= MAG_PERIOD) {
        mag_cnt = 0;
        read(fd_mag, mag_buf, sizeof(mag_buf));
        hmc5883_get_data(mag_buf, &mag_sample);
        // printf("mx=%f, my=%f, mz=%f\r\n", \
        //     mag_sample.x, mag_sample.y, mag_sample.z);
    }

    if (gps_cnt >= GPS_PERIOD) {
        gps_cnt = 0;
    }

    res = deu_poll_sync(top, node, &imu_sample1);
    printf("ax=%f, ay=%f, az=%f, gx=%f, gy=%f, gz=%f\r\n", \
        imu_sample1.acc_x, imu_sample1.acc_y, imu_sample1.acc_z, 
        imu_sample1.gyr_x, imu_sample1.gyr_y, imu_sample1.gyr_z);
    ins_cnt++;
    mag_cnt++;
    gps_cnt++;
}

void* sensor_collect_func(void *arg)
{
    // 该线程最先执行，以保证注册所有传感器话题
    printf("in sensor_collect_func\n");
    struct sensor_file_addr *fil = (struct sensor_file_addr *)arg; 
    int n = 10;
    // 创建定时器
    timer_t timer;
    // 异步通知
    struct sigevent evp;
    // 定时器
    struct itimerspec ts;
    int res = 0;

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

    fd_mag = open(fil->mag_file, O_RDWR);
    if (fd_mag < 0) {
        printf("can't open file %s\r\n", fil->mag_file);
        return NULL;
    }

    // uDEU
    sem_t imu_sem;
    DeuNode_t imu_node = NULL;
    res = deu_advertise(&imu);
    if (res < 0) {
        printf("deu_advertise failed\n");
        return (void *)(-1);
    }

    res = sem_init(&imu_sem, 0, 0);

    imu_node = deu_subscribe(&imu, &imu_sem, NULL);
    if (imu_node == NULL) {
        printf("deu_subscribe failed\n");
        return (void *)(-1);        
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
        // printf("in sensor_collect_func while\n");
        // 等待定时器发的信号量
        if (sem_wait(&sensor_sem) == 0) {// 在等待信号量过程中发生阻塞跳转到其他线程
            // printf("in sensor_loop\n");
            sensor_loop(&imu, imu_node);
            // printf("out sensor_loop\n");
        }
        // printf("out sensor_collect_func while\n");
    }
    // printf("out sensor_collect_func\n");
    close(fd_mems);
    close(fd_mag);
    return (void *)0;
    /*退出线程*/
    // pthread_exit(NULL);
}