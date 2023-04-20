#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>


#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "helper_functions.h"
#include "queue.h"

//-------------------------Compile-------------------------//
//gcc -o test_server test_server.c helper_functions.c queue.c  -lssl -lcrypto -lpthread


#define perror2(s, e) fprintf(stderr, "%s: %s\n", s, strerror(e))
#define BUF_SIZE 512
#define MAXARGS 100
#define MAXHEADLINES 10

int num_threads = 1, port = 80; // Default
char *home_directory;
char *server_name;

//TLS
int sock;
SSL_CTX *ctx;

// Lock
pthread_mutex_t mutex;
pthread_cond_t cond;

// Shared queue
QUEUE *socket_queue;

void close_connection(){
	close(sock);
    SSL_CTX_free(ctx);
    cleanup_openssl();
}

void init_connection(int port,int max_requests){
	/* initialize OpenSSL */
    init_openssl();
    /* setting up algorithms needed by TLS */
    ctx = create_context();
    /* specify the certificate and private key to use */
    configure_context(ctx);
    sock = create_socket(port,max_requests);
	return;
}


void terminate_Ssl(SSL *ssl, int socket_fd){
	SSL_shutdown(ssl);
	/* free an allocated SSL structure */
	SSL_free(ssl);
	close(socket_fd);
	return;
}

void print_parsedRequest(char* parsedRequest[MAXHEADLINES][MAXARGS]){
	int i=0;
	int j=0;

	while(parsedRequest[i][0]!= NULL){
		while(parsedRequest[i][j]!= NULL){
			printf("|%s|",parsedRequest[i][j]);
			j++;
		}
		i++;
		j=0;
		printf("\n");
	}
	return;
}

// Thread worker
void *worker(void *arg)
{
	int threadId = *((int*)arg);
	while(1)
	{
		int socket_fd;

		pthread_mutex_lock(&mutex);
		// If queue is empty wait
		if (isEmpty(socket_queue))
		{
			//printf("Thread %d blocked as queue is empty\n", threadId);
			pthread_cond_wait(&cond, &mutex);
		}
		
		//Serve next in line
		if (dequeue(socket_queue, &socket_fd) != 0)
		{
			fprintf(stderr, "dequeue: queue is not initialized or is empty\n");
			exit(1);
		}

		pthread_mutex_unlock(&mutex);
		printf("Thread %d, Entry: %d\n", threadId , socket_fd);
		/* creates a new SSL structure which is needed to hold the data 
		* for a TLS/SSL connection
		*/ 
		SSL *ssl;
		int num_headlines=-1;
		int i=0;
		char buffer[BUF_SIZE];
		char* tokenizedRequest[MAXHEADLINES];
		char* parsedRequest[MAXHEADLINES][MAXARGS];

		ssl = SSL_new(ctx);
		SSL_set_fd(ssl, socket_fd);

		/* wait for a TLS/SSL client to initiate a TLS/SSL handshake */
		if (SSL_accept(ssl) <= 0) {
			ERR_print_errors_fp(stderr);
		}
		/* if TLS/SSL handshake was successfully completed, a TLS/SSL 
		* connection has been established
		*/
		else {
			SSL_read(ssl, buffer, BUF_SIZE);
			//printf("%s",buffer);

			if ((num_headlines = tokenize(buffer, "\r\n", tokenizedRequest)) < 0)
				{	
					printf("Tokenization Error: %d",socket_fd);
					terminate_Ssl(ssl,socket_fd);
				}
			
			for(i=0;i<num_headlines;i++){
				if (tokenize(tokenizedRequest[i], " ", parsedRequest[i]) < 0)
					{	
						printf("Tokenization Error: %d",socket_fd);
						terminate_Ssl(ssl,socket_fd);
					}
			}

			print_parsedRequest(parsedRequest);
			
			//const char reply[] = "test\n";
			//SSL_write(ssl, reply, strlen(reply));
		}

		printf("Socket connection %d closed",socket_fd);
		terminate_Ssl(ssl,socket_fd);

					
		//printf("Thread %d, Entry: %d\n", threadId , socket_fd);
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

	if (configure_parameters(CONF_FILENAME, &num_threads, &port, &home_directory, &server_name) < 0)
	{
		fprintf(stderr, "Unable to set configuration file parameters.\n");
		exit(1);
	}
	
	printf("Threads: %d\n", num_threads);
	printf("Port: %d\n", port);
	printf("Home: %s\n", home_directory);
	printf("Server: %s\n", server_name);

	pthread_t tid[num_threads];
	int i, err,threadId[100];

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);

	initQueue(&socket_queue);


	//Spawn threads
	for (i = 0; i < num_threads; i++) 
	{	
		threadId[i]=i;

		// Create threads
		if (err = pthread_create(&tid[i], NULL, &worker, &threadId[i])) 
		{
			perror2("pthread_create", err); 
			exit(1);
		}
	}
	
	//Start Listening
	init_connection(port,num_threads);
	//------------------------------------------------------------------------------

	/* Handle connections */
    while(1) {
        struct sockaddr_in addr;
        uint len = sizeof(addr);
	
		//Accept connection get socket descriptor
        int client = accept(sock, (struct sockaddr*)&addr, &len);
        if (client < 0) {
            perror("Unable to accept");
            exit(EXIT_FAILURE);
        }

		//Queue socket descriptor, wake up a thread
		enqueue(client, socket_queue);
		pthread_cond_signal(&cond);
    }

	//Stop Listening
	close_connection();
	//------------------------------------------------------------------------------

	/*
	// Free memory
	free(home_directory);
	free(server_name);
	
	exit(1);
	*/
	
	// Join threads
	for (i = 0; i < num_threads; i++) 
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
