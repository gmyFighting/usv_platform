#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stddef.h>// NULL
#include <unistd.h>// sleep
#include "bmi088.h"
#include "uDEU.h"

extern struct deu_topic imu;

void* user_test_func(void *arg)
{
    printf("in user_test_func\n");
    sleep(1);// 跳转到其他线程
    int n = 10;
    DeuNode_t user_node = NULL;
    sem_t usr_sem;
    struct bmi088_data imu_sample = {0};

    sem_init(&usr_sem, 0, 0);

    user_node = deu_subscribe(&imu, &usr_sem, NULL);
    if (user_node == NULL) {
        printf("deu_subscribe failed\n");
        return (void *)(-1);        
    }

    while (n--) {
        // printf("in user_test_func while\n");
        if(deu_poll_sync(&imu, user_node, &imu_sample) == 0) {
            printf("test:ax=%f, ay=%f, az=%f, gx=%f, gy=%f, gz=%f\r\n", \
                imu_sample.acc_x, imu_sample.acc_y, imu_sample.acc_z, 
                imu_sample.gyr_x, imu_sample.gyr_y, imu_sample.gyr_z);
            // printf("out user_test_func while\n");
        }
    }
    // printf("out user_test_func\n");
    return (void *)0;
}