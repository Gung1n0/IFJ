#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

#include <stdlib.h>
#include <string.h>

static inline char *strduplicate(const char *s)
{
    char *p = (char *)malloc(strlen(s) + 1);
    if (p != NULL)
    {
        strcpy(p, s);
    }
    return p;
}

#endif