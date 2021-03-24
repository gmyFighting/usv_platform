#include "hmc5883.h"

#define   RANGE_8_1G_RESOLUTION     0.00457f
#define   RANGE_5_6G_RESOLUTION     0.00326f
#define   RANGE_4_7G_RESOLUTION     0.00277f
#define   RANGE_4G_RESOLUTION       0.00241f
#define   RANGE_2_5G_RESOLUTION     0.00163f
#define   RANGE_1_9G_RESOLUTION     0.00130f
#define   RANGE_1_3G_RESOLUTION     0.00098f
#define   RANGE_0_88G_RESOLUTION    0.00078f

void hmc5883_get_data(short in[3], struct hmc5883_data *out)
{
    out->x = (float)in[0] * RANGE_1_9G_RESOLUTION;
    out->y = (float)in[1] * RANGE_1_9G_RESOLUTION;
    out->z = (float)in[2] * RANGE_1_9G_RESOLUTION;
}
