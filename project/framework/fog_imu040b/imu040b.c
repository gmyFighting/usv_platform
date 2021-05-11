#include "imu040b.h"

// 帧头(2)+PAYLOAD+CheckSum(2)
static enum prot {
    IMU040_HEADER1 = 0,
    IMU040_HEADER2,
    IMU040_STATE,
    IMU040_PAYLOAD,
    IMU040_IDX,
    IMU040_CHECK_SUM,
    IMU040_TAIL
};

static unsigned char _parse_state = 0;
static unsigned char _ck_sum = 0;
static unsigned int payload_index = 0;

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

static imu040b_nav_buf_t imu040b_nav_buf;
struct imu040_frame imu040_sample = {};

static void _parse_char_reset(void)
{
    _parse_state = IMU040_HEADER1;
    _ck_sum = 0;
    payload_index = 0;
}

static void _checksum(const unsigned char ch)
{
	_ck_sum += ch;
}

void publish_msg()
{
    imu040_sample.state = imu040b_nav_buf.imu040b_data.state;
    imu040_sample.pitch = (float)(imu040b_nav_buf.imu040b_data.pitch)/100.0;
    imu040_sample.roll = (float)(imu040b_nav_buf.imu040b_data.roll)/100.0;
    imu040_sample.yaw = (float)(imu040b_nav_buf.imu040b_data.yaw)/100.0;
    imu040_sample.wx = (float)(imu040b_nav_buf.imu040b_data.wx)/10000.0;
    imu040_sample.wy = (float)(imu040b_nav_buf.imu040b_data.wy)/10000.0;
    imu040_sample.wz = (float)(imu040b_nav_buf.imu040b_data.wz)/10000.0;
    imu040_sample.ax = (float)(imu040b_nav_buf.imu040b_data.ax)/10000.0;
    imu040_sample.ay = (float)(imu040b_nav_buf.imu040b_data.ay)/10000.0;
    imu040_sample.az = (float)(imu040b_nav_buf.imu040b_data.az)/10000.0;    
}

int imu040_parse_char(char ch)
{
    switch(_parse_state) {
    case IMU040_HEADER1:
        if(ch == 0x5A) {
            _parse_state = IMU040_HEADER2;    
        }
    break;
        
    case IMU040_HEADER2:
        if(ch == 0xA5) {
            _parse_state = IMU040_STATE; 
        }
        else {
            // _parse_state = IMU040_HEADER1; 
            // _ck_sum = 0;
            _parse_char_reset();
        }
    break;
        
    case IMU040_STATE:       
        imu040b_nav_buf.buf_raw[payload_index++] = ch;
        _parse_state = IMU040_PAYLOAD;
    break;
        
    case IMU040_PAYLOAD:
        _checksum(ch);
        imu040b_nav_buf.buf_raw[payload_index++] = ch;
        if(payload_index == 33) {
            _parse_state = IMU040_IDX;
        }
	break;   
        
    case IMU040_IDX:
        _parse_state = IMU040_CHECK_SUM;
    break;
    
    case IMU040_CHECK_SUM:
        if (_ck_sum == ch) {
            _parse_state = IMU040_TAIL;
        }
        else {
            _parse_char_reset();
        }
    break;

    case IMU040_TAIL:
        if(ch == 0x55) {
            publish_msg();   
        }

        _parse_char_reset();
    break;
    }
    
    return 0;
}


