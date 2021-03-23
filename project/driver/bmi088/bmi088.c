#include "bmi088.h"
#include "nav_param.h"

int bmi088_get_data(short in[6], struct bmi088_data *out)
{
    out->acc_x = ((float)in[0]) /32768.0f * 6 * GRAV;
    out->acc_y = ((float)in[1]) /32768.0f * 6 * GRAV;
    out->acc_z = ((float)in[2]) /32768.0f * 6 * GRAV;  
    out->gyr_x = ((float)in[3]) /32768.0f * 6 * GRAV;
    out->gyr_y = ((float)in[4]) /32768.0f * 6 * GRAV;
    out->gyr_z = ((float)in[5]) /32768.0f * 6 * GRAV;  
}



