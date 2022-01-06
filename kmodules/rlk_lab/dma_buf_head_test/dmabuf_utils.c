#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
//#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "dmabuf_utils.h"
#include "ipcsocket.h"


void write_buffer_exporter(void *buffer, unsigned long len)
{
	char *ptr = (char *)buffer;

	if (!ptr) {
		fprintf(stderr, "<%s>: Invalid buffer...\n", __func__);
		return;
	}

	strcpy(ptr, "come from export\n");
}

void write_buffer_importer(void *buffer, unsigned long len)
{
	char *ptr = (char *)buffer;

	if (!ptr) {
		fprintf(stderr, "<%s>: Invalid buffer...\n", __func__);
		return;
	}

	strcpy(ptr, "come from importer\n");
}

void read_buffer(void *buffer, unsigned long len)
{
	unsigned char *ptr = (unsigned char *)buffer;

	if (!ptr) {
		fprintf(stderr, "<%s>: Invalid buffer...\n", __func__);
		return;
	}

	printf("%s\n", ptr);
	printf("\n");
}

int socket_send_fd(struct socket_info *info)
{
	int status;
	int fd, sockfd;
	struct socketdata skdata;

	if (!info) {
		fprintf(stderr, "<%s>: Invalid socket info\n", __func__);
		return -1;
	}

	sockfd = info->sockfd;
	fd = info->datafd;
	memset(&skdata, 0, sizeof(skdata));
	skdata.data = fd;
	skdata.len = sizeof(skdata.data);
	status = sendtosocket(sockfd, &skdata);
	if (status < 0) {
		fprintf(stderr, "<%s>: Failed: sendtosocket\n", __func__);
		return -1;
	}

	return 0;
}

int socket_receive_fd(struct socket_info *info)
{
	int status;
	int fd, sockfd;
	struct socketdata skdata;

	if (!info) {
		fprintf(stderr, "<%s>: Invalid socket info\n", __func__);
		return -1;
	}

	sockfd = info->sockfd;
	memset(&skdata, 0, sizeof(skdata));
	status = receivefromsocket(sockfd, &skdata);
	if (status < 0) {
		fprintf(stderr, "<%s>: Failed: receivefromsocket\n", __func__);
		return -1;
	}

	fd = (int)skdata.data;
	info->datafd = fd;

	return status;
}
