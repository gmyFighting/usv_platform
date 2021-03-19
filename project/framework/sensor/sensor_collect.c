#include <stdio.h>
#include <unistd.h>

/*
 * sudo ./bmi088_app /dev/bmi088
 * sudo ./hmc5883_app /dev/hmc5883
 */

void* sensor_collect_func(void *arg)
{
    int num = arg; /** sizeof(void*) == 8 and sizeof(int) == 4 (64 bits) */
    
    while (1) {
        printf("This is a thread for sensor collect, arg is %d\n", num);
        sleep(2);// 2s
    }
    /*退出线程*/
    // pthread_exit(NULL);
}