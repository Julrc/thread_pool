#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>

typedef struct {
	void (*fn)(void* arg);
	void *arg;
} task_t;

typedef struct {
	int THREAD_COUNT;
	int QUEUE_SIZE;
	pthread_t *threads;
	pthread_mutex_t lock;
	pthread_cond_t signal;
	pthread_cond_t finished_signal;
	task_t *task_queue;
	int queued;
	int queue_front;
	int queue_back;
	int stop;
} thread_pool_t;

int thread_pool_init(thread_pool_t *pool, int th_count, int q_sz);

void thread_pool_add_task(thread_pool_t *pool, void (*function)(void *), void *arg);

void thread_pool_destroy(thread_pool_t *pool);

#endif
