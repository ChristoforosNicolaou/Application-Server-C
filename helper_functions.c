#include "helper_functions.h"

// Tokenize string "buf" into "tokens" based on "delimeters"
int tokenize(char *buf, const char *delimiters, char **tokens)
{
	char *token = strtok(buf, delimiters);
	
	int i = 0;
	while (token != NULL)
	{
		// Store token
		tokens[i] = (char *) malloc(strlen(token) + 1);
		if (tokens[i] == NULL)
		{
			perror("malloc");
			return -1;
		}
		
		strcpy(tokens[i++], token);
		
		// Get next token
		token = strtok(NULL, delimiters);
	}
	
	tokens[i] = NULL;
	return i; // Return number of tokens
}

// Returns a substring from start to end
char *substr(char *string, int start, int end)
{
	char *str = (char *) malloc(end - start + 1);
	if (str == NULL)
	{
		perror("malloc");
		return NULL;
	}
	
	int i = 0;
	while (i < end - start && start < strlen(string))
	{
		str[i] = string[start + i];
		i++;
	}
	str[i] = '\0';
	return str;
}


// TLS server functionality
int create_socket(int port,int max_requests)
{
    int s;
    struct sockaddr_in addr;

    /* set the type of connection to TCP/IP */
    addr.sin_family = AF_INET;
    /* set the server port number */
    addr.sin_port = htons(port);
    /* set our address to any interface */
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
	perror("Unable to create socket");
	exit(EXIT_FAILURE);
    }

    /* bind serv information to s socket */
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
	perror("Unable to bind");
	exit(EXIT_FAILURE);
    }
    
    /* start listening allowing a queue of up to 1 pending connection */
    if (listen(s, max_requests) < 0) {
	perror("Unable to listen");
	exit(EXIT_FAILURE);
    }

    return s;
}

void init_openssl()
{ 
    SSL_load_error_strings();	
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl()
{
    EVP_cleanup();
}


SSL_CTX *create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    /* The actual protocol version used will be negotiated to the 
     * highest version mutually supported by the client and the server.      
     * The supported protocols are SSLv3, TLSv1, TLSv1.1 and TLSv1.2. 
     */
    //method = SSLv23_server_method();
    method = TLSv1_2_server_method(); 

    /* creates a new SSL_CTX object as framework to establish TLS/SSL or   
     * DTLS enabled connections. It initializes the list of ciphers, the 
     * session cache setting, the callbacks, the keys and certificates, 
     * and the options to its default values
     */
    ctx = SSL_CTX_new(method);
    if (!ctx) {
	perror("Unable to create SSL context");
	ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX *ctx)
{
    SSL_CTX_set_ecdh_auto(ctx, 1);

    /* Set the key and cert using dedicated pem files */
    if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
    }
}
