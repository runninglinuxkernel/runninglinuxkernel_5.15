/*
 * ionapp_import.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "dmabuf_utils.h"
#include "ipcsocket.h"

int main(void)
{
	int ret, status;
	int sockfd, shared_fd;
	unsigned char *map_buf;
	struct socket_info skinfo;
	int map_size = ONE_MEG; 

	/* This is the client part. Here 0 means client or importer */
	status = opensocket(&sockfd, SOCKET_NAME, 0);
	if (status < 0) {
		fprintf(stderr, "No exporter exists...\n");
		ret = status;
		goto err_socket;
	}

	skinfo.sockfd = sockfd;

	ret = socket_receive_fd(&skinfo);
	if (ret < 0) {
		fprintf(stderr, "Failed: socket_receive_fd\n");
		goto err_recv;
	}

	shared_fd = skinfo.datafd;
	printf("importer: Received buffer fd: %d\n", shared_fd);
	if (shared_fd <= 0) {
		fprintf(stderr, "ERROR: improper buf fd\n");
		ret = -1;
		goto err_fd;
	}

	/* mmap and write a simple pattern */
	map_buf = mmap(NULL,
		 map_size,
		 PROT_READ | PROT_WRITE,
		 MAP_SHARED,
		 shared_fd,
		 0);
	if (map_buf == MAP_FAILED) {
		printf("FAIL (mmap() failed)\n");
		ret = -1;
		goto out;
	}

	printf("importer: read data:\n");
	read_buffer(map_buf, map_size);

out:
	if (map_buf)
		munmap(map_buf, map_size);
err_fd:
err_recv:
err_socket:
	closesocket(sockfd, SOCKET_NAME);

	return ret;
}
