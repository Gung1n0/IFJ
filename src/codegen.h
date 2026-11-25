/*
 * implementation of a compiler for the language IFJ26
 * codegen.h
 * code generator interface
 * 
 * xbadint00 Tomáš Badinský
 * 
 */

#ifndef CODEGEN_H
#define CODEGEN_H

struct node;

/// @brief Generates code of the program in IFJcode26 to stdout
/// @param program Main node of the AST checked by semantic analysis
/// nodes have their data_types and links to the symtable 
void code_generate(struct node *program);

#endif
