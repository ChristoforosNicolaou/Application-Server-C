#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Tokenize string "buf" into "tokens" based on "delimeters"
int tokenize(char *buf, const char *delimiter, char **tokens);

#endif
