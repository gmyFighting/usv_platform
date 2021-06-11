#ifndef __LOOP_LOOGER_H__
#define __LOOP_LOOGER_H__


#define LOG_MAX_NAME_LEN            20
#define RING_BUF_MAX_LEN			16384
#define BLOCK_LEN					4096

typedef struct {
	char name[LOG_MAX_NAME_LEN];
	unsigned short type;
	unsigned short number;
}log_elem_t;

typedef struct {
	char name[LOG_MAX_NAME_LEN];
	unsigned char msg_id;
	unsigned char num_elem;
	log_elem_t* elem_list;
}log_field_t;


typedef struct {
	unsigned char num_field;
	log_field_t* filed_list;
}log_header_t;

/* cmd 文件设置 */
typedef struct {
	char* opt;
	char* val;
} sh_optv;


void* log_ringbuf_entry(void *arg);
//void* sd_test(void *arg);
void logger_entry(void *arg);


#endif







