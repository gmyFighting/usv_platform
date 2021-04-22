/*
 * uDEU(Micro Data Exchange Unit)类ROS通信的一种基于Posix多线程的
 * 通信方案
 */
#include <stdlib.h>// malloc
#include <pthread.h>
#include <stdio.h>// printf
#include <string.h>// strcmp
#include <assert.h>// assert
#include "uDEU.h"

static TopicList _Topic_List = {NULL, NULL};
static pthread_mutex_t deu_mutex = PTHREAD_MUTEX_INITIALIZER;

int deu_advertise(DeuTopic_t tpc)
{
    assert(tpc != NULL);
    
    // 查看话题名是否重名
    // pthread_mutex_lock();
    TopicList_t cp = &_Topic_List;// 用于操作链表指针
    while (cp->next) {
        if (strcmp(cp->tpc_t->name, tpc->name) == 0){
            printf("topic name was already exist\n");
            return -1;
        } 
        cp = cp->next;/* find last node */
    }

    // 创建话题 原子操作
    tpc->data = calloc(tpc->size, 1);
    if (tpc->data == NULL) {
        printf("malloc failed\n");
        return -1;
    }

    // 将话题添加到话题列表全局变量中
    /* update Mcn List */

    if (cp->tpc_t != NULL) {// 这个if是不是无意义
        cp->next = (TopicList_t)malloc(sizeof(TopicList));

        if (cp->next == NULL) {
            printf("malloc failed\n");
            return -1;
        }

        cp = cp->next;
    }

    cp->tpc_t = tpc;
    cp->next = NULL;
    //exit atomix

    return 0;
}

DeuNode_t deu_subscribe(DeuTopic_t tpc, sem_t sem, void (*cb)(void *parameter))
{
    printf("deu_subscribe start\n");
    assert(tpc != NULL);

    if (tpc->link_size >= DEU_MAX_LINK_NUM) {// 最多30个节点订阅该话题
        printf("deu link num is full!\n");
        return NULL;
    }

    // 在话题下添加订阅节点
    DeuNode_t node = (DeuNode_t)malloc(sizeof(DeuNode));
    if (node == NULL) {
        printf("deu create node fail!\n");
        return NULL;
    }
    else {
        printf("create node success!\n");
    }
	// 初始化节点
    node->renewal = 0;
    node->sem = sem;
    node->cb = cb;

    // MCN_ENTER_CRITICAL;// 临界段保护hub
    /* no node link yet */
    if (tpc->link_tail == NULL) {
        printf("no node link yet\n");
        tpc->link_head = tpc->link_tail = node;
    } else {// 尾节点又用吗？？？？
        // 更新原尾节点的next
        tpc->link_tail->next = node;
        // 更新尾节点
        tpc->link_tail = node;      
    }
    tpc->link_size++;
    printf("tpc->link_size = %d\n", tpc->link_size);
    //退出临界段

    if ((tpc->published == 1) && (node->cb != NULL)) {
        /* 若当前该话题已经有节点发布，则立即执行回调函数 */
        node->cb(tpc->data);
    }

    return node;
}

int deu_publish(DeuTopic_t tpc, const void* data)
{
    assert(tpc != NULL);    

    assert(data != NULL);

    // 该话题还未advertised，即未分配空间
    assert(tpc->data != NULL);

    int sem_val = 0;
    // MCN_ENTER_CRITICAL原子操作
    // 更新话题下data
    memcpy(tpc->data, data, tpc->size);
    /* 遍历话题下节点 */
    DeuNode_t node = tpc->link_head;

    // 发布信号量
    while (node != NULL) {
        /* 更新节点读取新消息标志位 */
        node->renewal = 1;
        /* send out event to wakeup waiting task */
        // if (node->sem != 0) {
        //     /* stimulate as mutex */
        //     sem_getvalue(&node->sem, &sem_val);
        //     if (sem_val == 0)
                printf("sem_post\n");
                sem_post(&node->sem);
        // }

        node = node->next;
    }
    printf("deu_publish\n");
    tpc->published = 1;
    // MCN_EXIT_CRITICAL;

    /* 执行回调函数 */
    node = tpc->link_head;

    while (node != NULL) {
        if (node->cb != NULL) {
            node->cb(tpc->data);
        }

        node = node->next;
    }   

    return 0;
}

// 考虑节点中是否要包含话题信息
int deu_poll_sync(DeuTopic_t top, DeuNode_t node, void * buf)
{
    assert(top != NULL);
    assert(node != NULL);
    assert(buf != NULL);
 
    if (top->data == NULL) {
        return -1;
    }

    if (!top->published) {
        return -1;
    }

    if (sem_wait(&node->sem) == 0) {
        memcpy(buf, top->data, top->size);
        node->renewal = 0;
    }

    return 0;
}


