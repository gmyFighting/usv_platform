#include <stdio.h>
#include <unistd.h>
#include <fcntl.h> // file contrl
#include <time.h>
#include <signal.h>
#include <semaphore.h> // sem
#include "sensor_collect.h"
#include "bmi088.h"
#include "hmc5883.h"
#include "ublox.h"
#include "uDEU.h"

#define INS_PERIOD 1
#define MAG_PERIOD 1
#define GPS_PERIOD 2

static sem_t sensor_sem;// 需要静态全局吗？
static int fd_mems, fd_mag;
static int* fd_serial;		//dvl gps imu040
static unsigned int ins_cnt = INS_PERIOD;
static unsigned int mag_cnt = MAG_PERIOD;
static unsigned int gps_cnt = GPS_PERIOD;
extern struct gps_frame gps_sample_new;

DEU_DEFINE(imu, sizeof(struct bmi088_data));
DEU_DEFINE(mag, sizeof(struct hmc5883_data));
DEU_DEFINE(gps, sizeof(struct gps_frame));


void sig_handler(int sig_num)
{
    if (sig_num == SIGUSR1) {
        // printf("Capture sign No.=SIGUSR1\n"); 
        // 发送信号量
        sem_post(&sensor_sem);
    } 
}

void sensor_loop(void)
{
    short mems_buf[6];
    short mag_buf[3];
	struct bmi088_data imu_sample = {0};
    struct hmc5883_data mag_sample = {0};
    int res;
    int sem_val;

    if (ins_cnt >= INS_PERIOD) {
        ins_cnt = 0;
        read(fd_mems, mems_buf, sizeof(mems_buf));
        bmi088_get_data(mems_buf, &imu_sample);

        deu_publish(&imu, &imu_sample);
    }

    if (mag_cnt >= MAG_PERIOD) {
        mag_cnt = 0;
        read(fd_mag, mag_buf, sizeof(mag_buf));
        hmc5883_get_data(mag_buf, &mag_sample);

		deu_publish(&mag, &mag_sample);		
    }

    if (gps_cnt >= GPS_PERIOD) {
        gps_cnt = 0;
		ublox_analys(*fd_serial);

		deu_publish(&gps, &gps_sample_new);
		
    }

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

	/* 串口初始化 */
    fd_serial = uart_deviece_init(fd_serial);
	
	/*if(-1 == fd_serial){
		printf("uart_deviece_init fail.\n");
	}*/
	
	/* 创建话题 */
	/* IMU */
	res = deu_advertise(&imu);
    if (!res) {
       printf("IMU:deu_advertise success.\n");
        		
    }else{
		printf("IMU:deu_advertise failed.\n");
		return (void *)(-1);
	}	
			
	/* MAG */
	res = deu_advertise(&mag);
    if (!res) {
        printf("MAG:deu_advertise success\n");
        	
    }else{
		printf("MAG:deu_advertise failed.\n");
		return (void *)(-1);
	}

	/* GPS */
	res = deu_advertise(&gps);
    if (!res) {
        printf("GPS:deu_advertise success\n");
    }else{
		printf("GPS:deu_advertise failed.\n");
		return (void *)(-1);
	}

    ts.it_interval.tv_sec = 0; // the spacing time  
    ts.it_interval.tv_nsec = 2000000;// 当前最小支持2ms 
    ts.it_value.tv_sec = 0;  // the delay time start
    ts.it_value.tv_nsec = 0;
    // 设置完定时器直接启动
    if (timer_settime(timer, 0, &ts, NULL) < 0) {
        printf("timer_settime failed\n"); 
    }

    while (1) {
        if (sem_wait(&sensor_sem) == 0) {// 在等待信号量过程中发生阻塞跳转到其他线程
            sensor_loop();
        }
    }

    close(fd_mems);
    close(fd_mag);
    return (void *)0;
    /*退出线程*/
    // pthread_exit(NULL);
}
