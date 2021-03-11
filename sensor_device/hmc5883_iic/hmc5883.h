#ifndef _HMC5883_H_
#define _HMC5883_H_

#define   RANGE_8_1G_RESOLUTION     0.00457
#define   RANGE_5_6G_RESOLUTION     0.00326
#define   RANGE_4_7G_RESOLUTION     0.00277
#define   RANGE_4G_RESOLUTION       0.00241
#define   RANGE_2_5G_RESOLUTION     0.00163
#define   RANGE_1_9G_RESOLUTION     0.00130
#define   RANGE_1_3G_RESOLUTION     0.00098
#define   RANGE_0_88G_RESOLUTION    0.00078

enum hmc5883_cmd
{
    HMC5883_MAG_RANGE = 0,      /* Mag full scale range */
    HMC5883_MAG_ODR = 1,        /* Output data rate(only in continuous mode) */
    HMC5883_MAG_OPER_MODE = 2,  /* Continuous or single measurement mode */
    HMC5883_MAG_MEAS_MODE = 3   /* number of samples averaged (1 to 8) per
                               measurement output */
};

enum hmc5883_range
{
    HMC5883_RANGE_0_88Ga  = 0, // ±0.88G Gain:1370
    HMC5883_RANGE_1_3Ga   = 1, // ±1.3G  Gain:1090
    HMC5883_RANGE_1_9Ga   = 2, // ±1.9G  Gain:820
    HMC5883_RANGE_2_5Ga   = 3, // ±2.5G  Gain:660
    HMC5883_RANGE_4_0Ga   = 4, // ±4.0G  Gain:440
    HMC5883_RANGE_4_7Ga   = 5, // ±4.7G  Gain:390
    HMC5883_RANGE_5_6Ga   = 6, // ±5.6G  Gain:330
    HMC5883_RANGE_8_1Ga   = 7  // ±8.1G  Gain:230
};

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

enum hmc5883_meas_mode
{
    HMC5883_SAMPLES_AVER_1 = 0, 
    HMC5883_SAMPLES_AVER_2 = 1, 
    HMC5883_SAMPLES_AVER_4 = 2, 
    HMC5883_SAMPLES_AVER_8 = 3
};

enum hmc5883_oper_mode
{
    HMC5883_CONTINUOUS_MEAS = 0, 
    HMC5883_SINGLE_MEAS     = 1
};

/* 3-axis data structure */
struct hmc5883_3axes
{
    int x;
    int y;
    int z;
};

#endif // _HMC5883_H_
