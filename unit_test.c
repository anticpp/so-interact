#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "args.c"
#include "define.h"

int test_parse_args() {
    struct test_data_s_ {
        const char *line;
        int argc;
        char *args[MAX_ARG_SIZE];
    } tests[] = {
        // Empty
        {"", 0, {}},

        // Simple
        {"hi", 1, {"hi"}} ,

        // Head trim
        {"  hi", 1, {"hi"}} ,      // Head SPACE
        {"\thi", 1, {"hi"}} ,      // Head TAB
        {"\t  hi", 1, {"hi"}} ,    // Head SPACE and TAB

        // Tail trim
        {"hi    ", 1, {"hi"}} ,    // Tail SPACE
        {"hi\t", 1, {"hi"}} ,      // Tail TAB
        {"hi   \t", 1, {"hi"}} ,   // Tail SPACE and TAB

        // 2 arguments
        {"hello supergui", 2, {"hello", "supergui"}}, // SPACE 
        {"hello\tsupergui", 2, {"hello", "supergui"}}, // TAB
        {"hello  \t  supergui", 2, {"hello", "supergui"}}, // SPACE and TAB
        {"  \thello  \t  supergui  \t  ", 2, {"hello", "supergui"}}, // SPACE and TAB, Trim

        // 3 arguments
        {"set name supergui", 3, {"set", "name", "supergui"}}, // SPACE
        {"set\tname\tsupergui", 3, {"set", "name", "supergui"}}, // TAB
        {"set  \t  name  \t  supergui", 3, {"set", "name", "supergui"}}, // SPACE and TAB
        {"  \tset  \t  name  \t  supergui  \t  ", 3, {"set", "name", "supergui"}}, // SPACE and TAB, Trim
    };
    
    int ret = 0;
    int size = sizeof(tests)/sizeof(struct test_data_s_);
    for( int i=0; i<size; i++ ) {
        struct test_data_s_ *t = &tests[i];
        int argc;
        char *args[MAX_ARG_SIZE];
        argc = parse_args(t->line, args, MAX_ARG_SIZE);
        if( argc<0 ) {
            printf("[Test %d] parse_args errror\n", i);
            ret = 1;
        } else if( argc!=t->argc ) {
            printf("[Test %d] argc(%d) != expected argc(%d)\n", i, argc, t->argc);
            ret = 1;
        } else {
            int j = 0;
            for( ; j<argc; j++ ) {
                if( strcmp(args[j], t->args[j])!=0 ) {
                    printf("[Test %d] args[%d]('%s') != expected args[%d]('%s')\n", i, j, args[j], j, t->args[j]);
                    break;
                }
            }
            
            /* Not totally matched */ 
            if( j!=argc ) {
                ret = 1;
            }
        }

        free_args(argc, args);
    }  
    return ret;
}

int main(int argc, char *argv[])
{
    printf("[ test_parse_args ]\n");
    if( test_parse_args()!=0 ) {
        printf("Fail\n");
    } else {
        printf("Success\n");
    }
    return 0;
}
