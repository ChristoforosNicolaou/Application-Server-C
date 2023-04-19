#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "helper_functions.h"
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
		
		int r = rand() % 5 + 1;
		sleep(1 * r);
	}
	
	pthread_exit(NULL);
}

// Configure server parameters
int configure_parameters(char *filename, int *num_threads, int *port, char **home_directory, char **server_name)
{
	int INPUT_BUF_SIZE = 512;
	int MAX_TOKENS = 64;
	
	int set_num_threads = 0, set_port = 0, set_home_directory = 0, set_server_name = 0;
	
	char input_buf[INPUT_BUF_SIZE];
	
	FILE *fp = NULL;
	if ((fp = fopen(filename, "r")) == NULL)
	{
		perror("fopen");
		return -1;
	}
	
	// Read file line by line
	while (fgets(input_buf, sizeof(input_buf), fp) != NULL)
	{
		char *tokens[MAX_TOKENS];
		int num_tokens;
		if ((num_tokens = tokenize(input_buf, "=\r\n\t ", tokens)) < 0)
		{
			return -1;
		}
		
		// Set token
		if (num_tokens >= 2)
		{
			if (strcmp(tokens[0], "THREADS") == 0)
			{
				*num_threads = atoi(tokens[1]);
				set_num_threads = 1;
			}
			else if (strcmp(tokens[0], "PORT") == 0)
			{
				*port = atoi(tokens[1]);
				set_port = 1;
			}
			else if(strcmp(tokens[0], "HOME") == 0)
			{
				*home_directory = (char *) malloc(strlen(tokens[1]) + 1);
				strcpy(*home_directory, tokens[1]);
				set_home_directory = 1;
			}
			else if(strcmp(tokens[0], "HOSTNAME") == 0)
			{
				*server_name = (char *) malloc(strlen(tokens[1]) + 1);
				strcpy(*server_name, tokens[1]);
				set_server_name = 1;
			}
		}
		
		// Free memory
		int i;
		for (i = 0; i < num_tokens; i++)
		{
			free(tokens[i]);
		}
	}
	
	if (!set_num_threads)
	{
		fprintf(stderr, "THREADS parameter not set!\n");
	}
	if (!set_port)
	{
		fprintf(stderr, "PORT parameter not set!\n");
	}
	if (!set_home_directory)
	{
		fprintf(stderr, "HOME parameter not set!\n");
	}
	if (!set_server_name)
	{
		fprintf(stderr, "HOSTNAME parameter not set!\n");
	}
	
	return (set_num_threads && set_port && set_home_directory && set_server_name > 0) ? 0 : -1;
}

int main(int argc, char **argv)
{
	char CONF_FILENAME[] = "config.txt";

	int num_threads = 1, port = 80; // Default
	char *home_directory;
	char *server_name;
	
	
	if (configure_parameters(CONF_FILENAME, &num_threads, &port, &home_directory, &server_name) < 0)
	{
		fprintf(stderr, "Unable to set configuration file parameters.\n");
		exit(1);
	}
	
	printf("Threads: %d\n", num_threads);
	printf("Port: %d\n", port);
	printf("Home: %s\n", home_directory);
	printf("Server: %s\n", server_name);
	
	// Free memory
	free(home_directory);
	free(server_name);
	
	exit(1);
	

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
