#include "error.h"
#include <stdio.h>
#include <stdlib.h>

void gen_error(const char *message, error_code err, int row, int column) {
  fprintf(stderr, "%s at row %i column %i\n", message, row, column);
  exit(err);
}