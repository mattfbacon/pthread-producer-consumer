#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <pthread.h>

#define BUF_SIZE 20

static char data[14] = "Hello, world!\n";

static volatile bool finished = false;

static volatile char event_buffer[BUF_SIZE] = { 0, };
static volatile ptrdiff_t start = 0;
static volatile ptrdiff_t end = 0;

static pthread_mutex_t buffer_lock;

int buffer_push(const char val) {
	pthread_mutex_lock(&buffer_lock);
	event_buffer[start] = val;
	end = (end + 1) % BUF_SIZE;
	if (start == end) {
		pthread_mutex_unlock(&buffer_lock);
		return 1;
	}
	pthread_mutex_unlock(&buffer_lock);
	return 0;
}

int buffer_pop(char*const out) {
	pthread_mutex_lock(&buffer_lock);
	if ((end - start) % BUF_SIZE == 0) {
		/* fprintf(stderr, "%ld %ld %ld\n", start, end, (end - start) % BUF_SIZE); */
		pthread_mutex_unlock(&buffer_lock);
		return 1;
	}
	const char val = event_buffer[start];
	start = (start + 1) % BUF_SIZE;
	pthread_mutex_unlock(&buffer_lock);
	*out = val;
	return 0;
}

int buffer_size() {
	pthread_mutex_lock(&buffer_lock);
	return (end - start) % BUF_SIZE;
	pthread_mutex_unlock(&buffer_lock);
}

void* thread_function(void*) {
	fputs("Thread start\n", stderr);
	for (size_t i = 0; i < sizeof(data); i++) {
		fputs("Push\n", stderr);
		if(buffer_push(data[i]) != 0) {
			fputs("Buffer overrun\n", stderr);
		}
		sleep(1);
	}
	finished = true;
	return NULL;
}

int main() {
	if (pthread_mutex_init(&buffer_lock, NULL) != 0) {
		fputs("Mutex Init Failed\n", stderr);
		return 1;
	}
	pthread_t producer_thread;
	fputs("Start\n", stderr);
	if (pthread_create(&producer_thread, NULL, thread_function, NULL)) {
		fputs("Thread Init Failed\n", stderr);
		return 1;
	}
	char event;
	while (!finished) {
		if (buffer_pop(&event) != 0) {
			sched_yield();
			continue;
		}
		fputs("Pop\n", stderr);
		putchar(event);
		fflush(stdout);
	}
	pthread_mutex_destroy(&buffer_lock);
	pthread_join(producer_thread, NULL);
	return 0;
}
