// Simple echo server implementation from that blog post:
// http://codefundas.blogspot.sg/2010/09/create-tcp-echo-server-using-libev.html
// Compile: gcc -Wall -pedantic -std=c99 -o echo_server{,.c} -lev
// Test: connect by telnet to the port 3033

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <ev.h>

#define ECHO_SERVER_PORT 3033
#define BUFFER_SIZE 1024

void setnonblock(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);
}

void read_cb(struct ev_loop * loop, struct ev_io * watcher, int revents)
{
	char buffer[BUFFER_SIZE];
	int read;

	if (EV_ERROR & revents)
	{
		perror("got invalid event");
		return;
	}

	read = recv(watcher->fd, buffer, BUFFER_SIZE, 0);

	if (read < 0)
	{
		perror("read error");
		return;
	}

	if (read == 0)
	{
		ev_io_stop(loop, watcher);
		close(watcher->fd);
		free(watcher);
		perror("peer closed connection");
		return;
	}

	if (read < BUFFER_SIZE)
	{
		buffer[read] = '\0';
	}

	printf("received %d bytes from fd[%d], message: %s\n", read, watcher->fd, buffer);

	// send message back
	send(watcher->fd, buffer, read, 0);
}

void accept_cb(struct ev_loop * loop, struct ev_io * watcher, int revents)
{
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_fd;
	struct ev_io * w_client = (struct ev_io *) malloc(sizeof(struct ev_io));

	if (EV_ERROR & revents)
	{
		perror("got invalid event");
		return;
	}

	// accept client request
	client_fd = accept(watcher->fd, (struct sockaddr *) &client_addr, &client_len);

	if (client_fd < 0)
	{
		perror("accept error");

		return;
	}

	printf("Successfully connected with client, fd[%d]\n", client_fd);

	// initialize and start watcher to read client requests
	ev_io_init(w_client, read_cb, client_fd, EV_READ);
	ev_io_start(loop, w_client);
}

int main(int argc, char * argv[])
{
	struct ev_loop * loop = ev_default_loop(0);
	struct ev_io w_accept;
	struct sockaddr_in addr;
	int fd, reuse = 1;

	// create server socket
	if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket error");

		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(ECHO_SERVER_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) != 0)
	{
		perror("bind error");

		return -1;
	}

	if (listen(fd, 5) < 0)
	{
		perror("listen error");

		return -1;
	}

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	setnonblock(fd);

	// initializing watcher to accept client requests
	ev_io_init(&w_accept, accept_cb, fd, EV_READ);
	ev_io_start(loop, &w_accept);

	ev_loop(loop, 0);

	return 0;
}

