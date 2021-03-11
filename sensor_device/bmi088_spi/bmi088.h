#ifndef _BMI088_H_
#define _BMI088_H_

#include <linux/types.h>

#define Grav (9.78033)

/* Wait Time */
#define BMI08X_ACCEL_SOFTRESET_DELAY_MS             1
#define BMI08X_GYRO_SOFTRESET_DELAY_MS              30
#define BMI08X_GYRO_POWER_MODE_CONFIG_DELAY         30
#define BMI08X_POWER_CONFIG_DELAY                   50

/* Accel Power Mode */
#define BMI08X_ACCEL_PM_ACTIVE                      (0x00)
#define BMI08X_ACCEL_PM_SUSPEND                     (0x03)
/* CMD: accel power save */
#define BMI08X_ACCEL_PWR_ACTIVE_CMD                 (0x00)
#define BMI08X_ACCEL_PWR_SUSPEND_CMD                (0x03)
/* CMD: accel power control */ 
#define BMI08X_ACCEL_POWER_DISABLE_CMD              (0x00)
#define BMI08X_ACCEL_POWER_ENABLE_CMD               (0x04)
/* Accel Output Data Rate */
#define BMI08X_ACCEL_ODR_12_5_HZ                    (0x05)
#define BMI08X_ACCEL_ODR_25_HZ                      (0x06)
#define BMI08X_ACCEL_ODR_50_HZ                      (0x07)
#define BMI08X_ACCEL_ODR_100_HZ                     (0x08)
#define BMI08X_ACCEL_ODR_200_HZ                     (0x09)
#define BMI08X_ACCEL_ODR_400_HZ                     (0x0A)
#define BMI08X_ACCEL_ODR_800_HZ                     (0x0B)
#define BMI08X_ACCEL_ODR_1600_HZ                    (0x0C)
/* Accel Range */
#define BMI088_ACCEL_RANGE_3G                       (0x00)
#define BMI088_ACCEL_RANGE_6G                       (0x01)
#define BMI088_ACCEL_RANGE_12G                      (0x02)
#define BMI088_ACCEL_RANGE_24G                      (0x03)
/* Accel Bandwidth */
#define BMI08X_ACCEL_BW_OSR4                        (0x00)
#define BMI08X_ACCEL_BW_OSR2                        (0x01)
#define BMI08X_ACCEL_BW_NORMAL                      (0x02)

/* Gyro Power mode */
#define BMI08X_GYRO_PM_NORMAL                       (0x00)
#define BMI08X_GYRO_PM_DEEP_SUSPEND                 (0x20)
#define BMI08X_GYRO_PM_SUSPEND                      (0x80)
/* Gyro Range */
#define BMI08X_GYRO_RANGE_2000_DPS                  (0x00)
#define BMI08X_GYRO_RANGE_1000_DPS                  (0x01)
#define BMI08X_GYRO_RANGE_500_DPS                   (0x02)
#define BMI08X_GYRO_RANGE_250_DPS                   (0x03)
#define BMI08X_GYRO_RANGE_125_DPS                   (0x04)
/* Gyro Output data rate and bandwidth */
#define BMI08X_GYRO_BW_532_ODR_2000_HZ              (0x00)
#define BMI08X_GYRO_BW_230_ODR_2000_HZ              (0x01)
#define BMI08X_GYRO_BW_116_ODR_1000_HZ              (0x02)
#define BMI08X_GYRO_BW_47_ODR_400_HZ                (0x03)
#define BMI08X_GYRO_BW_23_ODR_200_HZ                (0x04)
#define BMI08X_GYRO_BW_12_ODR_100_HZ                (0x05)
#define BMI08X_GYRO_BW_64_ODR_200_HZ                (0x06)
#define BMI08X_GYRO_BW_32_ODR_100_HZ                (0x07)
#define BMI08X_GYRO_ODR_RESET_VAL                   (0x80)

typedef enum 
{
    ACC_CHIP_ID_REG             = 0x00,
    ACC_ERR_REG                 = 0x02,
    ACC_STATUS_REG              = 0x03,
    ACC_X_LSB_REG               = 0x12,
    ACC_X_MSB_REG               = 0x13,
    ACC_Y_LSB_REG               = 0x14,
    ACC_Y_MSB_REG               = 0x15,
    ACC_Z_LSB_REG               = 0x16,
    ACC_Z_MSB_REG               = 0x17,
    TEMP_MSB_REG                = 0x22,
    TEMP_LSB_REG                = 0x23,
    ACC_CONF_REG                = 0x40,
    ACC_RANGE_REG               = 0x41,
    INT1_IO_CTRL_REG            = 0x53,
    INT2_IO_CTRL_REG            = 0x54,
    ACC_SELF_TEST_REG           = 0x6D,
    ACC_PWR_CONF_REG            = 0x7C,
    ACC_PWR_CTRL_REG            = 0x7D,
    ACC_SOFTRESET_REG           = 0x7E
} bmi088a_reg_list_t;

typedef enum 
{
    GYRO_CHIP_ID_REG            = 0x00,
    RATE_X_LSB_REG              = 0x02,
    RATE_X_MSB_REG              = 0x03,
    RATE_Y_LSB_REG              = 0x04,
    RATE_Y_MSB_REG              = 0x05,
    RATE_Z_LSB_REG              = 0x06,
    RATE_Z_MSB_REG              = 0x07,
    GYRO_INT_STAT_1_REG         = 0x0A,
    GYRO_RANGE_REG              = 0x0F,
    GYRO_BANDWIDTH_REG          = 0x10,
    GYRO_LPM1_REG               = 0x11,
    GYRO_SOFTRESET_REG          = 0x14,
    GYRO_INT_CTRL_REG           = 0x15
} bmi088g_reg_list_t;

enum hmc5883_odr
{
    HMC5883_ODR_0_75Hz   = 0, 
    HMC5883_ODR_1_5Hz    = 1, 
    HMC5883_ODR_3_0Hz    = 2, 
    HMC5883_ODR_7_5Hz    = 3, 
    HMC5883_ODR_15Hz     = 4, 
    HMC5883_ODR_30Hz     = 5, 
    HMC5883_ODR_75Hz     = 6, 
    HMC5883_ODR_Not_Used = 7  
};



enum hmc5883_oper_mode
{
    HMC5883_CONTINUOUS_MEAS = 0, 
    HMC5883_SINGLE_MEAS     = 1
};

struct bmi088_3axes
{
    short x;
    short y;
    short z;
};

struct bmi088_data
{
    float x;
    float y;
    float z;
};

#endif // _HMC5883_H_
