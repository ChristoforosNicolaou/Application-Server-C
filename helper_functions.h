#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


// Tokenize string "buf" into "tokens" based on "delimeters"
int tokenize(char *buf, const char *delimiter, char **tokens);

// Returns a substring from start to end
char *substr(char *string, int start, int end);

//TLS server functionality
int create_socket(int port,int max_requests);
void init_openssl();
void cleanup_openssl();
SSL_CTX *create_context();
void configure_context(SSL_CTX *ctx);

#endif
