/*
 * implementation of a compiler for the language IFJ26
 * semantics.h
 * xzombot00 Tomáš Zomborský
 */

#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "../Parser/ast.h"
#include "symtable.h"

/// @brief Analyzes the ast tree using 2 passes, one pass for analyzing all
/// function headeres (so functions can be used even if they weren't defined
/// yet), and then the second pass for actually analyzing the bodies of all
/// functions
/// @param program Root node of the AST tree
/// @param table Sym_table, for storing results of the analysis
void sem_analyze(node *program, sym_table *table);

#endif