/*************************************************************************************************
 *	ublox_gps analysis
 *
 * 
 *
 *
 *
 *
 *
 ************************************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<termios.h>
#include<errno.h>
#include<string.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include "ublox.h"

#define FALSE -1
#define TRUE 0
#define BUFLENGHT 512

// 帧头(2)+类(1)+ID(1)+LEN(2)+PAYLOAD+CheckSum(2)
#define UBX_HEADER1 0
#define UBX_HEADER2 1
#define UBX_CLASS   2
#define UBX_ID      3
#define UBX_LENGTH1 4
#define UBX_LENGTH2 5
#define UBX_PAYLOAD 6
#define UBX_CK_A    7
#define UBX_CK_B    8

// UBX Protocol帧头
#define SYNC_CHAR1 0xB5
#define SYNC_CHAR2 0x62
// UBX Class
#define UBX_NAV 0x01
/* Class: UBX_NAV
 * Name : DOP(dilution of precision)
 * 名称 : 几何精度因子
 * 说明 : 衡量观测卫星在空间几何分布对定位精度影响
 * 分类 :
 *       gDOP: Geometric DOP
 *       pDOP: position DOP
 *       vDOP: vertical DOP
 *       hDOP: Horizontal DOP 
 */
#define UBX_NAV_DOP 0x04
/* Class: UBX_NAV
 * Name : PVT(Position Velocity Time Solution)
 * 名称 : 位置速度时间方案
 * 说明 : 无
 * 分类 :
 *       tACC: Time accuracy estimate (UTC)
 *       numSV: Number of satellites used in Nav Solution
 *       hMSL: Height above mean sea level
 *       hACC: Horizontal accuracy estimate 
 *       vACC: Vertical accuracy estimate 
 *       sACC: Speed accuracy estimate 
 *       headACC: Heading accuracy estimate (both motion and vehicle)
 *       headMot: Heading of motion (2-D)
 *       headVeh: Heading of vehicle (2-D)
 *       magDec: Magnetic declination 
 *       magACC: Magnetic declination accuracy
 */
#define UBX_NAV_PVT 0x07
#define GPS_READ_SIZE 128
/* Rx NAV-PVT (ubx8) */
typedef struct {
	unsigned int	iTOW;		  /**< GPS time of week of the navigation epoch > [ms]**/
	unsigned short	year; 		  /**< Year > [y]**/
	char			month; 		  /**< Month > [month]**/
	char			day; 		  /**< Day of month > [d]**/
	char			hour; 		  /**< Hour of day > [h]**/
	char			min; 		  /**< Minute of hour > [min]**/
	char			sec;		  /**< Seconds of minute > [s]*/
	char			valid; 		  /**< Validity flags > [-]**/
	unsigned int	tAcc; 		  /**< Time accuracy estimate > [ns]**/
	int				nano;		  /**< Fraction of second > [-1e9...1e9 ns]**/
	char			fixType;	  /**< GNSSfix Type > [0:no fix/1:dead reckoning only/2:2D-fix/3:3D-fix/4:GNSS+dead reckoning combined]**/
	char			flags;		  /**< Fix Status Flags > [-]**/
	char			flags2;       /**< Additional Flags > [-]**/
	char			numSV;		  /**< Number of SVs used in Nav Solution > [-]**/
	int				lon;		  /**< Longitude > [deg(1e-7)]**/
	int				lat;		  /**< Latitude > [deg(1e-7)]**/
	int				height;		  /**< Height above ellipsoid > [mm]**/
	int				hMSL;		  /**< Height above mean sea level > [mm]**/
	unsigned int	hAcc;  		  /**< Horizontal accuracy estimate > [mm]**/
	unsigned int	vAcc;  		  /**< Vertical accuracy estimate > [mm]**/
	int				velN;		  /**< NED north velocity > [mm/s]**/
	int				velE;		  /**< NED east velocity > [mm/s]**/
	int				velD;		  /**< NED down velocity > [mm/s]**/
	int				gSpeed;		  /**< Ground Speed(2-D) > [mm/s]**/
	int				headMot;	  /**< Heading of motion(2-D) > [deg(1e-5)]**/
	unsigned int	sAcc;		  /**< Speed accuracy estimate > [mm/s]**/
	unsigned int	headAcc;	  /**< Heading accuracy estimate(both motion and vehicle) > [deg(1e-5)]**/
	unsigned short	pDOP;		  /**< Position DOP > [0.01]**/
    char     		reserved1[6]; /**< Reserved > [-]**/
    int     		headVeh;      /**< Heading of vehicle(2-D) > [deg(1e-5)]**/
    short     		magDec;       /**< Magnetic declination > [deg(1e-2)]**/
    unsigned short  magAcc;       /**< Magnetic declination accuracy > [deg(1e-2)]**/
} ubx_nav_pvt_payload_t;

// 用于快速解析ubx协议
typedef union {
    ubx_nav_pvt_payload_t    ubx_nav_pvt_buffer;
    char                  gps_buffer_raw[92];
} ubx_buf_t;

// ubx校验位
static char ck_a;
static char ck_b;
// 处理ubx单字节 
static char parse_state = 0;
static short int payload_length = 0;
static short int payload_index = 0;
// 快速解析ubx
ubx_buf_t ubx_buf;
struct gps_frame gps_sample_new = {0};
char gps_published = 0;

static void _parse_char_reset(void)
{
    parse_state = UBX_HEADER1;
    ck_a = 0;
    ck_b = 0;
    payload_length = 0;
    payload_index = 0;
}


static void _publish_gps_msg()
{			
	gps_sample_new.fixType = ubx_buf.ubx_nav_pvt_buffer.fixType;
    gps_sample_new.hAcc = (double)(ubx_buf.ubx_nav_pvt_buffer.hAcc)/1000.0;
    gps_sample_new.height = (double)(ubx_buf.ubx_nav_pvt_buffer.height)/1000.0;
    gps_sample_new.lat = (double)(ubx_buf.ubx_nav_pvt_buffer.lat)/10000000.0;//*deg2rad;
    gps_sample_new.lon = (double)(ubx_buf.ubx_nav_pvt_buffer.lon)/10000000.0;//*deg2rad;
    gps_sample_new.numSV = (unsigned short)ubx_buf.ubx_nav_pvt_buffer.numSV;
    gps_sample_new.pDOP = ubx_buf.ubx_nav_pvt_buffer.pDOP;
    gps_sample_new.sAcc = (double)(ubx_buf.ubx_nav_pvt_buffer.sAcc)/1000.0;
    gps_sample_new.vAcc = (double)(ubx_buf.ubx_nav_pvt_buffer.vAcc)/1000.0;
    gps_sample_new.vel.x = (double)(ubx_buf.ubx_nav_pvt_buffer.velN)/1000.0;
    gps_sample_new.vel.y = (double)(ubx_buf.ubx_nav_pvt_buffer.velE)/1000.0;
    gps_sample_new.vel.z = (double)(ubx_buf.ubx_nav_pvt_buffer.velD)/1000.0;
    gps_sample_new.time_us = 1.0;
//    if (gps_sample_new.time_us < 200000)
//    {
//        return;
//    }
    gps_published = 1;
}


static void _ubx_checksum(const char ch)
{
	ck_a += ch;
	ck_b += ck_a;
}

int _ubx_parse_char(char *data, int length)  
{
	for(int i = 0; i < length; i++)
	{
		char ch = *(data + i);
		switch(parse_state) {
    	case UBX_HEADER1:
        	if(ch == SYNC_CHAR1) {
            	parse_state = UBX_HEADER2;    
        	}
    	break;
        
    	case UBX_HEADER2:
        	if(ch == SYNC_CHAR2) {
            	parse_state = UBX_CLASS; 
        	}
        else {
            	parse_state = UBX_HEADER1; 
            	ck_a = 0;
            	ck_b = 0;
        	}
    	break;
        
    	case UBX_CLASS:       
        	if(ch == UBX_NAV) {
            	_ubx_checksum(ch);
            	parse_state = UBX_ID;
        	}
    	break;
        
    	case UBX_ID:
        	if(ch == UBX_NAV_PVT) {
          	_ubx_checksum(ch);
          	parse_state = UBX_LENGTH1;
        	}
    	break;
        
    	case UBX_LENGTH1:
        	_ubx_checksum(ch);
        	payload_length = ch;
        	parse_state = UBX_LENGTH2;
    	break;
    
    	case UBX_LENGTH2:
        	_ubx_checksum(ch);
        	payload_length |= ch << 8;
        	if(payload_length == 92) {
            	parse_state = UBX_PAYLOAD;
        	}
        	else {
            	_parse_char_reset();
        	}
    	break;
        
    	case UBX_PAYLOAD:
        	_ubx_checksum(ch);
        	ubx_buf.gps_buffer_raw[payload_index++] = ch;
        	if(payload_index == payload_length) {
           		parse_state = UBX_CK_A;
        	}
		break;   
        
    	case UBX_CK_A:
        	if(ck_a != ch) 
        	{
            	_parse_char_reset();
        	}
        	else 
        	{
            	parse_state = UBX_CK_B;
        	}
    	break;
        
    	case UBX_CK_B:
        	if(ck_b != ch) 
        	{
            	printf("ubx pvt ch_b error!\n");
        	}
        	else 
        	{
            	_publish_gps_msg();   
        	}
        	_parse_char_reset();
    		break;
    	}		
	}
    
    return 0;
}


/* ublox_analys:获取最新的一帧，解析，发布数据     */
/* param: fd:串口文件标识符 */
/* return: -1:fail 0:success */
int ublox_analys(int fd)			
{
	int len;
	char *databuf;
	databuf = calloc(BUFLENGHT, sizeof(char));
	
	len = uart_recv(fd, databuf, BUFLENGHT);
	if(FALSE == len){
		printf("uart_recv failed to get data.\n");	
		return -1;
	}
	
	_ubx_parse_char(databuf, len);			//+++++减少解析数据量？

	free(databuf);
	return 0;	
}























