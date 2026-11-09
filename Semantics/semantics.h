#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "../Parser/ast.h"
#include "symtable.h"

void sem_analyze(node *program, sym_table *table);

#endif