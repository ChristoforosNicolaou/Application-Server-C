#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include<sys/wait.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "helper_functions.h"
#include "queue.h"

//-------------------------Compile-------------------------//
//gcc -o test_server test_server.c helper_functions.c queue.c  -lssl -lcrypto -lpthread -Wno-deprecated-declarations


#define perror2(s, e) fprintf(stderr, "%s: %s\n", s, strerror(e))
#define BUF_SIZE 512
#define MAXARGS 100
#define MAXHEADLINES 10
#define MAX_FILE_EXTENSIONS 13
#define MAX_REQUEST_METHODS 4
#define MAX_EXEC_TYPE 2

char CONF_FILENAME[] = "config.txt";
char *file_extensions[MAX_FILE_EXTENSIONS];
char *exec_type[MAX_EXEC_TYPE];
char *request_methods[MAX_REQUEST_METHODS];
char cwd[1024];
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

//Head reply struct
struct head_struct {
	int msg;
	int length;
	char* server;
	char* connection;
	char* type;
	char* body;
}; 


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

char* add_null_termination(char* buffer) {
    size_t length = strlen(buffer);
    char* new_buffer = malloc(length + 1);  
    if (new_buffer == NULL) {
        return NULL; 
    }
    memcpy(new_buffer, buffer, length);  
    new_buffer[length] = '\0'; 
    return new_buffer;
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

// Returns file content type based on extension
char *get_content_type(char *extension, char *file_extensions[MAX_FILE_EXTENSIONS])
{
	int i, index = -1;
	// Find file extension index
	for (i = 0; i < MAX_FILE_EXTENSIONS; i++)
	{
		if (strcmp(extension, file_extensions[i]) == 0)
		{
			index = i;
			break;
		}
	}
	
	// Get content type based on extension
	char *type;
	switch (index)
	{
		case 0: 
		case 1: 
		case 2: 
		case 3: 
		case 4:
			type = "text/plain";
			break;
		case 5: 
		case 6:
			type = "text/html";
			break;
		case 7:
			type = "text/x-php";
			break;
		case 8:
			type = "application/x-python-code";
			break;
		case 9:
		case 10:
			type = "image/jpeg";
			break;
		case 11:
			type = "image/gif";
			break;
		case 12:
			type = "application/pdf";
			break;
		default:
			type = "application/octet-stream";
			break;
	}
	
	//char *content_type;
	//content_type = (char *) malloc(strlen(type) + 1);
	//strcpy(content_type, type);
	return type;
}

// Returns path, file and content type of given path
void get_file_data(char *path, char *file_extensions[MAX_FILE_EXTENSIONS], char **path_only, char **file, char **content_type)
{
	int i;
	int index_ext = -1, index_file = -1;
	
	// Find where to split path, file and extension
	for (i = strlen(path) - 1; i >= 0; i--)
	{
		if (path[i] == '.' && index_ext < 0)
		{
			index_ext = i + 1;
		}
		
		else if (path[i] == '/')
		{
			index_file = i + 1;
			break;
		}
	}
	
	// Get path and file
	if (index_file < 0)
	{
		*path_only = (char *) malloc(2); strcpy(*path_only, ".");
		*file = substr(path, 0, strlen(path));
	}
	else
	{
		*path_only = substr(path, 0, index_file);
		*file = substr(path, index_file, strlen(path));
	}
	
	// Get extension
	if (index_ext < 0)
	{
		*content_type = get_content_type("", file_extensions);
	}
	else
	{
		char *ext = substr(path, index_ext, strlen(path));
		*content_type = get_content_type(ext, file_extensions);
		free(ext);
	}
}

// Read from file
char *readFile(char *filename, struct head_struct* replyStruct) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        return NULL;
    }

    // Determine the file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

	replyStruct->length=size;

    // Allocate memory for the buffer
    char *buffer = (char *) malloc(size);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    // Read the file into the buffer
    size_t bytesRead = fread(buffer, 1, size, file);
    if (bytesRead != size) {
        fclose(file);
        free(buffer);
        return NULL;
    }

	//Null Terminate
	buffer[size] = '\0';

    fclose(file);
    return buffer;
}


// Returns the server response string
char *get_response_string(struct head_struct *header, int *total_bytes)
{
	char *s_msg;

	// Get message from code
	switch (header->msg)
	{
		case 200:
			s_msg = "OK";
			break;
		case 404:
			s_msg = "Not Found";
			break;
		case 501:
		default:
			s_msg = "Not Implemented";
			break;
	}

	char *response;
	
	// If only header
	if (header->body == NULL)
	{
		response = (char *) malloc(200);
	}
	else
	{
		response = (char *) malloc(200 + header->length);
	}
	
	if (response == NULL)
	{
		perror("malloc");
		return NULL;
	}
	
	sprintf(response, 	"HTTP/1.1 %d %s\r\n"
				"Server: %s\r\n"
				"Content-Length: %d\r\n"
				"Connection: %s\r\n"
				"Content-Type: %s\r\n\r\n",
				 header->msg, s_msg, header->server, header->length, header->connection, header->type);
	
	int i = 0;
	int c = strlen(response);
	// Copy body
	if (header->body != NULL)
	{
		while (i < header->length)
		{
			response[i + c] = header->body[i];
			i++;
		}
		// Not sure if needed:
		//response[i++ + c] = '\r';
		//response[i++ + c] = '\n';
		//response[i++ + c] = '\0'; 
		//printf("\n%d\n",(c));
	}	
	*total_bytes = i + c;
	return (char *) realloc(response, i + c);
}

// Gets connection type
int get_connection(char* parsedRequest[MAXHEADLINES][MAXARGS], int num_headlines)
{
	// Find connection
	int connection = 0, i;
	for (i = 0; i < num_headlines; i++)
	{
		if (strcmp(parsedRequest[i][0], "Connection:") == 0)
		{
			if (parsedRequest[i][1] != NULL)
			{
				if (strcmp(parsedRequest[i][1], "close") != 0)
				{
					connection = 1;
					break;
				}
			}
		}
	}
	
	return connection;
}

// Response for 501 Not Implemented
void not_implemented(struct head_struct* replyStruct)
{
	if (replyStruct == NULL)
	{
		return;
	}
	replyStruct->msg = 501;
	replyStruct->length = 24;
	replyStruct->server = server_name;
	replyStruct->type = "text/plain";
	replyStruct->body = (char *) malloc(25); 
	strcpy(replyStruct->body, "Method not implemented!\n");
}

//Execute file
void execFile(int flag,struct head_struct* replyStruct,char* file)
{
	int pipefd[2];
    pid_t pid;
    char buffer[2048];
	int size=0;
	char* argv[3]={NULL};

    // create a pipegcc -o tls_client tls_client.c -lssl -lcrypto -Wno-deprecated-declarations
    if (pipe(pipefd) == -1) {
        perror("pipe");
    }

	pid = fork();
	if (pid == -1) {
		exit(1);
	} else if (pid == 0) {
		// child process
        close(pipefd[0]); // close the read end of the pipe
		// redirect stdout to the write end of the pipe
        dup2(pipefd[1], STDOUT_FILENO);

		switch(flag){
			case 0: //python
				argv[0] = "python";
				argv[1] = file;
				argv[2] = NULL;
                if (execvp("python", argv) < 0) {
                    perror("execvp");
                    exit(1);
                }
				exit(0);
			break;
			case 1: //php
				argv[0] = "php";
				argv[1] = file;
				argv[2] = NULL;
                if (execvp("php", argv) < 0) {
                    perror("execvp");
                    exit(1);
                }
				exit(0);
			break;
			default:
				replyStruct->body=NULL;
				replyStruct->msg= 404;
				replyStruct->length=0;
				perror("ContentTypeGet");
				exit(1);
		}
	} else {
		close(pipefd[1]); // close the write end of the pipe

        // read the output from the child process
        while (read(pipefd[0], buffer, sizeof(buffer)) > 0) {
            //printf("%s", buffer);
        }
		if (buffer == NULL) {
			replyStruct->body=NULL;
			replyStruct->msg= 404;
			replyStruct->length=0;
            perror("read");
            exit(EXIT_FAILURE);
        }
		
		//printf("Output: %s", buffer);
		replyStruct->length = strlen(buffer);
		replyStruct->body = (char *) malloc(replyStruct->length + 1);
		strcpy(replyStruct->body, buffer);
		replyStruct->msg= 200;
		wait(NULL); 
		return;
	}
}

// Clean replyStruct memory
void free_replyStruct(struct head_struct* replyStruct)
{
	if (replyStruct->body != NULL)
	{
		free(replyStruct->body);
	}
}


// Splits the header and body to two separate strings
void split_header_body(char *buf, char *header, char *body, int bytes_read, int *body_bytes)
{
	int i = 0, c = 0;
	int read_header = 1;
	
	while (i < bytes_read)
	{
		// Switch from header to body
		if (i + 3 < bytes_read && buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' && buf[i + 3] == '\n')
		{
			read_header = 0;
			header[c] = '\0';
			c = 0;
			i += 4;
		}
		
		if (read_header)
		{
			header[c++] = buf[i];
		}
		else
		{
			body[c++] = buf[i];
		}
		i++;
	}
	
	if (read_header)
	{
		header[c] = '\0';
		c = 0;
	}
	
	body[c] = '\0';
	*body_bytes = c;
}

int process_request(char* parsedRequest[MAXHEADLINES][MAXARGS], struct head_struct* replyStruct, int request_id, char *body, int body_bytes)
{
	int i=0;
	int exec_flag=-1;

	// Get file data
	char *path, *file, *type;
	get_file_data(parsedRequest[0][1], file_extensions, &path, &file, &type);

	replyStruct->server= server_name;
	replyStruct->type= type;

	/*
	printf("Path: %s %ld\n", path, strlen(path));
	printf("File: %s %ld\n", file, strlen(file));
	printf("Type: %s %ld\n", type, strlen(type));
	*/

	// Fix path
	char full_path[strlen(path) + strlen(home_directory) + 1];
	strcpy(full_path, home_directory);
	strcat(full_path, path);
	//printf("Full path: %s %ld\n", full_path, strlen(full_path));
	
	// Move to given directory
	if (chdir(full_path) < 0)
	{
		perror(path);
		return -1;
	}

	if (request_id == 2) // POST
	{
		replyStruct->length = body_bytes;
		replyStruct->body = NULL;
		replyStruct->msg = 200;
	
		int fd;
		if ((fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0660)) < 0)
		{
			replyStruct->msg = 404;
			perror(file);
			return -1;
		}
		
		write(fd, body, body_bytes);
		//printf("Body bytes: %d\n", body_bytes);
		
		close(fd);
	}
	else
	{
		if (access(file, F_OK) != 0)
		{
			replyStruct->body=NULL;
			replyStruct->msg= 404;
			replyStruct->length=0;
			//perror(file);
		}
		else
		{
			// Find exec type (py, php)
			for(i = 0; i < MAX_EXEC_TYPE; i++)
			{
				//printf("%s : %s\n", type, exec_type[i]);
				if(strcmp(type, exec_type[i]) == 0)
				{
					exec_flag = i;
					break;
				}
			}

			if(exec_flag >= 0 && request_id != 3) // NOT DELETE
			{
				// Execute python or php script
				execFile(exec_flag, replyStruct, file);
			}
			else
			{
				replyStruct->body = readFile(file, replyStruct);
				replyStruct->msg= 200;
				
				if(request_id == 3) // DELETE
				{
					if(remove(file)!=0)
					{
						//perror("Delete");
						replyStruct->msg= 404;
					}
				}
				//printf("file exists\n");
			}

		}
	}
	
	// Free data
	free(path);
	free(file);
	// free(type); // NOT NEEDED - type is const char *

	// Change back to home directory 
	if (chdir(cwd) < 0)
	{
		perror("cd");
		return -1;
	}
	
	return 0;
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
		while (isEmpty(socket_queue))
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
		
		printf("Thread %d, Socket Descriptor: %d\n\n", threadId , socket_fd);
		/* creates a new SSL structure which is needed to hold the data 
		* for a TLS/SSL connection
		*/ 
		SSL *ssl;
		int num_headlines=-1;
		int i=0;
				
		char buffer[BUF_SIZE];	
		char header[BUF_SIZE]; // less than BUF_SIZE
		char body[BUF_SIZE];
			
		struct head_struct replyStruct;
		char *tokenizedRequest[MAXHEADLINES];
		char *parsedRequest[MAXHEADLINES][MAXARGS];
		int connection;

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
			do {
				// Clean from previous request
				free_replyStruct(&replyStruct);
				
				// Get client request			
				int bytes_read = SSL_read(ssl, buffer, BUF_SIZE), body_bytes = 0;
				//printf("%s",buffer);

				if (bytes_read < 0)
				{
					fprintf(stderr, "No bytes read from socket %d\n", socket_fd);
					break;
				}

				// Split request header and body
				split_header_body(buffer, header, body, bytes_read, &body_bytes);
				printf("***Incoming Request(%ld)***\n",strlen(header));
				printf("---Header---\n%s\n\n", header);

				if(strlen(body)!=0)
					printf("---Body(%ld)---\n%s\n\n",strlen(body),body);

				// Tokenize request header
				if ((num_headlines = tokenize(header, "\r\n", tokenizedRequest)) < 0)
				{	
					fprintf(stderr, "Tokenization Error: %d\n", socket_fd);
					//terminate_Ssl(ssl,socket_fd);
					break;
				}
				
				for(i = 0; i < num_headlines; i++)
				{
					if (tokenize(tokenizedRequest[i], " ", parsedRequest[i]) < 0)
					{	
						fprintf(stderr, "Tokenization Error: %d\n", socket_fd);
						//terminate_Ssl(ssl,socket_fd);
						break;
					}
				}
				
				//print_parsedRequest(parsedRequest);
				
				// Find request method
				int request_index = -1;
				for (i = 0; i < MAX_REQUEST_METHODS; i++)
				{
					if (strcmp(parsedRequest[0][0], request_methods[i]) == 0)
					{
						request_index = i;
						break;
					}
				}
				
				// Set connection
				connection = get_connection(parsedRequest, num_headlines);
				replyStruct.connection = (connection == 0) ? "close" : "keep-alive";
				
				// Perform action based on request method
				if (request_index >= 0)
				{
					if (process_request(parsedRequest, &replyStruct, request_index, body, body_bytes) < 0)
					{
						fprintf(stderr, "Request processing error: %d\n", socket_fd);
						break;
					}
					
					// HEAD or DELETE or POST
					if (request_index > 0)
					{
						free_replyStruct(&replyStruct);
						
						// In our design DELETE and POST do not have a body in the response
						if (request_index == 2 || request_index == 3)
						{
							replyStruct.length = 0;
						}
						
						replyStruct.body = NULL;
					}
				}
				else
				{
					// Method not implemented
					not_implemented(&replyStruct);
				}
				
				// Send response to client
				int total_bytes = 0;
				char *reply = get_response_string(&replyStruct, &total_bytes);
				if (reply != NULL)
				{
					printf("***Start of Reply Message***\n%s***End of Reply***\n", add_null_termination(reply));
				
					SSL_write(ssl, reply, total_bytes);
					free(reply);
					
					printf("Reply sent successfully!\n\n");
				}
				else
				{
					fprintf(stderr, "Unable to respond.\n");
				}
			
			} while(connection != 0);
		}

		printf("\nSocket connection %d closed\n",socket_fd);
		terminate_Ssl(ssl, socket_fd);

					
		//printf("Thread %d, Entry: %d\n", threadId , socket_fd);
	}
	
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	// Init constants
	file_extensions[0] = "txt";
	file_extensions[1] = "sed";
	file_extensions[2] = "awk";
	file_extensions[3] = "c";
	file_extensions[4] = "h";
	file_extensions[5] = "html";
	file_extensions[6] = "htm";
	file_extensions[7] = "php";
	file_extensions[8] = "py";
	file_extensions[9] = "jpeg";
	file_extensions[10] = "jpg";
	file_extensions[11] = "gif";
	file_extensions[12] = "pdf";
	
	request_methods[0] = "GET";
	request_methods[1] = "HEAD";
	request_methods[2] = "POST";
	request_methods[3] = "DELETE";

	exec_type[0]= "application/x-python-code";
	exec_type[1]= "text/x-php";

	if (configure_parameters(CONF_FILENAME, &num_threads, &port, &home_directory, &server_name) < 0)
	{
		fprintf(stderr, "Unable to set configuration file parameters.\n");
		exit(1);
	}
	
	printf("Threads: %d\n", num_threads);
	printf("Port: %d\n", port);
	printf("Home: %s\n", home_directory);
	printf("Server: %s\n\n", server_name);

	// Get current working directory
	getcwd(cwd, sizeof(cwd));

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
		pthread_mutex_lock(&mutex);
		enqueue(client, socket_queue);
		pthread_mutex_unlock(&mutex);
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
