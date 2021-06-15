 // linux串口驱动设置函数文件

#include <stdio.h>// perror
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "uart.h"
#include "sensor_collect.h"

static const int FALSE = -1;
static const int TRUE = 0;
/* 驱动文件地址 */
static struct uart_devieve_addr uart_addr = {
    .uart_sensor_num = 3,
    .ublox = "/dev/ttymxc1",    
    .dvl = "/dev/ttymxc2",
    .imu04b = "/dev/***",    
};


/*
 * 名称： UART_Open
 * 功能： 打开串口并返回串口设备文件描述
 * 入口参数： fd :文件描述符 port :串口号(ttyUSB0,ttymxc1,ttymxc2)
 * 出口参数： 正确返回为0，错误返回为-1
 */
int uart_open(int fd, char* port)
{
    int res;
    //int fd;
    // 返回最小的未被使用的描述符(后续操作都基于该描述符)
    // O_NOCTTY不把该设备作为终端设备,程序不会成为这个端口的控制终端,任何一个输入,例如键盘中止信号等,都将影响进程。
    // O_NONBLOCK/O_NDELAY非阻塞方式读取,不关心端口另一端状态
    // flag都是用八进制表示
    // bugs:O_NOCTTY加上flag没变化加上flag没变化,好像和串口是终端有关
    fd = open(port, O_RDWR|O_NONBLOCK);
    if (fd == FALSE) {
        perror("Can't Open Serial Port");
        return(FALSE);
    }

    // int flag = fcntl(fd, F_GETFL, 0);
    // flag |= O_NONBLOCK;
    // res = fcntl(fd, F_SETFL, flag);
    // if (res < 0) {
    //     printf("fcntl failed:%d\n", res);
    // }
    
    //测试是否为终端设备 1是终端 0不是终端
    if (isatty(fd) == 0) {
        printf("tty is not a terminal device\n");
        return(FALSE);
    }

    //*fd_t = fd;
    printf("uart:%s open\n",port);
    return fd;
}

void uart_close(int fd)
{
    close(fd);
}

/*
 * 名称： UART_Set
 * 功能： 设置串口数据位，停止位和效验位
 * 入口参数: 
 * fd 文件描述符
 * speed 串口速度 如B115200
 * flow_ctrl 数据流控制
 * databits 数据位 7/8
 * stopbits 停止位 1/2
 * parity 效验位 N/E/O/S
 * 出口参数： 
 * 正确返回为0，错误返回为-1
 */
int uart_set(int fd, speed_t speed, int flow_ctrl, int databits, int stopbits, char parity)
{
    // int status;
    struct termios options;

    /*tcgetattr获得终端参数
     *tcsetattr修改终端参数*/
    if (tcgetattr(fd, &options) != 0) {
        fprintf(stderr,"SetupSerial 1\n");
        return(FALSE);
    }
        
    /*cfsetispeed设置输入波特率
     *cfsetospeed设置输出波特率*/
    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);

	//修改控制模式，保证程序不会占用串口        
    options.c_cflag |= CLOCAL;
    //修改控制模式，使得能够从串口中读取输入数据
    options.c_cflag |= CREAD;
    //设置数据流控制
    switch (flow_ctrl) {
    case 0 : //不使用流控制
        options.c_cflag &= ~CRTSCTS;
    break;    
    case 1 : //使用硬件流控制
        options.c_cflag |= CRTSCTS;
    break;
    case 2 : //使用软件流控制
        options.c_cflag |= IXON | IXOFF | IXANY;
    break;
    }

    //设置数据位
    options.c_cflag &= ~CSIZE; //屏蔽其他标志位
    switch (databits) {
        case 5:
            options.c_cflag |= CS5;
        break;
        case 6:
            options.c_cflag |= CS6;
        break;
        case 7:
            options.c_cflag |= CS7;
        break;
        case 8:
            options.c_cflag |= CS8;
        break;
        default:
            fprintf(stderr,"Unsupported data size\n");
            return (FALSE);
    }

    //设置校验位
    switch (parity) {
    case 'n':
    case 'N': //无奇偶校验位。
        options.c_cflag &= ~PARENB;
        options.c_iflag &= ~INPCK;
    break;
    case 'o':
    case 'O': //奇校验
        options.c_cflag |= (PARODD | PARENB);
        options.c_iflag |= INPCK;
    break;
    case 'e':
    case 'E': //偶校验
        options.c_cflag |= PARENB;
        options.c_cflag &= ~PARODD;
        options.c_iflag |= INPCK;
    break;
    case 's':
    case 'S': //设置为空格
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
    break;
    default:
        fprintf(stderr,"Unsupported parity\n");
        return (FALSE);
    }

     // 设置停止位
    switch (stopbits) {
    case 1:
        options.c_cflag &= ~CSTOPB;
    break;
    case 2:
        options.c_cflag |= CSTOPB;
    break;
    default:
        fprintf(stderr,"Unsupported stop bits\n");
        return (FALSE);
    break;
    }

    //修改输出模式，原始数据输出
    options.c_oflag &= ~OPOST;
    //设置等待时间和最小接收字符
    options.c_cc[VTIME] = 1; /* 读取一个字符等待1*(1/10)s */
    options.c_cc[VMIN] = 1; /* 读取字符的最少个数为1 */
	options.c_lflag &= ~(ICANON | ECHO | ECHOE);
	
    /*TCIFLUSH清除正收到的数据,且不会读出来
      TCOFLUSH清楚正写入的数据,且不会发送至终端
      TCIOFLUSH清除所有正在发送的I/O数据*/
    tcflush(fd,TCIFLUSH);
   
    /* 设置终端
     * TCSANOW：立刻对值进行修改*/
    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        fprintf(stderr,"com set error!/n");
        return (FALSE);
    }
    return (TRUE);
}

int uart_init(int fd, int speed,int flow_ctrl, int databits,int stopbits,char parity)
{
    //设置串口数据帧格式
    if (FALSE == uart_set(fd,speed,flow_ctrl,databits,stopbits,parity)) {         
        return FALSE;
    } else {
        printf("uart set over\n");
        return TRUE;	   
    } 
}

/*
 * 名称： UART_Recv
 * 功能： 接收串口数据
 * 入口参数： fd :文件描述符
 * rcv_buf :接收串口中数据存入rcv_buf缓冲区中
 * data_len :一帧数据的长度
 * 出口参数： 正确返回为len，错误返回为-1
 */
int uart_recv(int fd, char *rcv_buf, int data_len)
{
    // 判断输入参数正确性
    

    int len = 0;
    int fs_sel = 0;
    fd_set fs_read;
    
    struct timeval time;
    
    FD_ZERO(&fs_read);
    FD_SET(fd,&fs_read);
    
    time.tv_sec = 5;
    time.tv_usec = 0;
    
    // 用于监视文件句柄,没数据会阻塞
    fs_sel = select(fd+1,&fs_read,NULL,NULL,&time);
    if(fs_sel) {
        len = read(fd,rcv_buf,data_len);// 返回字节数
        return len;
    } else {
	printf("select() error：no data\n");
       return FALSE;	
     }    
}

/*
 * 名称： UART_Send
 * 功能： 发送数据
 * 入口参数： fd :文件描述符
 * send_buf :存放串口发送数据
 * data_len :一帧数据的个数
 * 出口参数： 正确返回为0，错误返回为-1
 */
int uart_send(int fd, char *send_buf,int data_len)
{
    int ret;
    
    ret = write(fd,send_buf,data_len);
    if (data_len == ret ) {    
        return ret;
    } 
    else {
        tcflush(fd,TCOFLUSH);
        return FALSE;  
    } 
}

/*  uart deviece initialization */ 
int* uart_deviece_init(int *fd)
{	
	int res;
	int *sfd;
	*sfd = *fd;
	
	/* ublox init */
	*sfd = uart_open(*sfd, uart_addr.ublox);

	res = uart_init(*sfd, B115200, 0, 8, 1, 'N');
	if(res = FALSE){
		printf("ublox_uart init failed.\n");
		return (int*)(-1);
	}

	/* dvl init */
	*(sfd + 1) = uart_open(*(sfd + 1), uart_addr.dvl);

	res = uart_init(*(sfd + 1), B9600, 0, 8, 1, 'N');
	if(res = FALSE){
		printf("dvl_uart open failed.\n");
		return (int*)(-1);
	}

	/* imu040b init */
	*(sfd + 2) = uart_open(*(sfd + 2), uart_addr.imu04b);

	res = uart_init(*(sfd + 2), B921600, 0, 8, 1, 'E');
	if(res = FALSE){
		printf("imu040b_uart open failed.\n");
		return (int*)(-1);
	}
	
	return *sfd;
}

