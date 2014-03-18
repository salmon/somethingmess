#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "list.h"

#define BLK_SIZE 4096
#define GROUP_CNT 1
#define BLKS_PER_GROUP 10240
#define MAX_BLOCKS 122095999

//#define GROUP0_DATA_START 917320
//#define GROUP0_DATA_END 917520
#define GROUP0_DATA_START 850
#define GROUP0_DATA_END 122095999

/* file size between 120M and 1.5G */
#define DIND_MIN_OFFSET 30
#define DIND_MAX_OFFSET 384

#define READ_BUFSIZE (1024 * 1024 * 4)
#define DUMP_THREAD_NUM 4
#define MAX_QUEUED_NUM 2000
#define CHECK_POINT 100000

struct dumpdind_queue {
	int head;
	int tail;
	int size;
	uint32_t block_no[MAX_QUEUED_NUM];
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

struct dump_thread_info {
	pthread_t thread_id;
	int thread_num; 
	int disk_fd;
	struct dumpdind_queue *queue;
};

char zero_buf[BLK_SIZE - DIND_MAX_OFFSET * sizeof(uint32_t)];
static int is_finish = 0;

static int queue_full(struct dumpdind_queue *queue)
{
	return (queue->size >= MAX_QUEUED_NUM);
}

static int queue_empty(struct dumpdind_queue *queue)
{
	return (queue->size == 0);
}

static int enqueue(struct dumpdind_queue *queue, uint32_t blkno)
{
	if (MAX_QUEUED_NUM == queue->size)
		return -1;

	queue->block_no[queue->tail] = blkno;
	queue->tail = (queue->tail + 1) % MAX_QUEUED_NUM;
	queue->size ++;

	return 0;
}

static int dequeue(struct dumpdind_queue *queue, uint32_t *blkno)
{
	if (0 == queue->size)
		return -1;

	*blkno = queue->block_no[queue->head];
	queue->head = (queue->head + 1) % MAX_QUEUED_NUM;
	queue->size --;

	return 0;
}

static int dump_block(uint32_t blkno, char *blk_buf, int disk_fd)
{
	off64_t offset, ret;
	int len = 0;

	offset = (off64_t)blkno * BLK_SIZE;
	ret = lseek64(disk_fd, offset, SEEK_SET);
	if (ret < 0) {
		printf("%s llseek %lu error, %s\n", __func__, offset, strerror(errno));
		return -1;
	}

	len = read(disk_fd, blk_buf, BLK_SIZE);
	if (len != BLK_SIZE) {
		printf("%s read error, %s\n", __func__, strerror(errno));
		return -1;
	}

	return 0;
}

int guess_dind_data(char *blk_data)
{
	int i;
	uint32_t pre_num = 0;
	uint32_t *p_num = (uint32_t *)blk_data;

	int bad_cnt = 0, good_cnt = 0;
	int cost = 0;

	char *p = blk_data + BLK_SIZE - DIND_MAX_OFFSET * sizeof(uint32_t);

	/* we just need doube indirect ptr */
	/* and video file are smaller than 1G */
	if (memcmp(p, zero_buf, BLK_SIZE - DIND_MAX_OFFSET * sizeof(uint32_t))) {
		return -1;
	}

	for (i = 0; i < BLK_SIZE / sizeof(uint32_t); i ++) {
		/* maybe file is too small */
		if (*p_num == 0 && i >= DIND_MIN_OFFSET) {
			break;
		} else if (*p_num == 0 && i <= DIND_MIN_OFFSET) {
			//printf("Not Valid: Too small i: %d, pre_num %d\n", i, pre_num);
			return -1;
		}

		/* exceeds max blocks */
		if (*p_num > MAX_BLOCKS) {
			return -1;
		}

		/* not implement */
		/* more smart algo */
		if (*p_num > pre_num) {
			/* not finish ... */
			good_cnt ++;
			cost = *p_num - pre_num;
		} else {
			bad_cnt ++;
		}

		/* not finish, need consider more info(good_cnt/cost) */
		if (bad_cnt > 10)
			return -1;

		pre_num = *p_num;
		p_num ++;
	}

	return 0;
}

static int __dump_dind_data(uint32_t blkno, int disk_fd, int file_fd)
{
	char addr_buf[BLK_SIZE];
	int ret, i;
	uint32_t *first_iblkno;
	char first_ibuf[BLK_SIZE];
	uint32_t *data_blkno;
	char data_buf[BLK_SIZE];

	//memset(blk_buf, 0, BLK_SIZE);
	/* dump 2nd index data */
	if (dump_block(blkno, addr_buf, disk_fd)) {
		printf("dump %u file, disk read secend index error, %s\n", blkno, strerror(errno));
		return -1;
	}

	first_iblkno = (uint32_t *)addr_buf;
	for (i = 0; i < DIND_MAX_OFFSET; i ++) {
		/* find valid 1st index */
		if (0 == *first_iblkno) {
			return 0;
		}

		/* dump 1st index data */
		printf("first blkno %u, %d\n", *first_iblkno, i);
		if (dump_block(*first_iblkno, first_ibuf, disk_fd)) {
			printf("dump %u file, disk read %u error, %s\n", blkno, *first_iblkno, strerror(errno));
			return -1;
		}

		data_blkno = (uint32_t *)first_ibuf;
		int n;
		for (n = 0; n < BLK_SIZE / sizeof(uint32_t); n ++) {
			//printf("data blkno %u, %d\n", *data_blkno, n);
			if (0 == n && 0 == *data_blkno) {
				/* break or return ? */
				return -1;
			} else if (0 == *data_blkno) {
				return 0;
			}

			/* exceeds max blocks */
			if (*data_blkno > MAX_BLOCKS) {
				return -1;
			}

			/* dump data */
			if (dump_block(*data_blkno, data_buf, disk_fd)) {
				printf("dump %u file, disk read %u error, %s\n", blkno, *data_blkno, strerror(errno));
				return -1;
			}

			ret = write(file_fd, data_buf, BLK_SIZE);
			if (BLK_SIZE != ret) {
				printf("dump %u file, file write error, %s\n", blkno, strerror(errno));
				return -1;
			}

			data_blkno ++;
		}

		first_iblkno ++;
	}

	return 0;
}

int dump_dind_data(uint32_t blkno, int disk_fd)
{
	char filename[256];
	int fd, ret;

	sprintf(filename, "%u_file.recovery", blkno);
	printf("DINT %u\n", blkno);

	fd = open(filename, O_CREAT | O_RDWR, 0666);
	if (fd < 0) {
		printf("dump %u file open error, %s\n", blkno, strerror(errno));
		return -1;
	}

	ret = __dump_dind_data(blkno, disk_fd, fd);

	close(fd);

	if (ret < 0) {
		if (0 == access(filename, F_OK)) {
			printf("%s is not a valid file. rm it\n", filename);
			remove(filename);
		}
	}

	return 0;
}

int get_block_data(int fd, char *buf, uint32_t start_blk_no)
{
	off64_t offset, ret;
	int len = 0;

	offset = (off64_t)start_blk_no * BLK_SIZE;
	ret = lseek64(fd, offset, SEEK_SET);
	if (ret < 0) {
		printf("%s llseek %ld error, %s\n", __func__, offset, strerror(errno));
		return -1;
	}
	//printf("seek at %ld, blkno %u\n", offset, start_blk_no);

	len = read(fd, buf, READ_BUFSIZE);
	if (len < 0) {
		printf("%s read error, %s\n", __func__, strerror(errno));
		return -1;
	}

	return len;
}

void *dump_file_thread(void *args)
{
	uint32_t blk_no;
	int empty;
	struct dump_thread_info *pinfo = (struct dump_thread_info *)args;
	struct dumpdind_queue *queue = pinfo->queue;
	int disk_fd = pinfo->disk_fd;

	printf("thread %d start\n", pinfo->thread_num);
	while (1) {
		pthread_mutex_lock(&queue->mutex);
		if (queue_empty(queue)) {
			empty = 1;
			if (is_finish == 1) {
				pthread_mutex_unlock(&queue->mutex);
				break;
			}
		} else {
			empty = 0;
			dequeue(queue, &blk_no);
		}
		if (queue_full(queue))
			pthread_cond_signal(&queue->cond);
		pthread_mutex_unlock(&queue->mutex);

		if (empty) {
			sleep(1);
		} else {
			printf("thread_num %d, now dumping %u DIND\n", pinfo->thread_num, blk_no);
			dump_dind_data(blk_no, disk_fd);
			printf("thread_num %d, finish dumping %u DIND\n", pinfo->thread_num, blk_no);
		}
	}
	printf("thread %d exit\n", pinfo->thread_num);

	return NULL;
}

int main(int argc, char **argv)
{
	int fd = 0, ret, i;
	char *buf = NULL, *p = NULL;
	uint32_t next_block = GROUP0_DATA_START;
	struct dump_thread_info dti[DUMP_THREAD_NUM];
	struct dumpdind_queue dindqueue;

	memset(zero_buf, 0, sizeof(zero_buf));
	memset(&dti, 0, sizeof(struct dump_thread_info));
	if (argc != 2) {
		printf("%s: <disk_name>\n", argv[0]);
		return -1;
	}
	memset(dindqueue.block_no, 0, sizeof(dindqueue.block_no));
	dindqueue.head = 0;
	dindqueue.tail = 0;
	dindqueue.size = 0;
	pthread_mutex_init(&dindqueue.mutex, NULL);
	pthread_cond_init(&dindqueue.cond, NULL);

	fd = open(argv[1], O_RDONLY | O_LARGEFILE);
	if (fd < 0) {
		printf("%s: open error. %s\n", argv[1], strerror(errno));
		return -1;
	}

	buf = malloc(READ_BUFSIZE);
	if (NULL == buf) {
		printf("malloc error\n");
		return -1;
	}
	memset(buf, 0, READ_BUFSIZE);

	for (i = 0; i < DUMP_THREAD_NUM; i ++) {
		dti[i].thread_num = i;
		dti[i].disk_fd = fd;
		dti[i].queue = &dindqueue;

		ret = pthread_create(&dti[i].thread_id, NULL, dump_file_thread, &dti[i]);
		if (ret < 0) {
			printf("create thread %d error, %s\n", i, strerror(errno));
			return -1;
		}
	}

	while (1) {
		ret = get_block_data(fd, buf, next_block);
		if (ret < 0) {
			printf("%s read error, %s\n", __func__, strerror(errno));
			return -1;
		}

		for (i = 0; i < (ret / BLK_SIZE); i ++) {
			p = buf + i * BLK_SIZE;
			//printf("guess block %u, p 0x%x\n", next_block, *p);
			if (0 == guess_dind_data(p)) {
				printf("find a DIND %u\n", next_block);
				pthread_mutex_lock(&dindqueue.mutex);
				if (queue_full(&dindqueue)) {
					pthread_cond_wait(&dindqueue.cond, &dindqueue.mutex);
					enqueue(&dindqueue, next_block);
				} else {
					enqueue(&dindqueue, next_block);
				}
				pthread_mutex_unlock(&dindqueue.mutex);
			}
			next_block ++;

			if (! (next_block % CHECK_POINT)) {
				printf("Now Scan Block %u\n", next_block);
			}

			if (next_block > GROUP0_DATA_END)
				break;
		}

		if (next_block > GROUP0_DATA_END) {
			is_finish = 1;
			printf("\nScan IND_DATA Finish!!\n");
			break;
		}
	}

	for (i = 0; i < DUMP_THREAD_NUM; i ++) {
		pthread_join(dti[i].thread_id, NULL);
	}

	close(fd);

	return 0;
}
