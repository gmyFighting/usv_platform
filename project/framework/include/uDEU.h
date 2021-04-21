#ifndef _UDEU_H_
#define _UDEU_H_

#include<semaphore.h>

#define DEU_MAX_LINK_NUM 20
// #字符串化 ##将宏参数连接
#define DEU_DEFINE(_name, _size) \
    struct deu_topic = {         \
        .name = #_name,          \
        .size = #_size,          \
    }                            
// 节点
struct deu_node {
    volatile char renewal;// 话题有新数据节点未读取
    sem_t sem;// 长整形union
    void (*cb)(void *parameter);
    struct deu_node * next;
};
typedef struct deu_node DeuNode;
typedef struct deu_node * DeuNode_t;

// 话题类型
struct deu_topic {
    const char * name;
    const unsigned int size;
    void *data;
    DeuNode_t link_head;
    DeuNode_t link_tail;
    unsigned short link_size;
    unsigned char published;// 话题是否发布
};
typedef struct deu_topic * DeuTopic_t;

// 话题链表
struct topic_list {
    DeuTopic_t tpc_t;
    struct topic_list * next;
};
typedef struct topic_list TopicList;
typedef struct topic_list *TopicList_t;

int deu_advertise(DeuTopic_t tpc);
DeuNode_t deu_subscribe(DeuTopic_t tpc, sem_t sem, void (*cb)(void *parameter));
int deu_publish(DeuTopic_t tpc, const void* data);

#endif // !_UDEU_H_

