#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t lock;

double timeout = 0.00001;
ev_timer timeout_watcher;
int timeout_count = 0;

ev_async async_watcher;
int async_count = 0;

struct ev_loop * thread_loop = NULL;

void * thread_func(void * args)
{
	printf("Inside thread_func\n");

	ev_run(thread_loop, 0);

	return NULL;
}

static void async_cb(EV_P_ ev_async * w, int revents)
{
	pthread_mutex_lock(&lock);

	++async_count;

	printf("async = %d, timeout = %d\n", async_count, timeout_count);

	pthread_mutex_unlock(&lock);
}

static void timeout_cb(EV_P_ ev_timer * w, int revents)
{
	if (0 == ev_async_pending(&async_watcher))
	{
		ev_async_send(thread_loop, &async_watcher);
	}

	pthread_mutex_lock(&lock);

	++timeout_count;

	pthread_mutex_unlock(&lock);

	w->repeat = timeout;
	ev_timer_again(loop, &timeout_watcher);
}

int main(int argc, char * argv[])
{
	if (argc != 2)
	{
		printf("Usage: %s <timeout>\n", argv[0]);
		return -1;
	}

	timeout = atof(argv[1]);

	struct ev_loop * loop = EV_DEFAULT;

	// initialize pthread
	pthread_mutex_init(&lock, NULL);
	pthread_t thread;

	// this loop seats in the thread
	thread_loop = ev_loop_new(0);

	ev_async_init(&async_watcher, async_cb);
	ev_async_start(thread_loop, &async_watcher);
	pthread_create(&thread, NULL, thread_func, NULL);

	ev_timer_init(&timeout_watcher, timeout_cb, timeout, 0.);
	ev_timer_start(loop, &timeout_watcher);

	// now wait for events
	ev_run(loop, 0);

	pthread_join(thread, NULL);

	pthread_mutex_destroy(&lock);

	return 0;
}

