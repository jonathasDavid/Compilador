#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "symbol_table.h"

void semantic_init(void);
void semantic_check_declaration(const char *name, VarType type, int line, int size, int dec_before, int dec_after);
void semantic_check_assignment(const char *name, VarType rtype, int line, int rsize, int rdec_after);
void semantic_declare_function(const char *fname, VarType ret_type, int line, int size);
void semantic_enter_function(const char *fname);
void semantic_leave_function(void);
void semantic_check_return(VarType rtype, int rsize, int rdec_after, int line);

#endif
