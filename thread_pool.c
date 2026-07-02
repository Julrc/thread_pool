#include "thread_pool.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

// thread_pool initialization
//
// Create thread_pool structure
// Create taskqueue structure
// Mutex for thread_pool access
// Condition variable to start a thread
//
//Design thread array, task queue, synchronization structures
//


static void *thread_function(void *thread_pool) {
	thread_pool_t* pool = (thread_pool_t *)thread_pool;
	void *thread_ctx = pool->on_thread_start ? pool->on_thread_start() : NULL;

	while (1) {
		pthread_mutex_lock(&(pool->lock));

		while (pool->queued == 0 && !pool->stop) {
			pthread_cond_wait(&(pool->signal), &(pool->lock));
		}

		if (pool->stop && pool->queued == 0) {
			if (pool->on_thread_stop)
			{
				(*pool->on_thread_stop)(thread_ctx);
			}
			pthread_mutex_unlock(&(pool->lock));
			pthread_exit(NULL);
		}
		// deque 
		task_t task = pool->task_queue[pool->queue_front];
		pool->queue_front = (pool->queue_front + 1) % pool->QUEUE_SIZE;
		pool->queued--;
		pthread_cond_signal(&(pool->finished_signal));
		pthread_mutex_unlock(&(pool->lock));
		(*(task.fn))(task.arg, thread_ctx);
	}
	return NULL;
}

void thread_pool_destroy(thread_pool_t *pool) {
	pthread_mutex_lock(&(pool->lock));
	pool->stop = 1;
	pthread_cond_broadcast(&(pool->signal));
	pthread_cond_broadcast(&(pool->finished_signal));
	pthread_mutex_unlock(&(pool->lock));

	for (int i = 0; i < pool->THREAD_COUNT; ++i) {
		pthread_join(pool->threads[i], NULL);
	}

	pthread_mutex_destroy(&(pool->lock));
	pthread_cond_destroy(&(pool->signal));
	pthread_cond_destroy(&(pool->finished_signal));
	free(pool->threads);
	free(pool->task_queue);
}

int thread_pool_init(thread_pool_t* pool, int th_count, int q_sz, void*(*on_start)(void), void (*on_stop)(void *)) {

	pool->on_thread_start = on_start;
	pool->on_thread_stop = on_stop;
	pool->QUEUE_SIZE = q_sz;
	pool->THREAD_COUNT = th_count;
	pool->queued = 0;
	pool->queue_front = 0;
	pool->queue_back = 0;
	pool->stop = 0;

	pool->threads = malloc(sizeof(pthread_t) * th_count);
	pool->task_queue = malloc(sizeof(task_t) * q_sz);

	if (pool->threads == NULL)  {
		return 1;
	}

	if (pool->task_queue == NULL) {
		free(pool->threads); return 2;
	}

	pthread_mutex_init(&(pool->lock), NULL);
	pthread_cond_init(&(pool->signal), NULL);
	pthread_cond_init(&(pool->finished_signal), NULL);

	int status, i;
	for (i = 0; i < pool->THREAD_COUNT; ++i) {
		status = pthread_create(&(pool->threads[i]), NULL, thread_function, pool);

		if (status != 0) {
			pool->THREAD_COUNT = i;
			thread_pool_destroy(pool);
			return 3;
		}
	}

	return 0;
}

void thread_pool_add_task(thread_pool_t *pool, void(*function)(void *, void *), void *arg) {
	pthread_mutex_lock(&(pool->lock));
	// sleep thread until other task opens up space in q
	while (pool->queued >= pool->QUEUE_SIZE && !pool->stop) {
		pthread_cond_wait(&(pool->finished_signal), &(pool->lock));
	}
	if (pool->stop) {
		pthread_mutex_unlock(&(pool->lock));
		return; // back to caller
	}
	int next_rear = (pool->queue_back + 1) % pool->QUEUE_SIZE;
	pool->task_queue[pool->queue_back].fn = function;
	pool->task_queue[pool->queue_back].arg = arg;
	pool->queue_back = next_rear;
	pool->queued++;
	pthread_cond_signal(&(pool->signal));

	pthread_mutex_unlock(&(pool->lock));
}

