#ifndef STRTOK_QUO_
#define STRTOK_QUO_

#include <string.h>
#include <ctype.h>
#include <stdbool.h>

size_t strcspn_quo (char **str);
char * strtok_quo (char *s, char **save_ptr);

#endif
