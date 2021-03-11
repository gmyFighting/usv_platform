#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/spi/spi.h>
#include "bmi088.h"

#define SENSOR_CNT    1
#define SENSOR_NAME   "bmi088"

struct bmi088_dev {
    int major;
    int minor;
    dev_t devid;// 由主次设备号生成
    struct cdev cdev;
    struct class *class;
    struct device *device;
    void *private_data;	
    struct device_node *nd;
    int cs_gpio[2];// 1:acc 2:gyr
};

static struct bmi088_dev bmi088_dev_t;

/*--------------------芯片说明----------------------------
gyro part:
    由PS引脚决定I2C or SPI模式;
    上电后直接进入正常模式;
    bit 0:读写位,0写,1读
acc part:
    自动I2C模式,CSB1出现上升沿后改变为SPI模式,改变后需进行一次dummy read;
    上电后进入休眠模式,需要1.wait 1ms 2.向0x7D写入0x04 3. wait 50ms;
    bit 0:读写位,0写,1读
    读操作的第一个字节都为dummy(unpredictable)
*/
static int _bmi088_read_regs(struct bmi088_dev *dev, u8 reg, void *buf, int len, u8 type)
{
    int ret;
	unsigned char txdata[len];
	struct spi_message msg;
	struct spi_transfer *t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);// 申请内存
	struct spi_device *spi = (struct spi_device *)dev->private_data;// spi device里面都有啥?

    // 临时，以后换成字符串
    if(type == 1) {// acc
        gpio_set_value(dev->cs_gpio[0], 0);	
    }
    else if(type == 2) {// gyr
        gpio_set_value(dev->cs_gpio[1], 0);	
    }
				
	/* 第1次，发送要读取的寄存地址 */
	txdata[0] = reg | 0x80;		/* 写数据的时候寄存器地址bit8要置1 */
	t->tx_buf = txdata;			/* 要发送的数据 */
	t->len = 1;					/* 1个字节 */
	spi_message_init(&msg);		/* 初始化spi_message */
	spi_message_add_tail(t, &msg);/* 将spi_transfer添加到spi_message队列 */
	ret = spi_sync(spi, &msg);	/* 同步发送 */

	/* 第2次，读取数据 */
	txdata[0] = 0xff;			/* 随便一个值，此处无意义 */
	t->rx_buf = buf;			/* 读取到的数据 */
    if(type == 1) {
        t->len = len+1;				/* 要读取的数据长度,第一个值为无效值 */
    }
    else {
        t->len = len;
    }
	
	spi_message_init(&msg);		/* 初始化spi_message */
	spi_message_add_tail(t, &msg);/* 将spi_transfer添加到spi_message队列 */
	ret = spi_sync(spi, &msg);	/* 同步发送 */

	kfree(t);									/* 释放内存 */
    if(type == 1) {
        gpio_set_value(dev->cs_gpio[0], 1);	
    }
    else if(type == 2) {
        gpio_set_value(dev->cs_gpio[1], 1);	
    }

    return ret;
}

static int _bmi088_write_regs(struct bmi088_dev *dev, u8 reg, u8 *buf, u8 len, u8 type)
{
    int ret;
	unsigned char txdata[len];
	struct spi_message msg;
	struct spi_transfer *t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
	struct spi_device *spi = (struct spi_device *)dev->private_data;

    if(type == 1) {
        gpio_set_value(dev->cs_gpio[0], 0);	
    }
    else if(type == 2) {
        gpio_set_value(dev->cs_gpio[1], 0);	
    }

	/* 第1次，发送要读取的寄存地址 */
	txdata[0] = reg & 0x7f;	/* 写数据的时候寄存器地址bit8要清零 */
	t->tx_buf = txdata;			/* 要发送的数据 */
	t->len = 1;					/* 1个字节 */
	spi_message_init(&msg);		/* 初始化spi_message */
	spi_message_add_tail(t, &msg);/* 将spi_transfer添加到spi_message队列 */
	ret = spi_sync(spi, &msg);	/* 同步发送 */

	/* 第2次，发送要写入的数据 */
	t->tx_buf = buf;			/* 要写入的数据 */
	t->len = len;				/* 写入的字节数 */
	spi_message_init(&msg);		/* 初始化spi_message */
	spi_message_add_tail(t, &msg);/* 将spi_transfer添加到spi_message队列 */
	ret = spi_sync(spi, &msg);	/* 同步发送 */

	kfree(t);					/* 释放内存 */

    if(type == 1) {
        gpio_set_value(dev->cs_gpio[0], 1);	
    }
    else if(type == 2) {
        gpio_set_value(dev->cs_gpio[1], 1);	
    }

    return ret;
}

static int _bmi088a_init(struct bmi088_dev *dev_t)
{
	int ret = 0;
    u8 buf_tx;
    u8 chip_acc_id[2] = {0};
    // 上升沿进入SPI模式
    gpio_set_value(dev_t->cs_gpio[0], 0);
    mdelay(1);
    gpio_set_value(dev_t->cs_gpio[0], 1);

    ret = _bmi088_read_regs(dev_t, ACC_CHIP_ID_REG, chip_acc_id, 1, 1);    
    if(ret) {
        printk("read acc failed!\n");
        goto error;
    }
    if (chip_acc_id[1] != 0x1E) 
    {
        printk("Fail initialize acc!\n");
        goto error;        
    }

    mdelay(10);
    // reset 
    buf_tx = 0xB6;
    if (_bmi088_write_regs(dev_t, ACC_SOFTRESET_REG, &buf_tx, 1, 1) != 0)
    {
        printk("Fail reset acc!\n");
        goto error;
    }
    mdelay(BMI08X_ACCEL_SOFTRESET_DELAY_MS);
    // 上升沿进入SPI模式
    gpio_set_value(dev_t->cs_gpio[0], 0);
    mdelay(1);
    gpio_set_value(dev_t->cs_gpio[0], 1);
    
    // test
    printk("acc init succeed\n");
    return ret;

error:
    return -1;    
}

static int _bmi088g_init(struct bmi088_dev *dev_t)
{
    u8 buf_tx;
    u8 chip_gyr_id = 0;
    if (_bmi088_read_regs(dev_t, GYRO_CHIP_ID_REG, &chip_gyr_id, 1, 2) != 0)
    {
        printk("Fail reset gyr!\n");
        goto error;
    }

    if (chip_gyr_id != 0xF) 
    {
        printk("Fail initialize gyr!\n");
        goto error;
    }
    buf_tx = 0xB6;
    if (_bmi088_write_regs(dev_t, GYRO_SOFTRESET_REG, &buf_tx, 1, 2) != 0)
    {
        printk("Fail reset gyr!\n");
        goto error;
    }
    mdelay(BMI08X_GYRO_SOFTRESET_DELAY_MS);
    // test 
    printk("gyr init succeed\n");
    return 0;
    
error:
    return -1;
}

static int _bmi088a_set_power_mode(struct bmi088_dev *dev_t, u8 power_mode)
{
    u8 data[2];
    u8 buf_rx;
    
    if (power_mode == BMI08X_ACCEL_PM_ACTIVE) 
    {
        data[0] = BMI08X_ACCEL_PWR_ACTIVE_CMD;// 0x00
        data[1] = BMI08X_ACCEL_POWER_ENABLE_CMD;// 0x04
    } 
    else if (power_mode == BMI08X_ACCEL_PM_SUSPEND) 
    {
        data[0] = BMI08X_ACCEL_PWR_SUSPEND_CMD;
        data[1] = BMI08X_ACCEL_POWER_DISABLE_CMD;
    } 
    else 
    {
        printk("Invalid acc power mode!\n");
        goto error;          
    }

    if (_bmi088_write_regs(dev_t, ACC_PWR_CONF_REG, &data[0], 1, 1) == 0)
    {
        mdelay(BMI08X_POWER_CONFIG_DELAY);
        if (_bmi088_write_regs(dev_t, ACC_PWR_CTRL_REG, &data[1], 1, 1) == 0)
        {
            mdelay(BMI08X_POWER_CONFIG_DELAY);
            return 0;
        }
        else
        {
            printk("Failed write ACC_PWR_CTRL_REG\n");
            goto error;
        }
    }
    else
    {
        printk("Failed write ACC_PWR_CONF_REG\n");
        goto error;
    }

    _bmi088_read_regs(dev_t, ACC_PWR_CONF_REG, &buf_rx, 1, 1);
    if (buf_rx != 0x0) {
        printk("ACC_PWR_CONF_REG != 0x00\n");
        goto error;
    }

    _bmi088_read_regs(dev_t, ACC_PWR_CTRL_REG, &buf_rx, 1, 1);
    if (buf_rx != 0x4) {
        printk("ACC_PWR_CTRL_REG != 0x04\n");
        goto error;
    }
    
error:
    return -1;
}

static int _bmi088g_set_power_mode(struct bmi088_dev *dev_t, u8 power_mode)
{
    u8 buf_rx;
	u8 is_power_switching_mode_valid = 1;
    
    _bmi088_read_regs(dev_t, GYRO_LPM1_REG, &buf_rx, 1, 2);
    if (power_mode == buf_rx) 
    {
        return 0;
    }
    else 
    {
        // only switching between normal mode and the suspend mode is allowed
        if ((power_mode == BMI08X_GYRO_PM_SUSPEND) && (buf_rx == BMI08X_GYRO_PM_DEEP_SUSPEND)) 
        {
            is_power_switching_mode_valid = 0;
        }  
        if ((power_mode == BMI08X_GYRO_PM_DEEP_SUSPEND) && (buf_rx == BMI08X_GYRO_PM_SUSPEND))
        {
            is_power_switching_mode_valid = 0;
        }
        
        if (is_power_switching_mode_valid) 
        {
            if (_bmi088_write_regs(dev_t, GYRO_LPM1_REG, &power_mode, 1, 2) == 0)
            {
                mdelay(BMI08X_GYRO_POWER_MODE_CONFIG_DELAY);
                _bmi088_read_regs(dev_t, GYRO_LPM1_REG, &buf_rx, 1, 2);
                if (buf_rx != 0x00)
                {
                    printk("GYRO_LPM1_REG != 0x00\n");
                }
            }
        }
        else
        {
            printk("Invalid gyro mode switch\n");
            goto error;        
        }
    }
    
error:
    return -1;   
}

static int _bmi088a_set_meas_conf(struct bmi088_dev *dev_t)// 输入参数待改进,配置完要记录到bmi088结构体中
{
    u8 data[2] = {0};
    u8 reg_val[3] = {0};
    
    if (_bmi088_read_regs(dev_t, ACC_CONF_REG, data, 2, 1) == 0)// dummy read
    {
        data[0] = (0xA<<4) | (0xB<<0);// bandwidth of lpf = normal, odr = 800是否设置过采样
        _bmi088_write_regs(dev_t, ACC_CONF_REG, &data[0], 1, 1);
            
        data[1] = 0x01;// range = 6G
        _bmi088_write_regs(dev_t, ACC_RANGE_REG, &data[1], 1, 1);
            
        mdelay(10);
        _bmi088_read_regs(dev_t, ACC_CONF_REG, reg_val, 2, 1);// dummy read
        if ((reg_val[1] == 0xAB) && (reg_val[2] == 0x01)) 
        {
            // dbg
            printk("acc_conf_reg=0x%x, acc_range_reg=0x%x\n", reg_val[1], reg_val[2]);
            return 0;
        }
    }
        
    return -1;
}

static int _bmi088g_set_meas_conf(struct bmi088_dev *dev_t)
{
    u8 data;
    u8 reg_val[2] = {0};

    data = 0x01;// ODR = 2000Hz, Filter bandwidth = 230Hz
    if (_bmi088_write_regs(dev_t, GYRO_BANDWIDTH_REG, &data, 1, 2) == 0)
    {
        data = 0x00;// range = 2000deg/s
        if (_bmi088_write_regs(dev_t, GYRO_RANGE_REG, &data, 1, 2) == 0) 
        {
            mdelay(10);
            _bmi088_read_regs(dev_t, GYRO_RANGE_REG, reg_val, 2, 2);
            if ((reg_val[0] == 0x00) && (reg_val[1] == 0x81))// read only reg:7 bit always 1
            {
                // dbg
                printk("gyr_range_reg=0x%x, gyr_bw_reg=0x%x\n", reg_val[0], reg_val[1]);
                return 0;
            }                
        }                
    }
    return -1;    
}

static int _bmi088_get_accel(struct bmi088_dev *dev_t, struct bmi088_3axes *tmp)
{ 
    // accel_x_in_mg = accel_x_int16/32768*1000*2^(<0x41> + 1)*1.5  
    u8 buffer[7];
    u8 lsb, msb;

    if (_bmi088_read_regs(dev_t, ACC_X_LSB_REG, buffer, 6, 1) != 0) 
    {
        return -1;
    }

    lsb = buffer[1];
    msb = buffer[2];
    tmp->x = (short)((msb << 8) | lsb); /* X */
    lsb = buffer[3];
    msb = buffer[4];
    tmp->y = (short)((msb << 8) | lsb);/* Y */
    lsb = buffer[5];
    msb = buffer[6];
    tmp->z = (short)((msb << 8) | lsb);/* Z */

    // buf->x = ((float)tmp.x) /32768.0 * 6 * Grav;
    // buf->y = ((float)tmp.y) /32768.0 * 6 * Grav;
    // buf->z = ((float)tmp.z) /32768.0 * 6 * Grav;  
    return 0;  
}

static int _bmi088_get_gyro(struct bmi088_dev *dev_t, struct bmi088_3axes *tmp)
{ 
    u8 buffer[6];
    u8 lsb, msb;

    if (_bmi088_read_regs(dev_t, RATE_X_LSB_REG, buffer, 6, 2) != 0) 
    {
        return -1;
    }

    lsb = buffer[0];
    msb = buffer[1];
    tmp->x = (short)((msb << 8) | lsb); /* X */
    lsb = buffer[2];
    msb = buffer[3];
    tmp->y = (short)((msb << 8) | lsb);/* Y */
    lsb = buffer[4];
    msb = buffer[5];
    tmp->z = (short)((msb << 8) | lsb);/* Z */
 
    return 0;  
}

int bmi088_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &bmi088_dev_t;// 用于read和release
    printk("bmi088_open\n");
    return 0;
}

static ssize_t bmi088_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
    int err;
    //struct bmi088_data data;
    short data[6] = {0};
    struct bmi088_3axes acc_data = {};
    struct bmi088_3axes gyr_data = {};
    struct bmi088_dev *dev_t = (struct bmi088_dev *)filp->private_data;
    _bmi088_get_accel(dev_t, &acc_data);
    _bmi088_get_gyro(dev_t, &gyr_data);
    
    data[0] = acc_data.x;
    data[1] = acc_data.y;
    data[2] = acc_data.z;  
    data[3] = gyr_data.x;
    data[4] = gyr_data.y;
    data[5] = gyr_data.z;
    err = copy_to_user(buf, &data, sizeof(data));//sizeof(数组名)?
    
    return 0;
}

static int bmi088_release(struct inode *inode, struct file *filp)
{
    printk("bmi088_release\n");
    return 0;
}

/* bmi088操作函数 */
static const struct file_operations bmi088_ops = {
	.owner = THIS_MODULE,
	.open = bmi088_open,
	.read = bmi088_read,
	.release = bmi088_release,
};
struct bmi088_3axes test;
static int bmi088_spi_probe(struct spi_device *spi)
{
    int ret = 0;

    // 1.申请设备号
    if (bmi088_dev_t.major) {// 手动生成设备号
        bmi088_dev_t.devid = MKDEV(bmi088_dev_t.major, 0);
        ret = register_chrdev_region(bmi088_dev_t.devid, SENSOR_CNT, SENSOR_NAME);
    }
    else {// 未指定设备号
        // dev_t *, unsigned, unsigned, const char *
        ret = alloc_chrdev_region(&bmi088_dev_t.devid, 0, SENSOR_CNT, SENSOR_NAME);
        bmi088_dev_t.major = MAJOR(bmi088_dev_t.devid);
        bmi088_dev_t.minor = MINOR(bmi088_dev_t.devid);
    }

    if (ret < 0) {
        printk("alloc chrdev_region err!\n");
        unregister_chrdev_region(bmi088_dev_t.devid, SENSOR_CNT);
        return ret;
    }
    else {
        printk( KERN_ALERT "bmi088 major=%d, minor=%d\r\n", bmi088_dev_t.major, bmi088_dev_t.minor);
    }

    // 2.注册设备
    bmi088_dev_t.cdev.owner = THIS_MODULE;
	cdev_init(&bmi088_dev_t.cdev, &bmi088_ops);
	ret = cdev_add(&bmi088_dev_t.cdev, bmi088_dev_t.devid, SENSOR_CNT);
    if (ret < 0) {
        printk("cdev_add fail!\n");
        cdev_del(&bmi088_dev_t.cdev);
        return ret;
    }

    // 3.创建类
    bmi088_dev_t.class = class_create(THIS_MODULE, SENSOR_NAME);
    if(IS_ERR(bmi088_dev_t.class)) {
        return PTR_ERR(bmi088_dev_t.class);
    }

    // 4.创建设备
    // struct device *device_create(struct class *cls, struct device *parent,
	// 		     dev_t devt, void *drvdata,
	// 		     const char *fmt, ...);
    bmi088_dev_t.device = device_create(bmi088_dev_t.class, NULL, bmi088_dev_t.devid, NULL, SENSOR_NAME);
	if(IS_ERR(bmi088_dev_t.device)) {
        class_destroy(bmi088_dev_t.class);
		return PTR_ERR(bmi088_dev_t.device);
	}

    // 5.获取自定义片选信号
    // spi->dev.of_node//节点spidev:bmi088@0
    // bmi088_dev_t.nd = of_get_parent(spi->dev.of_node);//获取ecspi1也可以
	bmi088_dev_t.nd = of_find_node_by_path("/soc/aips-bus@2000000/spba-bus@2000000/ecspi@2008000");
	if(bmi088_dev_t.nd == NULL) {
		printk("ecspi1 node not find!\r\n");
		return -EINVAL;
	} 
    else {
        printk( KERN_ALERT "get bmi088 ecspi1 node\n");
    }
	// 获取设备树中的gpio属性:MX6UL_PAD_CSI_DATA00__GPIO4_IO21,MX6UL_PAD_CSI_DATA01__GPIO4_IO22
	bmi088_dev_t.cs_gpio[0] = of_get_named_gpio(bmi088_dev_t.nd, "cs-gpio-0", 0);// index=0
    bmi088_dev_t.cs_gpio[1] = of_get_named_gpio(bmi088_dev_t.nd, "cs-gpio-1", 0);
	if(bmi088_dev_t.cs_gpio[0] < 0 || bmi088_dev_t.cs_gpio[1] < 0) {
		printk("can't get cs-gpio");
		return -EINVAL;
	}
    else {
        printk( KERN_ALERT "get cs0=%d & cs1=%d\n", bmi088_dev_t.cs_gpio[0], bmi088_dev_t.cs_gpio[1]);
    }
	/* 设置CS引脚为输出，并且输出高电平,使用时再拉低 */
	ret = gpio_direction_output(bmi088_dev_t.cs_gpio[0], 1);
    if(ret < 0) {
		printk("can't set gpio!\r\n");
	}
    ret = gpio_direction_output(bmi088_dev_t.cs_gpio[1], 1);
	if(ret < 0) {
		printk("can't set gpio!\r\n");
	}

    // 初始化spi_device结构体
	spi->mode = SPI_MODE_0;	/*MODE0，CPOL=0，CPHA=0,默认MSB first*/
	spi_setup(spi);
	bmi088_dev_t.private_data = spi; /* 设置私有数据 */

    // 初始化acc gyr器件
    _bmi088a_init(&bmi088_dev_t);// 加速度计初始化完是在suspend模式
    _bmi088a_set_power_mode(&bmi088_dev_t, BMI08X_ACCEL_PM_ACTIVE);//suspend-->active
    _bmi088a_set_meas_conf(&bmi088_dev_t);
    _bmi088g_init(&bmi088_dev_t);
    _bmi088g_set_power_mode(&bmi088_dev_t, BMI08X_GYRO_PM_NORMAL);
    _bmi088g_set_meas_conf(&bmi088_dev_t);

	return ret;
}

static int bmi088_spi_remove(struct spi_device *spi)
{
    cdev_del(&bmi088_dev_t.cdev);
    unregister_chrdev_region(bmi088_dev_t.devid, SENSOR_CNT);
    device_destroy(bmi088_dev_t.class, bmi088_dev_t.devid);
    class_destroy(bmi088_dev_t.class);
    printk("bmi088_spi_remove\r\n");
    return 0;
}

/* 设备树匹配列表 */
static const struct of_device_id bmi088_of_match[] = {
	{ .compatible = "bosch,bmi088" },
	{ /* Sentinel */ }
};

/* spi驱动结构体 */	
static struct spi_driver bmi088_spi_driver = {
	.probe = bmi088_spi_probe,
	.remove = bmi088_spi_remove,
	.driver = {
			.owner = THIS_MODULE,
		   	.name = SENSOR_NAME,
		   	.of_match_table = bmi088_of_match, 
	},
};

static int __init bmi088_spi_init(void)
{
    int ret = 0;
	ret = spi_register_driver(&bmi088_spi_driver);// 里面会调用device_register
	if (ret != 0) {
		printk("Failed to register BMI088 SPI driver, %d\n", ret);
		return ret;
	}
    return 0;
}

static void __exit bmi088_spi_exit(void)
{
    spi_unregister_driver(&bmi088_spi_driver);

}

module_init(bmi088_spi_init);
module_exit(bmi088_spi_exit);

MODULE_LICENSE("GPL");

