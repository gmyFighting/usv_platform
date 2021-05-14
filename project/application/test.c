#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stddef.h>// NULL
#include <unistd.h>// sleep
#include "bmi088.h"
#include "uDEU.h"
#include "imu040b.h"
#include "uart.h"

extern struct deu_topic imu;

void* user_test_func(void *arg)
{
    char rcv_buf = 0;
    printf("in user_test_func\n");
    sleep(1);// 跳转到其他线程
    int n = 200;
    int fd = 0;
    int res;
    char *imu_name = "/dev/ttyUSB0";

    // test DEU
    // DeuNode_t user_node = NULL;
    // sem_t usr_sem;
    // struct bmi088_data imu_sample = {0};

    // sem_init(&usr_sem, 0, 0);

    // user_node = deu_subscribe(&imu, &usr_sem, NULL);
    // if (user_node == NULL) {
    //     printf("deu_subscribe failed\n");
    //     return (void *)(-1);        
    // }

    // test uart
    res = uart_open(&fd, imu_name);
    if (res != 0) {
        printf("uart open failed\n");
        return (void *)(-1);
    }
    printf("fd = %d\n", fd);
    res = uart_set(fd, B921600, 0, 8, 1, 'E');
    if (res != 0) {
        printf("uart set failed\n");
        return (void *)(-1);
    }
    
    while (1) {
        res = uart_recv(fd, &rcv_buf, 1);
        if (res > 0) {
            imu040_parse_char(rcv_buf);
            // printf("0x%x ",rcv_buf);
        }
            
        // if(deu_poll_sync(&imu, user_node, &imu_sample) == 0) {
        //     printf("test:ax=%f, ay=%f, az=%f, gx=%f, gy=%f, gz=%f\r\n", \
        //         imu_sample.acc_x, imu_sample.acc_y, imu_sample.acc_z, 
        //         imu_sample.gyr_x, imu_sample.gyr_y, imu_sample.gyr_z);
        //     // printf("out user_test_func while\n");
        // }
    }
    printf("out user_test_func\n");
    return (void *)0;
}
