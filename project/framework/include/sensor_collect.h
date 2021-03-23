#ifndef _SENSOR_COLLECT_H_
#define _SENSOR_COLLECT_H_

struct sensor_file_addr {
    char *mems_file;
    char *gps_file;
    char *mag_file;
};

void* sensor_collect_func(void *arg);


#endif // _SENSOR_COLLECT_H_
