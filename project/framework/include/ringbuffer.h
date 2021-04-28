#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

#include <assert.h>// assert
#include <stddef.h>// NULL

/* ring buffer */
struct ringbuffer
{
    unsigned char *buffer_ptr;
    /* 使用最高位msb of the {read,write}_index作为mirror位. You can see this as
     * if the buffer adds a virtual mirror and the pointers point either to the
     * normal or to the mirrored buffer. 如果write_index和read_index相等,但是mirror
     * 值不同,则表示缓存区满,反之表示buffer为空:
     *
     *          mirror = 0                    mirror = 1
     * +---+---+---+---+---+---+---+|+~~~+~~~+~~~+~~~+~~~+~~~+~~~+
     * | 0 | 1 | 2 | 3 | 4 | 5 | 6 ||| 0 | 1 | 2 | 3 | 4 | 5 | 6 | Full
     * +---+---+---+---+---+---+---+|+~~~+~~~+~~~+~~~+~~~+~~~+~~~+
     *  read_idx-^                   write_idx-^
     *
     * +---+---+---+---+---+---+---+|+~~~+~~~+~~~+~~~+~~~+~~~+~~~+
     * | 0 | 1 | 2 | 3 | 4 | 5 | 6 ||| 0 | 1 | 2 | 3 | 4 | 5 | 6 | Empty
     * +---+---+---+---+---+---+---+|+~~~+~~~+~~~+~~~+~~~+~~~+~~~+
     * read_idx-^ ^-write_idx
     * Ref: http://en.wikipedia.org/wiki/Circular_buffer#Mirroring */
    unsigned short read_mirror : 1;// 用于当写/读index相等时
    unsigned short read_index : 15;
    unsigned short write_mirror : 1;
    unsigned short write_index : 15;
    /* as we use msb of index as mirror bit, the size should be signed and
     * could only be positive. */
    unsigned short buffer_size;
};

enum ringbuffer_state
{
    RT_RINGBUFFER_EMPTY,
    RT_RINGBUFFER_FULL,
    /* half full is neither full nor empty */
    RT_RINGBUFFER_HALFFULL,
};

void ringbuffer_init(struct ringbuffer *rb, unsigned char *pool, unsigned short size);
void ringbuffer_reset(struct ringbuffer *rb);
int ringbuffer_put(struct ringbuffer *rb, const unsigned char *ptr, unsigned short length);
int ringbuffer_get(struct ringbuffer *rb, unsigned char *ptr, unsigned short length);
int ringbuffer_data_len(struct ringbuffer *rb);
struct ringbuffer* rt_ringbuffer_create(unsigned short length);
void ringbuffer_destroy(struct ringbuffer *rb);

inline unsigned short ringbuffer_get_size(struct ringbuffer *rb)
{
    assert(rb != NULL);
    return rb->buffer_size;
}

/** 返回可用空间长度 */
#define ringbuffer_space_len(rb) ((rb)->buffer_size - ringbuffer_data_len(rb))

#endif
