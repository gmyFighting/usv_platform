#ifndef _UART_H_
#define _UART_H_

#include <termios.h> // B115200等

#define UART2_NAME "/dev/ttymxc1"
#define UART3_NAME "/dev/ttymxc2"

int uart_open(int *fd_t, char* port);
void uart_close(int fd);
int uart_set(int fd, speed_t speed, int flow_ctrl, int databits, int stopbits, char parity);
int uart_init(int fd, int speed, int flow_ctrl, int databits, int stopbits, char parity);
int uart_recv(int fd, char *rcv_buf, int data_len);
int uart_send(int fd, char *send_buf, int data_len);

#endif
