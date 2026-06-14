#include "thread_pool.h"
#include <stdio.h>
#include <stdlib.h>

void example_task(void *arg) {
	int *num = (int *)arg;
	printf("Processing task %d\n", *num);
	free(arg);
}

int main() {
	thread_pool_t pool;
	if (thread_pool_init(&pool, 8, 100) != 0) {
		printf("Failed to initialize pool\n");
		exit(-1);
	}

	for (int i = 0; i < 300; ++i) {
		int *task_num = malloc(sizeof(int));
		*task_num = i;
		thread_pool_add_task(&pool, example_task, task_num);
		// what if we free task_num in this line
	}
	thread_pool_destroy(&pool);
}



