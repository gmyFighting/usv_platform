#ifndef _SENSOR_COLLECT_H_
#define _SENSOR_COLLECT_H_

struct sensor_file_addr {
    char *mems_file;
    char *mag_file;
};

struct uart_devieve_addr{
	int uart_sensor_num;
	char *ublox;
	char *dvl;
	char *imu04b;
};
void* sensor_collect_func(void *arg);


#endif // _SENSOR_COLLECT_H_
