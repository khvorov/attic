// Simple echo server implementation with thread pool which serves read requests
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <ev.h>
#include <pthread.h>

#define ECHO_SERVER_PORT 3033
#define BUFFER_SIZE 1024

#define THREADS_COUNT 4

struct inner_worker_t
{
	struct ev_loop * loop;
    struct ev_async asw;
	unsigned int fdsNo;
};

struct worker_t
{
	struct inner_worker_t iw;
	char padding[CACHE_LINE_SIZE - sizeof(struct inner_worker_t)];
};

struct worker_t workers[THREADS_COUNT];
pthread_spinlock_t workersLock;

struct watcher_data_t
{
    struct ev_io io;
    struct worker_t * worker;
};

void * worker_function(void * arg)
{
	if (NULL == arg)
	{
		return NULL;
	}

	struct worker_t * wd = (struct worker_t *) arg;

    printf("worker thread [%p] started\n", (void *) wd);

	ev_run(wd->iw.loop, 0);

    printf("leaving worker thread [%p]\n", (void *) wd);

	return NULL;
}

void setnonblock(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);
}

static void async_cb(EV_P_ ev_async * w, int revents)
{
    // doing nothing
}

void read_cb(struct ev_loop * loop, struct ev_io * watcher, int revents)
{
    struct worker_t * worker = ((struct watcher_data_t *) watcher)->worker;
	char buffer[BUFFER_SIZE];
	int bytes_received;

	if (EV_ERROR & revents)
	{
		perror("got invalid event");
		return;
	}

	bytes_received = recv(watcher->fd, buffer, BUFFER_SIZE, 0);

	if (bytes_received < 0)
	{
		perror("read error");
		return;
	}

	if (bytes_received == 0)
	{
		// decreasing number of connections
		int fdsNo;

		pthread_spin_lock(&workersLock);
        fdsNo = --(worker->iw.fdsNo);
		pthread_spin_unlock(&workersLock);

        char msg[256];
        snprintf(msg, 256, "peer with fd[%d] closed connection, worker[%p], fdsNo[%d]", watcher->fd, (void *) worker, fdsNo);

        // cleaning up resources
		ev_io_stop(loop, watcher);
		close(watcher->fd);
		free(watcher);

		perror(msg);
		return;
	}

	if (bytes_received < BUFFER_SIZE)
	{
		buffer[bytes_received] = '\0';
	}

	printf("received %d bytes from fd[%d], message: %s\n", bytes_received, watcher->fd, buffer);

	// send message back
	send(watcher->fd, buffer, bytes_received, 0);
}

void accept_cb(struct ev_loop * loop, struct ev_io * watcher, int revents)
{
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_fd;
    struct watcher_data_t * w_client = (struct watcher_data_t *) malloc(sizeof(struct watcher_data_t));

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

    printf("accepting new client...\n");

	// finding a thread with minimal number of client connections
	unsigned int fdsNo = UINT_MAX;
	int threadNumber = 0;

	// TODO: think about spinlock replacement
	pthread_spin_lock(&workersLock);
	
	for (int i = 0; i < THREADS_COUNT; ++i)
	{
		if (workers[i].iw.fdsNo < fdsNo)
		{
			fdsNo = workers[i].iw.fdsNo;
			threadNumber = i;
		}
	}

	fdsNo = ++workers[threadNumber].iw.fdsNo;

	pthread_spin_unlock(&workersLock);

	printf("client connected, fd[%d], worker id = %d [%p], fdsNo = %d\n", client_fd, threadNumber, (void *) (workers + threadNumber), fdsNo);

	// initialize and start watcher to read client requests
    w_client->worker = &workers[threadNumber];

	ev_io_init(&(w_client->io), read_cb, client_fd, EV_READ);
	ev_io_start(w_client->worker->iw.loop, &(w_client->io));

    // wake up thread
    if (0 == ev_async_pending(&w_client->worker->iw.asw))
    {
        ev_async_send(w_client->worker->iw.loop, &w_client->worker->iw.asw);
    }
}

int main(int argc, char * argv[])
{
	struct ev_loop * loop = EV_DEFAULT;
	struct ev_io w_accept;
	struct sockaddr_in addr;
	int fd, reuse = 1;

    // initializing spin lock
    pthread_spin_init(&workersLock, PTHREAD_PROCESS_PRIVATE);

    // initializing worker threads
    pthread_t threads[THREADS_COUNT];

    for (int i = 0; i < THREADS_COUNT; ++i)
    {
        memset(&workers[i], 0, sizeof(struct worker_t));

        workers[i].iw.loop = ev_loop_new(0);

        ev_async_init(&workers[i].iw.asw, async_cb);
        ev_async_start(workers[i].iw.loop, &workers[i].iw.asw);

        pthread_create(&threads[i], NULL, worker_function, (void *) &workers[i]);
    }
    
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

	if (listen(fd, 1024) < 0)
	{
		perror("listen error");

		return -1;
	}

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	setnonblock(fd);

	// initializing watcher to accept client requests
	ev_io_init(&w_accept, accept_cb, fd, EV_READ);
	ev_io_start(loop, &w_accept);

    printf("starting main event loop...\n");

	ev_run(loop, 0);

    // waiting for all threads
    for (int i = 0; i < THREADS_COUNT; ++i)
    {
        pthread_join(threads[i], NULL);
    }

	return 0;
}

