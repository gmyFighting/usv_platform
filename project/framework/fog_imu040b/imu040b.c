#include "imu040b.h"

/* 自行者 IMU040B
 * 组成: 光纤陀螺(光纤环直径40mm)+mems加速度计
 * 零偏(10s): 0.2deg/h
 * 航向对准精度: 1deg
 * 航向保持精度: deg/h
 * 导航数据口-uartB-921600bps-81Even
 */
#pragma pack(1) // 需要单字节对齐
// #pragma pack(push, 1)
// #pragma pack(pop)
typedef struct {
    char state;// 工作状态：0-监控状态 1-静态对准 2-INS
    short pitch;// ([-90,90] Unit:0.01)
    short roll;// ([-180,180] Unit:0.01)
    short yaw;// ([-180,180] Unit:0.01 左偏正)
    float wx;// (Unit:0.0001deg/s)
    float wy;
    float wz;
    float ax;// (Unit:0.0001m/s/s)
    float ay;
    float az;
    short tmp;
} imu040b_nav_payload_t;// 单字节对齐:33 4字节对齐:36

// 用于快速解析协议
typedef union {
    imu040b_nav_payload_t    imu040b_data;
    char                     buf_raw[33];
} imu040b_nav_buf_t;

#pragma pack()
