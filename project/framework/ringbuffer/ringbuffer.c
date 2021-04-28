#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memcpy
#include "ringbuffer.h"

enum ringbuffer_state ringbuffer_status(struct ringbuffer *rb)
{
    if (rb->read_index == rb->write_index)
    {
        if (rb->read_mirror == rb->write_mirror)
            return RT_RINGBUFFER_EMPTY;
        else
            return RT_RINGBUFFER_FULL;
    }
    return RT_RINGBUFFER_HALFFULL;// 只要读/写index不相等就是halffull
}

void ringbuffer_init(struct ringbuffer *rb,
                        unsigned char        *pool,
                        unsigned short        size)
{
    assert(rb != NULL);
    assert(size > 0);

    /* initialize read and write index */
    rb->read_mirror = rb->read_index = 0;
    rb->write_mirror = rb->write_index = 0;

    /* set buffer pool and size */
    rb->buffer_ptr = pool;
    rb->buffer_size = size;//测一下struct大小,看看是否要对齐
}

/**
 * 存数据
 */
int ringbuffer_put(struct ringbuffer *rb,
                            const unsigned char   *ptr,
                            unsigned short         length)
{
    unsigned short size;

    assert(rb != NULL);

    size = ringbuffer_space_len(rb);// 可用空间

    if (size == 0)
        return 0;

    /* 丢掉部分数据 */
    if (size < length)
        length = size;

    /*存数据包含两种情况:
        1|-----r++++++w---| buf_size-write_index可能>len
        2|+++++w-----r+++| buf_size-write_index一定>len
    */
    if (rb->buffer_size - rb->write_index > length)
    {
        memcpy(&rb->buffer_ptr[rb->write_index], ptr, length);
        rb->write_index += length;
        return length;
    }

    memcpy(&rb->buffer_ptr[rb->write_index],
           &ptr[0],
           rb->buffer_size - rb->write_index);
    memcpy(&rb->buffer_ptr[0],
           &ptr[rb->buffer_size - rb->write_index],
           length - (rb->buffer_size - rb->write_index));

    /*write_index溢出,write_mirror*/
    rb->write_mirror = ~rb->write_mirror;
    rb->write_index = length - (rb->buffer_size - rb->write_index);

    return length;
}

/**
 *  取数据
 */
int ringbuffer_get(struct ringbuffer *rb,
                            unsigned char           *ptr,
                            unsigned short           length)
{
    int size;

    assert(rb != NULL);

    size = ringbuffer_data_len(rb);

    if (size == 0)
        return 0;

    if (size < length)
        length = size;

    /*取数据包含两种情况:
        1|+++++r------w+++| write_index-read_index>len==>buf_size-read_index一定>len
        2|-----w+++++r---| buf_size-read_index可能>len
    */
    if (rb->buffer_size - rb->read_index > length) 
    {
        memcpy(ptr, &rb->buffer_ptr[rb->read_index], length);
        rb->read_index += length;
        return length;
    }

    memcpy(&ptr[0],
           &rb->buffer_ptr[rb->read_index],
           rb->buffer_size - rb->read_index);
    memcpy(&ptr[rb->buffer_size - rb->read_index],
           &rb->buffer_ptr[0],
           length - (rb->buffer_size - rb->read_index));

    rb->read_mirror = ~rb->read_mirror;
    rb->read_index = length - (rb->buffer_size - rb->read_index);

    return length;
}

/** 
 * 返回待读空间的长度
 */
int ringbuffer_data_len(struct ringbuffer *rb)
{
    switch (ringbuffer_status(rb))
    {
    case RT_RINGBUFFER_EMPTY:
        return 0;
    case RT_RINGBUFFER_FULL:
        return rb->buffer_size;
    case RT_RINGBUFFER_HALFFULL:
    default:
        if (rb->write_index > rb->read_index)
            return rb->write_index - rb->read_index;
        else
            return rb->buffer_size - (rb->read_index - rb->write_index);
    };
}

/** 
 * 清空环形存储 
 */
void ringbuffer_reset(struct ringbuffer *rb)
{
    assert(rb != NULL);

    rb->read_mirror = 0;
    rb->read_index = 0;
    rb->write_mirror = 0;
    rb->write_index = 0;
}

// 动态内存分配方式创建
struct ringbuffer* ringbuffer_create(unsigned short size)
{
    struct ringbuffer *rb;
    unsigned char *pool;

	assert(size > 0);

    // size = RT_ALIGN_DOWN(size, RT_ALIGN_SIZE);

    rb = (struct ringbuffer *)malloc(sizeof(struct ringbuffer));
    if (rb == NULL)
        goto exit;

    pool = (unsigned char *)malloc(size);
    if (pool == NULL)
    {
        free(rb);
        rb = NULL;
        goto exit;
    }
    ringbuffer_init(rb, pool, size);

exit:
    return rb;
}

void ringbuffer_destroy(struct ringbuffer *rb)
{
    assert(rb != NULL);

    free(rb->buffer_ptr);
    free(rb);
}

