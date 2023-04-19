#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "queue.h"

#define perror2(s, e) fprintf(stderr, "%s: %s\n", s, strerror(e))

#define NUM_THREADS 10

// Lock
pthread_mutex_t mutex;
pthread_cond_t cond;

// Shared queue
QUEUE *q;

// Thread worker
void *worker(/*void *arg*/)
{
	srand(time(NULL));
	while(1)
	{
		int entry;
		pthread_mutex_lock(&mutex);
		// If queue is empty wait
		if (isEmpty(q))
		{
			printf("Thread %ld blocked as queue is empty\n", pthread_self());
			pthread_cond_wait(&cond, &mutex);
		}
		
		// Unlock and continue
		if (dequeue(q, &entry) != 0)
		{
			fprintf(stderr, "dequeue: queue is not initialized or is empty\n");
			exit(1);
		}
		pthread_mutex_unlock(&mutex);
		
		printf("Thread %ld, Entry: %d\n", pthread_self(), entry);

		/*
		while (connection == keep-alive)
		{
			response
			wait for request
		}
		response with connection close
		*/

		int r = rand() % 5 + 1;
		sleep(1 * r);
	}
	
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);

	initQueue(&q);
	
	pthread_t tid[NUM_THREADS];
	int i, err;
	
	for (i = 0; i < 100; i++)
	{
		enqueue(i, q);
	}
	
	for (i = 0; i < NUM_THREADS; i++) 
	{
		//bounds[i].low = i*N;
		//bounds[i].high = bounds[i].low + N;
		//bounds[i].mysum = 0;
		
		// Create threads
		if (err = pthread_create(&tid[i], NULL, &worker, NULL/*(void *) &bounds[i]*/)) 
		{
			perror2("pthread_create", err); 
			exit(1);
		}
	}
	
	sleep(12);
	
	for (i = 0; i < 5; i++)
	{
		enqueue(i, q);
		pthread_cond_signal(&cond);
	}
	
	sleep(2);
	
	for (i = 0; i < 30; i++)
	{
		enqueue(i, q);
		pthread_cond_signal(&cond);
	}
	
	// Wait for threads
	for (i = 0; i < NUM_THREADS; i++) 
	{
		if (err = pthread_join(tid[i], NULL/*(void **) &status*/)) 
		{ /* wait for thread to end */
			perror2("pthread_join", err); 
			exit(1);
		}
	}
	
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
	
	return 0;
}
