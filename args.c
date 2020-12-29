#include "define.h"

// -1:  parse error
// >=0: args
static int parse_args(const char *line, 
                char **args, 
                int max) {
    int argc = 0;
    const char *p1 = line;
    const char *p2 = 0;

    // Trim head
    while( *p1==k_tab || *p1==k_space ) 
        p1++;

    while( *p1!='\0' ) {
        p2 = strchr(p1, k_space);
        if( !p2 ) {
            p2 = strchr(p1, k_tab);
            if( !p2 ) {
                break;
            }
        }

        if( argc>=max ) {
            return -1;
        }

        // Push argument
        args[argc] = malloc(p2-p1+1);
        memcpy(args[argc], p1, p2-p1);
        args[argc][p2-p1] = '\0';
        argc ++;

        // Trim tail
        while( *p2==k_tab || *p2==k_space ) 
            p2++;
        p1 = p2;
    }

    // Last argument
    if( *p1!='\0' ) {
        if( argc>=max ) {
            return -1;
        }
        
        int n = strlen(p1)+1;
        args[argc] = malloc(n);
        memcpy(args[argc], p1, n);
        args[argc][n] = '\0';
        argc ++;
    }
    return argc;
}

static void print_args(int argc, char **args) {
    for( int i=0; i<argc; i++ ) {
        printf("args[%d] = '%s'\n", i, args[i]);
    }
}

static void free_args(int argc, char **args) {
    for( int i=0; i<argc; i++ ) {
        free(args[i]);
        args[i] = 0;
    }
}
