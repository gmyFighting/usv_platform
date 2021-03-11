#include <linux/types.h>
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
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include "hmc5883.h"

#define PLATFORM_CNT    1
#define PLATFORM_NAME   "hmc5883"

struct hmc5883_dev {
    int major;
    int minor;
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    void *private_data;	
    double gain_resolution;
};

static struct hmc5883_dev hmc5883_dev_t;
    /*i2c_transfer传入参数需要i2c_adapter 当设备匹配设备树时，
    会自动执行probe并向probe传入i2c_client,而里面有i2c_adapter*/
static int _hmc5883_read_regs(struct hmc5883_dev *dev, u8 reg, void *val, int len)
{
    // dev中包含器件地址，还需要读寄存器地址，写入值，写入长度
	int res;
	struct i2c_msg msg[2];
    struct i2c_client *client = (struct i2c_client *)dev->private_data;
    int val_prt;

	/* msg[0]为发送要读取的首地址 */
	msg[0].addr = client->addr;			/* hmc5883地址 */
	msg[0].flags = 0;					/* 标记为发送数据 */
	msg[0].buf = &reg;					/* 读取的首地址 */
	msg[0].len = 1;						/* reg长度*/
	/* msg[1]读取数据 */
	msg[1].addr = client->addr;			/* hmc5883地址 */
	msg[1].flags = I2C_M_RD;			/* 标记为读取数据*/
	msg[1].buf = val;					/* 读取数据缓冲区,内核会将寄存器值存入该地址 */
	msg[1].len = len;					/* 要读取的数据长度*/    

	res = i2c_transfer(client->adapter, msg, 2);// 已经在内核空间注册EXPORT_SYMBOL

	if(res == 2) {
        val_prt = *(char *)val;
        //printk("_hmc5883_read_regs addr=0x%x,reg=0x%x,val=0x%x\r\n", msg[1].addr, reg, val_prt);
		res = 0;
	} else {
		 printk("i2c rd failed=%d reg=%06x len=%d\n",res, reg, len);
		res = -EREMOTEIO;
	}

	return res;
}

// 写寄存器时一次性写入
static int _hmc5883_write_regs(struct hmc5883_dev *dev, u8 reg, u8 *val, int len)
{
    // dev中包含器件地址，还需要写寄存器地址，写入值，写入长度
	u8 b[256];
	struct i2c_msg msg;
    // 通过client找adapter
	struct i2c_client *client = (struct i2c_client *)dev->private_data;
	
    // printk("_hmc5883_write_regs\r\n");
	b[0] = reg;					/* 寄存器首地址 */
	memcpy(&b[1],val,len);		/* 将要写入的数据拷贝到数组b里面 */
		
	msg.addr = client->addr;	/* hmc5883地址 */
	msg.flags = 0;				/* 标记为写数据 */

	msg.buf = b;				/* 要写入的数据缓冲区 */
	msg.len = len + 1;			/* 要写入的数据长度 */

	return i2c_transfer(client->adapter, &msg, 1);
}

int _hmc5883_set_param(struct hmc5883_dev *dev, enum hmc5883_cmd cmd, u8 param)
{
    int res = 0;
    u8 val = 0;

    // printk("_hmc5883_set_param\r\n");
    switch (cmd)
    {
    case HMC5883_MAG_RANGE:
        switch (param) 
        {
        case HMC5883_RANGE_0_88Ga:
            val = 0x0;
            res = _hmc5883_write_regs(dev, 0x01, &val, 1);
            dev->gain_resolution = RANGE_0_88G_RESOLUTION;
            break;
        case HMC5883_RANGE_1_3Ga:
            val = 0x20;
            res = _hmc5883_write_regs(dev, 0x01, &val, 1);
            dev->gain_resolution = RANGE_1_3G_RESOLUTION;
            break;
        case HMC5883_RANGE_1_9Ga:
            val = 0x40;
            res = _hmc5883_write_regs(dev, 0x01, &val, 1);
            dev->gain_resolution = RANGE_1_9G_RESOLUTION;
            break;
        case HMC5883_RANGE_2_5Ga:
            val = 0x60;
            res = _hmc5883_write_regs(dev, 0x01, &val, 1);
            dev->gain_resolution = RANGE_2_5G_RESOLUTION;
            break;
        case HMC5883_RANGE_4_0Ga:
            val = 0x80;
            res = _hmc5883_write_regs(dev, 0x01, &val, 1);
            dev->gain_resolution = RANGE_4G_RESOLUTION;
            break;
        case HMC5883_RANGE_4_7Ga:
            val = 0xA0;
            res = _hmc5883_write_regs(dev, 0x01, &val, 1);
            dev->gain_resolution = RANGE_4_7G_RESOLUTION;
            break;
        case HMC5883_RANGE_5_6Ga:
            val = 0xC0;
            res = _hmc5883_write_regs(dev, 0x01, &val, 1);
            dev->gain_resolution = RANGE_5_6G_RESOLUTION;
            break;
        case HMC5883_RANGE_8_1Ga:
            val = 0xE0;
            res = _hmc5883_write_regs(dev, 0x01, &val, 1);
            dev->gain_resolution = RANGE_8_1G_RESOLUTION;
            break;
        }    
        break;
    case HMC5883_MAG_ODR: // 临时
        val = param;
        res = _hmc5883_write_regs(dev, 0x00, &val, 1);
        break;
    case HMC5883_MAG_OPER_MODE: // 临时
        val = param;
        res = _hmc5883_write_regs(dev, 0x02, &val, 1);
        break;
    case HMC5883_MAG_MEAS_MODE: // 临时
        res = _hmc5883_write_regs(dev, 0x00, &param, 1);
        break;
    }

    return res;
}

int hmc5883_open(struct inode *inode, struct file *filp)
{
    u8 val[3] = {};
    filp->private_data = &hmc5883_dev_t;// read write用

    // test
    // _hmc5883_test_self

    // config sensor
    _hmc5883_set_param(&hmc5883_dev_t, HMC5883_MAG_ODR, 0x78);// 0x00：samples averaged 8 per output;75Hz(连续模式)
    _hmc5883_set_param(&hmc5883_dev_t, HMC5883_MAG_RANGE, HMC5883_RANGE_1_9Ga);// 0x01
    // _hmc5883_set_param(&hmc5883_dev_t, HMC5883_MAG_OPER_MODE, 0x01);// 0x02:single meas mode

    _hmc5883_read_regs(&hmc5883_dev_t, 0x00, &val[0], 1);
    _hmc5883_read_regs(&hmc5883_dev_t, 0x01, &val[1], 1);
    _hmc5883_read_regs(&hmc5883_dev_t, 0x02, &val[2], 1);

    printk("reg=0x00, val=0x%x", val[0]);
    printk("reg=0x01, val=0x%x", val[1]);
    printk("reg=0x02, val=0x%x", val[2]);

    return 0;
}

static int _hmc5883_get_mag_raw(struct hmc5883_dev *dev, struct hmc5883_3axes *mag)
{
    u8 buffer[6];
    int res;
    u8 val;

    val = 0x01;
    _hmc5883_write_regs(dev, 0x02, &val, 1);// 每次设置成single meas mod 数值才会变化
    res = _hmc5883_read_regs(dev, 0x03, buffer, 6);
    if (res != 0)
    {
        printk("func(_hmc5883_read_regs) failed\r\n");
        return res;
    }

    mag->x = (s16)(buffer[1] | ((s16)buffer[0] << 8));
    mag->y = (s16)(buffer[5] | ((s16)buffer[4] << 8));
    mag->z = (s16)(buffer[3] | ((s16)buffer[2] << 8));
    printk("mag_x=%d,mag_y=%d,mag_z=%d\r\n", mag->x, mag->y, mag->z);
    return res;
}

struct hmc5883_3axes mag_test;
ssize_t hmc5883_read(struct file *filp, char __user *buf, size_t size, loff_t *off)
{
	unsigned long res = 0;
	struct hmc5883_dev *dev = (struct hmc5883_dev *)filp->private_data;
	//_hmc5883_write_regs(dev, 0x02, 0x01, 1);
	_hmc5883_get_mag_raw(dev, &mag_test);
    // printk("mag_data:x=%d,y=%d,z=%d\r\n", mag_test.x, mag_test.y, mag_test.z);

	// data[0] = dev->ir;
	// data[1] = dev->als;
	// data[2] = dev->ps;
	res = copy_to_user(buf, &mag_test, sizeof(mag_test));
	return 0;
}

int hmc5883_release(struct inode *inode, struct file *filp)
{
//    struct hmc5883_dev *dev = (struct hmc5883_dev *)filp->private_data;
    printk("hmc5883_release\r\n");
    return 0;
}

static const struct file_operations hmc5883_ops = {
	.owner = THIS_MODULE,
	.open = hmc5883_open,
	.read = hmc5883_read,
	.release = hmc5883_release,
};

int hmc5883_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int res = 0;
    printk("hmc5883_probe\r\n");
	/* 1、向内核申请设备号 */
	if (hmc5883_dev_t.major) {
		hmc5883_dev_t.devid = MKDEV(hmc5883_dev_t.major, 0);
		res = register_chrdev_region(hmc5883_dev_t.devid, PLATFORM_CNT, PLATFORM_NAME);
	} else {
		res = alloc_chrdev_region(&hmc5883_dev_t.devid, 0, PLATFORM_CNT, PLATFORM_NAME);
		hmc5883_dev_t.major = MAJOR(hmc5883_dev_t.devid);
        hmc5883_dev_t.minor = MINOR(hmc5883_dev_t.devid);
	}
    if (res < 0) {
        printk("alloc chrdev_region err!\n");
        unregister_chrdev_region(hmc5883_dev_t.devid, PLATFORM_CNT);
        return res;
    }
    printk("hmc5883 major=%d, minor=%d\r\n", hmc5883_dev_t.major, hmc5883_dev_t.minor);
	
    /* 2、注册设备 */
    hmc5883_dev_t.cdev.owner = THIS_MODULE;
	cdev_init(&hmc5883_dev_t.cdev, &hmc5883_ops);
	res = cdev_add(&hmc5883_dev_t.cdev, hmc5883_dev_t.devid, PLATFORM_CNT);
    if (res < 0) {
        printk("cdev_add fail!\n");
        cdev_del(&hmc5883_dev_t.cdev);
        return res;
    }

	/* 3、创建类 */
	hmc5883_dev_t.class = class_create(THIS_MODULE, PLATFORM_NAME);
	if (IS_ERR(hmc5883_dev_t.class)) {
		return PTR_ERR(hmc5883_dev_t.class);
	}

	/* 4、创建设备 */
	hmc5883_dev_t.device = device_create(hmc5883_dev_t.class, NULL, hmc5883_dev_t.devid, NULL, PLATFORM_NAME);
	if (IS_ERR(hmc5883_dev_t.device)) {
        class_destroy(hmc5883_dev_t.class);
		return PTR_ERR(hmc5883_dev_t.device);
	}

	hmc5883_dev_t.private_data = client;

    return 0;
}

int hmc5883_remove(struct i2c_client *client)
{
    printk("hmc5883_remove\r\n");
	/* 删除设备 */
	cdev_del(&hmc5883_dev_t.cdev);
	unregister_chrdev_region(hmc5883_dev_t.devid, PLATFORM_CNT);

	/* 注销掉类和设备 */
	device_destroy(hmc5883_dev_t.class, hmc5883_dev_t.devid);
	class_destroy(hmc5883_dev_t.class);
    return 0;
}

/* 设备树匹配列表 */
static const struct of_device_id hmc5883_of_match[] = {
	{ .compatible = "honeywell,hmc5883" },
	{ /* Sentinel */ }
};

/* i2c驱动结构体 */	
static struct i2c_driver hmc5883_driver = {
	.probe = hmc5883_probe,
	.remove = hmc5883_remove,
	.driver = {
			.owner = THIS_MODULE,
		   	.name = "hmc5883",
		   	.of_match_table = hmc5883_of_match, 
	},
};

static int __init hmc5883_init(void)
{
    int res = 0;
	res = i2c_add_driver(&hmc5883_driver);// 里面会调用device_register
	return res;
}

static void __exit hmc5883_exit(void)
{
    i2c_del_driver(&hmc5883_driver);
}

module_init(hmc5883_init);
module_exit(hmc5883_exit);

MODULE_LICENSE("GPL");

