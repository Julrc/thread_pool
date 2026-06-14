all:
	gcc -Wall -Wextra -pthread -o thread_pool main.c thread_pool.c
clean:
	rm -f thread_pool
