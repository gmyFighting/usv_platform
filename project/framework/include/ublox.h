#ifndef _UBLOX_H
#define _UBLOX_H

struct Vector3d
{
    double x;
    double y;
    double z;
};

struct gps_frame {// 4字节对齐 88字节
    double            lat; // rad
    double            lon; // rad
    double            height; // m
    struct Vector3d   vel; // NED m/s
    char              fixType;// 锁定类型
    char           	  numSV;// 
    short int         pDOP;// 
    double            hAcc; // m
    double            vAcc; // m
    double            sAcc; // m/s
    double	          time_us;	///< timestamp of the measurement (uSec)
};

int ublox_analys(int fd);	

#endif








