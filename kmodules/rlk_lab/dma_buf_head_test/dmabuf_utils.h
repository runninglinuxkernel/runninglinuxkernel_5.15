#ifndef __ION_UTILS_H
#define __ION_UTILS_H

#define SOCKET_NAME "ion_socket"

#define ONE_MEG (4096)

struct socket_info {
	int sockfd;
	int datafd;
	unsigned long buflen;
};

/* This is used to fill the data into the mapped buffer */
void write_buffer_exporter(void *buffer, unsigned long len);
void write_buffer_importer(void *buffer, unsigned long len);

/* This is used to read the data from the exported buffer */
void read_buffer(void *buffer, unsigned long len);

/* This is used to send FD to another process using socket IPC */
int socket_send_fd(struct socket_info *skinfo);

/* This is used to receive FD from another process using socket IPC */
int socket_receive_fd(struct socket_info *skinfo);
#endif
