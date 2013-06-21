#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#define TDRV_IOC_MAGIC 'k'
#define TDRV_IOC_SETTIME _IOW(TDRV_IOC_MAGIC, 1, int)
#define BUF_SIZE 20

int write_buf(int fd, char *buf)
{
	ssize_t size;

	size = lseek(fd, SEEK_SET, 0);
	if (size < 0) {
		printf("write error, %s\n", strerror(errno));
	}
	size = write(fd, buf, BUF_SIZE);
	if (size < 0) {
		printf("write error, %s\n", strerror(errno));
		exit(1);
	} else if (size != BUF_SIZE)
		printf("write %d bytes\n", size);
	return 0;
}

int read_buf(int fd, char *buf)
{
	ssize_t size;

	memset(buf, 0, BUF_SIZE);
	size = lseek(fd, SEEK_SET, 0);
	size = read(fd, buf, BUF_SIZE);
	if (size < 0) {
		printf("read error, %s\n", strerror(errno));
		exit(1);
	}

	return 0;
}

int main()
{
	char buf[BUF_SIZE];
	int fd, ret, interval;

	interval = 5;
	fd = open("tdrv", O_RDWR);

	strncpy(buf, "fill bufaaaaaaaaaaaa", BUF_SIZE - 1);
	write_buf(fd, buf);
	printf("write to tdrv, data is \"%s\"\n", buf);

	ret = ioctl(fd, TDRV_IOC_SETTIME, &interval);
	if (ret) {
		printf("ioctl error, %s\n", strerror(errno));
		exit(1);
	}
	printf("set %d seconds to clean buffer\n", interval);

	read_buf(fd, buf);
	printf("read tdrv immadately, data is \"%s\"\n", buf);

	sleep(interval);

	read_buf(fd, buf);
	if (!strlen(buf))
		printf("after %d seconds then read tdrv, data is \"NULL\"\n", interval);
	else
		printf("after %d seconds then read tdrv, data is \"%s\"\n", interval, buf);

	return 0;
}
