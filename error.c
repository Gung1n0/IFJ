#include <stdio.h>
#include "error.h"

void error_msg(error_code code, const char* msg){
    fprintf(stderr, "Error %d: %s\n", (int)code, msg);
    exit((int)code);
}