#ifndef ERROR_H
#define ERROR_H

typedef enum{
    
    NO_ERR = 0,
    LEX_ERR = 1,
    SYN_ERR = 2,
    UNDEF_SEM_ERR = 3,
    REDEF_SEM_ERR = 4,
    ARG_SEM_ERR = 5,
    TYPE_SEM_ERR = 6,
    IMMUT_SEM_ERR = 7,
    OTHER_SEM_ERR = 10,

    RUNTIME_ERR = 40,
    INTERNAL_ERR = 99

} error_code;

void error_msg(error_code code, const char* msg);

#endif