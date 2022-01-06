/*
 * ionapp_export.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdint.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "dma-buf.h"
#include "dma-heap.h"

#include "dmabuf_utils.h"
#include "ipcsocket.h"

#define DEVPATH "/dev/dma_heap/system"

static int dmabuf_heap_open(char *name)
{
	int fd;

	fd = open(name, O_RDWR);
	if (fd < 0)
		printf("open %s failed!\n", name);
	return fd;
}

static int dmabuf_heap_alloc_fdflags(int fd, size_t len, unsigned int fd_flags,
				     unsigned int heap_flags, int *dmabuf_fd)
{
	struct dma_heap_allocation_data data = {
		.len = len,
		.fd = 0,
		.fd_flags = fd_flags,
		.heap_flags = heap_flags,
	};
	int ret;

	if (!dmabuf_fd)
		return -EINVAL;

	ret = ioctl(fd, DMA_HEAP_IOCTL_ALLOC, &data);
	if (ret < 0)
		return ret;
	*dmabuf_fd = (int)data.fd;
	return ret;
}

static int dmabuf_heap_alloc(int fd, size_t len, unsigned int flags,
			     int *dmabuf_fd)
{
	return dmabuf_heap_alloc_fdflags(fd, len, O_RDWR | O_CLOEXEC, flags,
					 dmabuf_fd);
}

static int dmabuf_sync(int fd, int start_stop)
{
	struct dma_buf_sync sync = {
		.flags = start_stop | DMA_BUF_SYNC_RW,
	};

	return ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
}

void print_usage(int argc, char *argv[])
{
	printf("Usage: %s [-h <help>] [-i <heap id>] [-s <size in bytes>]\n",
		argv[0]);
}

int main(int argc, char *argv[])
{
	int ret, status;
	int sockfd;
	struct socket_info skinfo;
	int heap_fd = -1, dmabuf_fd = -1;
	void *p = NULL;

	int map_len = ONE_MEG;

	heap_fd = dmabuf_heap_open(DEVPATH);
	if (heap_fd < 0)
		return -1;

	ret = dmabuf_heap_alloc(heap_fd, map_len, 0, &dmabuf_fd);
	if (ret) {
		printf("FAIL (Allocation Failed!)\n");
		ret = -1;
		goto out;
	}

		/* mmap and write a simple pattern */
	p = mmap(NULL,
		 map_len,
		 PROT_READ | PROT_WRITE,
		 MAP_SHARED,
		 dmabuf_fd,
		 0);
	if (p == MAP_FAILED) {
		printf("FAIL (mmap() failed)\n");
		ret = -1;
		goto out;
	}

	dmabuf_sync(dmabuf_fd, DMA_BUF_SYNC_START);
	write_buffer_exporter(p, map_len);
	dmabuf_sync(dmabuf_fd, DMA_BUF_SYNC_END);

	/* This is server: open the socket connection first */
	/* Here; 1 indicates server or exporter */
	status = opensocket(&sockfd, SOCKET_NAME, 1);
	if (status < 0) {
		fprintf(stderr, "<%s>: Failed opensocket.\n", __func__);
		goto err_socket;
	}
	skinfo.sockfd = sockfd;

	/* share ion buf fd with other user process */
	printf("exporter: Sharing fd: %d\n", dmabuf_fd);
	skinfo.datafd = dmabuf_fd;

	ret = socket_send_fd(&skinfo);
	if (ret < 0) {
		fprintf(stderr, "FAILED: socket_send_fd\n");
		goto err_send;
	}

err_send:
err_socket:
	closesocket(sockfd, SOCKET_NAME);
out:
	if (p)
	    munmap(p, map_len);
	if (dmabuf_fd >= 0)
		close(dmabuf_fd);
	if (heap_fd >= 0)
		close(heap_fd);

	return 0;
}
