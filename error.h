#ifndef ERROR_H
#define ERROR_H

#include "enums.h"

void gen_error(const char *message, error_code err, int row, int column);

#endif