#include "extend.h"

static void __link_buffer(struct extend_buf *b, uint32_t eno)
{

}

static void __unlink_buffer(struct extend_buf *b, uint32_t eno)

static void __relink_lru(struct extend_buf *b, uint32_t eno)

static struct extend_buf *__extend_new(struct queue *q, uint32_t eno, int *need_submit)

static void *new_extend(struct queue *q, uint32_t eno, struct extend_buf **bp)

void *extend_buf_new(struct queue *q, uint32_t eno, struct extend_buf **bp)

void *extend_buf_read(struct queue *q, uint32_t eno, struct extend_buf **bp)

void extend_buf_mark_dirty(struct extend_buf *b)
{
	struct queue *q = b->q;

	queue_lock(q);
	BUG_ON(test_bit(B_READING, &b->state));
	if (!test_and_set_bit(B_DIRTY, &b->state))
		__relink_lru(b, LIST_DIRTY);
	queue_unlock(q);
}

void extend_close(struct extend_buf *b)
{
	struct queue *q = b->q;

	queue_lock(q);
	BUG_ON(!b->hold_count);
	b->hold_count--;
	queue_unlock(q);
}

void extend_release(struct extend_buf *b)
{
	struct queue *q = b->q;

	queue_lock(q);
	BUG_ON(!b->hold_count);
	b->hold_count--;

	if (!b->hold_count) {
		pthread_mutex_lock(q->free_lock);
		pthread_cond_signal(q->free_cond);
		pthread_mutex_unlock(q->free_lock);

		if ((b->read_error || b->write_error) &&
		   !test_bit(B_READING, &b->state) &&
		   !test_bit(B_WRITING, &b->state) &&
		   !test_bit(B_DIRTY, &b->state)) {
			__unlink_buffer(b);
			__free_buffer_wake(b);
		}
	}
	queue_unlock(q);
}

void *extend_release_move(struct extend_buf *b, uint32_t new_eno)
{
	struct queue *q = b->q;
	struct extend_buf *new;

	queue_lock(q);
	new = __find(q, new_eno);
	if (new) {
		/* found it */
	}

	BUG_ON(!b->hold_count);
	BUG_ON(test_bit(B_READING, &b->state));

	if (b->hold_count == 1) {
	} else {
	}
	queue_unlock(q);
}

struct extend_buf *queue_create()
{

}

void queue_destroy()
{

}
