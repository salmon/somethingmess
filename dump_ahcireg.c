#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define IOPORTS_PATH		"/proc/ioports"
#define IOMEM_PATH	"/proc/iomem"
#define BUF_LEN	512

struct iobases {
	unsigned long long iomem_start;
	unsigned long long iomem_end;
};

struct ahci_link {
	struct iobases iomem;
	struct ahci_link *next;
};

int find_ahci_line(char *line)
{
	unsigned int idx = 0;
	int i = 0;
	while(line[idx++]) {
		switch (i) {
		case 0:
			if (line[idx] == 'a') {
				i = 1;
			}
			break;
		case 1:
			if (line[idx] == 'h') {
				i = 2;
			} else {
				i = 0;
			}
			break;
		case 2:
			if (line[idx] == 'c') {
				i = 3;
			} else {
				i = 0;
			}
			break;
		case 3:
			if (line[idx] == 'i') {
				return 1;
			} else {
				i = 0;
			}
			break;
		default:
			printf("state mechine error\n");
			exit(1);
			break;
		}
	}
	return 0;
}

void parse_line(char *buf, struct ahci_link *link)
{
	int i = 0;
	unsigned long long tmp_start = 0;
	unsigned long long tmp_end = 0;
	int flag = 0;

	struct ahci_link *ahci_one = malloc(sizeof(struct ahci_link));
	ahci_one->iomem.iomem_start = 0;
	ahci_one->iomem.iomem_end = 0;
	ahci_one->next = NULL;

	while (buf[i]) {
		if (buf[i]>='0' && buf[i]<='9') {
			if (flag == 0) {
				tmp_start <<= 4;
				tmp_start += (buf[i] - '0');
			} else {
				tmp_end <<= 4;
				tmp_end += (buf[i] - '0');
			}
		} else if (buf[i]>='a' && buf[i]<='f') {
			if (flag == 0) {
				tmp_start <<= 4;
				tmp_start += (buf[i]-'a'+10);
			} else {
				tmp_end <<= 4;
				tmp_end += (buf[i]-'a'+10);
			}
		} else if (buf[i] == '-') {
			flag = 1;
		} else if (flag == 1) {
			break;
		}
		i++;
	}

	ahci_one->iomem.iomem_start = tmp_start;
	ahci_one->iomem.iomem_end = tmp_end;

	while (link->next);
	link->next = ahci_one;
}

int getbaseaddr(struct ahci_link *link)
{
	int fd;
	off_t cur;
	char *buf;
	int idx;
	ssize_t rsize;

	buf = malloc(BUF_LEN);
	if (NULL == buf) {
		printf("malloc error\n");
		exit(1);
	}
	fd = open(IOMEM_PATH, O_RDONLY);
	if (fd < 0) {
		printf("open %s error\n", IOMEM_PATH);
		free(buf);
		exit(1);
	}
	while (1) {
		cur = lseek(fd, 0, SEEK_CUR);
		idx = 0;
		rsize = read(fd, buf, BUF_LEN);
		if(rsize <= 0) {
			break;
		}
		while (buf[idx++] != '\n' && idx < rsize);
		if (idx < rsize) {
			lseek(fd, cur+idx, SEEK_SET);
			buf[idx] = 0;
		}
		if (find_ahci_line(buf)) {
			parse_line(buf, link);
		}
	}
	free(buf);
}

int main()
{
	struct ahci_link head;
	struct ahci_link *tmp;
	unsigned long long tmp_start=0, tmp_end=0;

	head.next = NULL;
	getbaseaddr(&head);
	if (head.next == NULL) {
		printf("can't find ahci controller\n");
		return 1;
	}
	tmp = &head;
	while (1) {
		tmp = tmp->next;
		tmp_start = tmp->iomem.iomem_start;
		tmp_end = tmp->iomem.iomem_end;
		printf("ahci start %lx end %lx\n", tmp_start, tmp_end);
		if (!tmp->next) {
			break;
		}
	}
	return 0;
}
