/*
 * File      : uMCN.c
 *
 * 函数1：订阅话题信息 初始化过程？（自动初始化：通过遍历topic_list/手动初始化:添加deu_subscribe）放在main函数中?
 * 函数2：写入循环缓存 循环
 * 函数3：定时将ringbuffer中数据写入sd
 * Change Logs:
 * Date           Author       	Notes
 * 2017-10-16     zoujiachi   	the first version
 */

//#include <atomic.h>//atomic
#include <stdlib.h>// malloc
#include <pthread.h>
#include <stdio.h>// printf
#include <string.h>// strcmp
#include <assert.h>// assert
#include <sys/sem.h>//sem
#include <unistd.h>//sleep
#include "uDEU.h"
#include "ringbuffer.h" //ringbuf
#include "loop_logger.h"
//#include "topic_advertise.h"//test
#include "bmi088.h"
#include "hmc5883.h"
#include "ublox.h"

enum {
	LOG_INT8 = 0,
	LOG_UINT8,
	LOG_INT16,
	LOG_UINT16,
	LOG_INT32,
	LOG_UINT32,
	LOG_FLOAT,
	LOG_DOUBLE,
	LOG_BOOLEAN,
};

static struct bmi088_data* imu_sample_t;
static struct hmc5883_data* mag_sample_t;
static struct gps_frame* gps_sample_t;


#define LOG_ELEMENT(_name, _type) { \
	#_name, \
		_type, \
		1 \
}
	
#define LOG_ELEMENT_ARRAY(_name, _type, _num) { \
	#_name, \
		_type, \
		_num \
}

#define LOG_FIELD(_name, _id, _elem_list) { \
	#_name, \
	_id, \
	sizeof(_elem_list)/sizeof(log_elem_t), \
    _elem_list \
}

log_elem_t _IMU_elem[] = {
	LOG_ELEMENT_ARRAY("gyr_radPs_B", LOG_FLOAT, 3),
	LOG_ELEMENT_ARRAY("acc_mPs2_B", LOG_FLOAT, 3),
	LOG_ELEMENT("timestamp_ms", LOG_UINT32)
};

log_elem_t _MAG_elem[] = {
	LOG_ELEMENT_ARRAY("mag_ga_B", LOG_FLOAT, 3),
	LOG_ELEMENT("timestamp_ms", LOG_UINT32)
};

log_elem_t _GPS_uBlox_elem[] = {
	LOG_ELEMENT("iTOW", LOG_UINT32),
	LOG_ELEMENT("year", LOG_UINT16),
	LOG_ELEMENT("month", LOG_UINT8),
	LOG_ELEMENT("day", LOG_UINT8),
	LOG_ELEMENT("hour", LOG_UINT8),
	LOG_ELEMENT("min", LOG_UINT8),
	LOG_ELEMENT("sec", LOG_UINT8),
	LOG_ELEMENT("valid", LOG_UINT8),
	LOG_ELEMENT("tAcc", LOG_UINT32),
	LOG_ELEMENT("nano", LOG_INT32),
	LOG_ELEMENT("fixType", LOG_UINT8),
	LOG_ELEMENT("flags", LOG_UINT8),
	LOG_ELEMENT("reserved1", LOG_UINT8),
	LOG_ELEMENT("numSV", LOG_UINT8),
	LOG_ELEMENT("lon", LOG_INT32),
	LOG_ELEMENT("lat", LOG_INT32),
	LOG_ELEMENT("height", LOG_INT32),
	LOG_ELEMENT("hMSL", LOG_INT32),
	LOG_ELEMENT("hAcc", LOG_UINT32),
	LOG_ELEMENT("vAcc", LOG_UINT32),
	LOG_ELEMENT("velN", LOG_INT32),
	LOG_ELEMENT("velE", LOG_INT32),
	LOG_ELEMENT("velD", LOG_INT32),
	LOG_ELEMENT("gSpeed", LOG_INT32),
	LOG_ELEMENT("headMot", LOG_INT32),
	LOG_ELEMENT("sAcc", LOG_UINT32),
	LOG_ELEMENT("headAcc", LOG_UINT32),
	LOG_ELEMENT("pDOP", LOG_UINT16),
	LOG_ELEMENT("reserved2", LOG_UINT16),
	LOG_ELEMENT("timestamp_ms", LOG_UINT32)
};


log_field_t _log_field_list[] = {
	LOG_FIELD("IMU", 1, _IMU_elem),
	LOG_FIELD("MAG", 2, _MAG_elem),
	LOG_FIELD("GPS_uBlox", 3, _GPS_uBlox_elem),
};

/* 文件信息 */
typedef struct
{
	unsigned int total_msg;
	unsigned int lost_msg;
} log_filed_status;

struct log_status_t
{
	unsigned char 			file_name[20];
	unsigned char 			is_logging;
	char 					can_be_stopped;
	log_filed_status		field_status[sizeof(_log_field_list)/sizeof(log_field_t)];
}_log_status;


/*typedef struct
{ 
	volatile unsigned short counter; 
} atomic_t;*/
//static struct atomic_t rp, wp;

/* ringbufput 无法写入单字节！ */
static unsigned char imu_id[7] = {'I', 'M', 'U', 'F', 'L', 'A', 'G'};
static unsigned char mag_id[7] = {'M', 'A', 'G', 'F', 'L', 'A', 'G'};
static unsigned char gps_id[7] = {'G', 'P', 'S', 'F', 'L', 'A', 'G'};

#define sd_path "/mnt/sd/znn/usv_platform_20210526.txt"
static log_header_t _log_header = { sizeof(_log_field_list) / sizeof(log_field_t), _log_field_list };
static FILE* _log_fid_t;
static char log_buf[RING_BUF_MAX_LEN];		//环形缓存区(静态创建)
static struct ringbuffer logger_buf;		//环形缓存结构体(静态创建)
static int rb_rlen;							//环形缓存代读取数据长度
static pthread_mutex_t deu_mutex = PTHREAD_MUTEX_INITIALIZER;
static sem_t IMU_sem;						//imu话题信号量


//创建节点信息
static struct deu_node Node_IMU;
static struct deu_node Node_MAG;
static struct deu_node Node_GPS;

DeuNode_t Node_IMU_t = NULL;
DeuNode_t Node_MAG_t = NULL;
DeuNode_t Node_GPS_t = NULL;

//话题信息
extern struct deu_topic imu;
extern struct deu_topic mag;
extern struct deu_topic gps;

/* 寻找注册的日志字段 */
static int _find_filed_index(unsigned char msg_id)
{
	for(int i = 0 ; i < sizeof(_log_field_list)/sizeof(log_field_t) ; i++){
		if(_log_field_list[i].msg_id == msg_id) {
			return i;
		}
	}	
	return -1;
}

static char write_buffer[256] = {0};
/* 应更改为自动寻找设备 */
static int buffer_log(void)
{		
	struct bmi088_data imu_sample = {0};	//惯导
	struct hmc5883_data mag_sample = {0};	//磁力计
	struct gps_frame gps_sample = {0};		//gps
	int res;
	int len;

	imu_sample_t = &imu_sample;
	mag_sample_t = &mag_sample;
	gps_sample_t = &gps_sample;

	int index;
	if((_log_fid_t == NULL || !_log_status.is_logging)){
		printf("_log_fid_t == NULL || !_log_status.is_logging__test\n");
		return -1;
	}

	res = deu_poll_sync(&imu, Node_IMU_t, &imu_sample);
	/* 存储方式为字符串 */
	if(0 == res){										//同步信号
		ringbuffer_put(&logger_buf, (unsigned char *)imu_id, 7);								//协议帧头
		//res = ringbuffer_put(&logger_buf, (unsigned char *)imu_sample_t, sizeof(imu_sample));	//原始数据
		len = sprintf(write_buffer, " %f %f %f %f %f %f", imu_sample.acc_x, imu_sample.acc_y, imu_sample.acc_z, imu_sample.gyr_x, imu_sample.gyr_y, imu_sample.gyr_z);
		res = ringbuffer_put(&logger_buf, (unsigned char *)write_buffer, len);	//imu数据
		
		if(res < len){					//res < sizeof(imu_sample)
			printf("imu_data lost.\n");
			index = _find_filed_index(*imu_id);
			_log_status.field_status[index].lost_msg += 1;										//记录数据丢失
		}else{
			printf("IMU: ringbuf write length is %d.\n", res);
		}

		if(!deu_poll(&mag, Node_MAG_t, &mag_sample)){
			ringbuffer_put(&logger_buf, (unsigned char *)mag_id, 7);
			//res = ringbuffer_put(&logger_buf, (unsigned char *)mag_sample_t, sizeof(mag_sample));	//原始数据
			len = sprintf(write_buffer, " %f %f %f", mag_sample.x, mag_sample.y, mag_sample.z);
			res = ringbuffer_put(&logger_buf, (unsigned char *)write_buffer, len);
			
			if(res < len){				//res < sizeof(mag_sample)
				printf("mag_data lost.\n");
				index = _find_filed_index(*mag_id);
				_log_status.field_status[index].lost_msg += 1;
			}else{
				printf("MAG: ringbuf write length is %d.\n", res);
			}
		}else{
			printf("failed to get mag data!\n");
		}

		if(!deu_poll(&gps, Node_GPS_t, &gps_sample)){
			ringbuffer_put(&logger_buf, (unsigned char *)gps_id, 7);
			len = sprintf(write_buffer, " %f %f %f %f %f %f %c %c %hd %f %f %f %f", 
							gps_sample.lat, gps_sample.lon, gps_sample.height, gps_sample.vel.x, gps_sample.vel.y, gps_sample.vel.z,
							gps_sample.fixType, gps_sample.numSV, gps_sample.pDOP, gps_sample.hAcc, gps_sample.vAcc, gps_sample.sAcc, gps_sample.time_us);
			//res = ringbuffer_put(&logger_buf, (unsigned char *)gps_sample_t, sizeof(gps_sample));	//原始数据
			res = ringbuffer_put(&logger_buf, (unsigned char *)write_buffer, len);	//原始数据
			if(res < len){					//res < sizeof(gps_sample)
				printf("gps_data lost.\n");
				index = _find_filed_index(*gps_id);
				_log_status.field_status[index].lost_msg += 1;
			}else{
				printf("GPS: ringbuf write length is %d.\n", res);
			}
		}else{
			printf("failed to get gps data!\n");
		}

		*write_buffer = '\n';
		res = ringbuffer_put(&logger_buf, (unsigned char *)write_buffer, 1);	//换行
		return 0;
	}else{
		//printf("deu_poll_sync return value is %d.\n", res);
		printf("failed to get imu data!\n");
		return -1;
	}
}

static int sd_log_write(void)
{	
	int res;
	int size;
	int len;
	unsigned char extra_buf[BLOCK_LEN] = {0};

	/* atomic 头文件寻找失败 */
	/*atomic_set(&rp.counter, &logger_buf.read_index);
	atomic_set(&wp.counter, &logger_buf.write_index);*/

	unsigned short rp;
	unsigned short wp;
	/* 锁信号量 */
	pthread_mutex_lock(&deu_mutex);//保证在某一时刻只有一个线程能访问关键数据
	rp = logger_buf.read_index;
	wp = logger_buf.write_index;
	pthread_mutex_unlock(&deu_mutex);
	
	/* log file log is not open */
	if(_log_fid_t == NULL){
		printf("FILE is closed now.\n");
		return -1;
	}

	/* 一次拷贝BLOCK_LEN长度的数据至SD */
	while(ringbuffer_data_len(&logger_buf)){
		/* ringbuf剩余数据不足BLOCK_LEN,       获取剩余长度 */
		if((ringbuffer_get(&logger_buf, extra_buf, BLOCK_LEN)) < BLOCK_LEN){			
			if(wp > rp){
				len = wp - rp;
			}else{
				len = logger_buf.buffer_size - (rp - wp);
			}
			_log_status.can_be_stopped = 1;			//自动保存标志位	

			
		}else{
			len = BLOCK_LEN;
		}

		size = fwrite(*(&extra_buf), sizeof(unsigned char), len, _log_fid_t);

		if(size){
			printf("%d byte be writen.\n", size);
		}
	}

	//printf("%d is_logging , %d can_be_stopped \n", _log_status.is_logging, _log_status.can_be_stopped);
	/* 保存数据方式：1.通过用户输入stop       logging置标志位 */        
	if(!(_log_status.is_logging) && (_log_status.can_be_stopped)){
		printf("auto close sd.\n");
		res = fclose(_log_fid_t);
		if(0 == !res){
			printf("fail to close sd.\n");
		}else{
			printf("success to close sd.\n");
			
		}
	}

	/* 保存数据方式：2.通过缓存区数据读取完毕置标志位，需要手动开启文件fopen */

	return 0;
}

/* linux下读写sd卡，需要先挂载设备 */
static int sd_log_start(const char *pathname)
{
	_log_fid_t = fopen(pathname, "a+");
	if(NULL == _log_fid_t){
		printf("sd card open failed!\n");
		return -1;
	}else{
		printf("sd card open success_test.\n");
	}
	
	/* set log_status */
	strcpy(_log_status.file_name, "sensor_data");
	_log_status.is_logging = 1;
	/* initialization ringbuf */
	ringbuffer_init(&logger_buf, log_buf, RING_BUF_MAX_LEN);
		
	/* write header */
	ringbuffer_put(&logger_buf, &_log_header.num_field, sizeof(_log_header.num_field));    // write num_filed

	/* write log filed */
	for(int n = 0 ; n < _log_header.num_field ; n++) {
		ringbuffer_put(&logger_buf, _log_header.filed_list[n].name, LOG_MAX_NAME_LEN);
		ringbuffer_put(&logger_buf, &_log_header.filed_list[n].msg_id, sizeof(_log_header.filed_list[n].msg_id));
		ringbuffer_put(&logger_buf, &_log_header.filed_list[n].num_elem, sizeof(_log_header.filed_list[n].num_elem));

		/* write log element */
		for(int k = 0 ; k < _log_header.filed_list[n].num_elem ; k++) {
			ringbuffer_put(&logger_buf, _log_header.filed_list[n].elem_list[k].name, LOG_MAX_NAME_LEN);
			ringbuffer_put(&logger_buf, (unsigned char *)&_log_header.filed_list[n].elem_list[k].type, sizeof(_log_header.filed_list[n].elem_list[k].type));
			ringbuffer_put(&logger_buf, (unsigned char *)&_log_header.filed_list[n].elem_list[k].number, sizeof(_log_header.filed_list[n].elem_list[k].number));
		}
	}
	return 0;
	//printf("start logging now...\n");	
}

static void log_stop(void)
{
	_log_status.is_logging = 0;
}

/*************************************************************************************
 *function 		subscribe topic
 *parameter 	void
 *return		0 succeed  -1 fail
 *************************************************************************************/
static int topic_sub(void)
{
	int res = 0;
	//sem_t IMU_sem;
	sem_init(&IMU_sem, 0, 0);
	
	Node_IMU_t = deu_subscribe(&imu, &IMU_sem, NULL);
	if(Node_IMU_t == NULL){
		printf("IMU topic subscribe failed!\n");
		res = -1;
	}
	
	Node_MAG_t = deu_subscribe(&mag, NULL, NULL);
	if(Node_MAG_t == NULL){
		printf("mag topic subscribe failed!\n");
		res = -1;
	}
	
	Node_GPS_t = deu_subscribe(&gps, NULL, NULL);
	if(Node_GPS_t == NULL){
		printf("GPS topic subscribe failed!\n");
		res = -1;
	}
	return res;	
}

int handle_log_shell_cmd(int argc, char** argv, int optc, sh_optv* optv)
{
	int res = 0;

	if(argc > 1) {
		if(strcmp(argv[1], "start") == 0) {
			if(argc > 2)
				res = sd_log_start((const char *)"/mnt/sd/znn/Data000.txt");	// default period
		}

		if(strcmp(argv[1], "stop") == 0) {
			log_stop();
		}

		/*if(strcmp(argv[1], "status") == 0) {
			log_dump_status();
		}*/
	}
	return res;
}

/* 写sd线程入口 */
void logger_entry(void *arg)
{
	int res;
	int count = 0;	//时间计数
	int flag = 0;;	//文件标志
	char path[32];	//文件路径

	//path = "/mnt/sd/znn/Data000.txt";
	sd_log_start((const char *)"/mnt/sd/znn/Data000.txt");
	
	while(1) {
		/* 约300s数据 or 停止指令（在sd_log_start中执行）保存一次txt文件 */
		if(300 == count){
			count = 0;
			flag++;
			res = fclose(_log_fid_t);
			if(0 == !res){
				printf("fail to close sd.\n");
			}else{
				printf("success to close sd.\n");
			}
			sprintf(path, "/mnt/sd/znn/Data%03d.txt", flag);
			sd_log_start(path);
		}
		
		res = sd_log_write();
		if(!res){
			printf("sd write success.\n");
			
		}else{
			printf("write sd error.\n");
		}

		count++;
		printf("new file open count %d\n", count);
		sleep(1);
	}
}

/* ringbuf环形缓存入口 */
void* log_ringbuf_entry(void *arg)
{
	printf("now in log_ringbuf_entry...\n");
	int res = 0;
	int n = 10;

	res = topic_sub();
	if(res){
		printf("fail to subscribe topic.\n");
	}

	while(1){
		res = buffer_log();
		if(0 == res){
			printf("thread __log_ringbuf_entry__ get data.\n");
		}else{
			printf("__buffer_log__ get no data___test.\n");
		}
	}
}

/* 写sd测试 */
/*void* sd_test(void *arg)
{
	int res;
	int n = 10;
	int flag = 0;;//测试停止标志
	res = sd_log_start();
	if(0 == res){
		printf("start logging now...\n");
	}
	
	while(1) {
		if (0 == n){
			printf("stop sd test......\n");	
			flag = 1;
		}
		if(1 == flag){
			log_stop();
			printf("%d is_logging\n", _log_status.is_logging);
			sd_log_write();		
		}else{
			res = sd_log_write();
			if(!res){
				printf("sd write success.\n");
			}else{
				printf("write sd error.\n");
			}
			n--;
		}			
		sleep(1);
	}

}*/








