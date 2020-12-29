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
    
    int size = sizeof(tests)/sizeof(struct test_data_s_);
    for( int i=0; i<size; i++ ) {
        printf("Test ==> [%d]\n", i);
        struct test_data_s_ *t = &tests[i];
        int argc;
        char *args[MAX_ARG_SIZE];
        argc = parse_args(t->line, args, MAX_ARG_SIZE);

        int succ = 1;
        if( argc!=t->argc ) {
            printf("argc(%d) != expected argc(%d)\n", argc, t->argc);
            succ = 0;
        } else {
            if( argc>0 ) {
                for( int j=0; j<argc; j++ ) {
                    if( strcmp(args[j], t->args[j])!=0 ) {
                        printf("args[%d]('%s') != expected args[%d]('%s')\n", j, args[j], j, t->args[j]);
                        succ = 0;
                        break;
                    }
                }
            }
        }

        if( argc>0 ) {
            free_args(argc, args);
        }

        if( succ ) {
            printf("success\n", i);
        } else {
            printf("fail\n", i);
        }
    }  
    return 0;
}

int main(int argc, char *argv[])
{
    printf("+--------- test_parse_args ----------+\n");
    test_parse_args();   
    return 0;
}
