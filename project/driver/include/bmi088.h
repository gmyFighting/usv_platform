#ifndef _BMI088_H_
#define _BMI088_H_

struct bmi088_data
{
    float acc_x;
    float acc_y;
    float acc_z;
    float gyr_x;
    float gyr_y;
    float gyr_z;
};

void bmi088_get_data(short in[6], struct bmi088_data *out);




#endif // _BMI088_H_
