
typedef struct {
    signed int gyro_x;
    signed int gyro_y;
    signed int gyro_z;
    float acc_x;
    float acc_y;
    float acc_z;
    signed short gyro_tmp_x;
    signed short gyro_tmp_y;
    signed short gyro_tmp_z;
    signed short acc_tmp_x;
    signed short acc_tmp_y;
    signed short acc_tmp_z;
} imu040b_fog_payload_t;

// #pragma pack(push, 1)
// #pragma pack(pop)
// 需要单字节对齐
typedef struct {
    char state;
    short pitch;
    short roll;
    short yaw;
    float wx;
    float wy;
    float wz;
    float ax;
    float ay;
    float az;
    short tmp;
} imu040b_nav_payload_t;

// 用于快速解析协议
typedef union {
    imu040b_fog_payload_t    imu040b_data;
    char                     buf_raw[92];
} imu040b_fog_buf_t;

typedef union {
    imu040b_nav_payload_t    imu040b_data;
    char                     buf_raw[92];
} imu040b_nav_buf_t;
