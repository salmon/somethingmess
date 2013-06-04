#ifndef __EXTEND_H__
#define __EXTEND_H__

#include "list.h"

#define LIST_CLEAN	0
#define LIST_DIRTY	1
#define LIST_SIZE	2

enum {
	B_READING = 0,
	B_WRITING = 1,
	B_DIRTY = 2,
}

struct queue {
	struct list_head lru[LIST_SIZE];
	struct hlist_head cache_hash;

	struct list_head resvered_buffers;
	unsigned int need_resvered_buffers;

	pthread_mutex_t free_lock;
	pthread_cond_t free_cond;

	pthread_mutex_t lock;
};

struct extend_buf {
	struct hlist_node hash_list;
	struct list_head lru_list;
	unsigned int state;
	unsigned int hold_cnt;
	int error;
	unsigned long last_accessed;

	uint32_t eno;
	char *data;

	pthread_mutex_t lock;
	pthread_cond_t cond;

	struct queue *q;
};

#endif
