#ifndef _HMC5883_H_
#define _HMC5883_H_

struct hmc5883_data
{
    float x;
    float y;
    float z;
};

void hmc5883_get_data(short in[3], struct hmc5883_data *out);

#endif
