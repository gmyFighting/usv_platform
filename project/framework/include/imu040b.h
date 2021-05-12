#ifndef _IMU040B_H_
#define _IMU040B_H_

struct imu040_frame {
    unsigned char state;
    float pitch;
    float roll;
    float yaw;
    float wx;
    float wy;
    float wz;
    float ax;
    float ay;
    float az;
};

int imu040_parse_char(char ch);

#endif
