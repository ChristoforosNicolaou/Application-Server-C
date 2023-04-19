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
