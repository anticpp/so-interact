#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <ctype.h>
#include <linenoise/linenoise.h>

#if 0
static struct termios origin_tm;

static void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &origin_tm);
}

int enable_raw_mode() {
    struct termios tm;

    if( !isatty(STDIN_FILENO) ) {
        return 0;
    }

    if( tcgetattr(STDIN_FILENO, &origin_tm)<0 ) {
        return -1;
    }
    tm = origin_tm;
    cfmakeraw(&tm);

    if( tcsetattr(STDIN_FILENO, TCSAFLUSH, &tm)<0 ) {
        return -1;
    }

    // Reset attr on exit
    atexit(disable_raw_mode);
    return 0;
}

/* Print buffer
 * printable => char
 * '\r'  => '\r'
 * '\n'  => '\n'
 * other => {HEX}
 * .... (to be added)
 */
static void printbuf(FILE *s, char *buf, size_t n) {
    char c;
    for(int i=0; i<n; i++) {
        c = buf[i];
        if( isprint(c) ) {
            fprintf(s, "%c", c);          
        } else if( c=='\r' ) {
            fprintf(s, "\\r");
        } else if( c=='\n' ) {
            fprintf(s, "\\n");
        } else {
            fprintf(s, "[%X]", c);
        }
    }
}

int main(int argc, char *argv[])
{
    if( argc<3 ) {
        fprintf(stderr, "Usage: %s ip port\n", argv[0]);
        return 1;
    }

    const char *ip = argv[1];
    short port = atoi(argv[2]);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if( fd<0 ) {
        fprintf(stderr, "create socket error: %s\n", strerror(errno));
        return 1;      
    }
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    if( inet_pton(AF_INET, ip, &saddr.sin_addr)<0 ) {
        fprintf(stderr, "inet_pton error: %s, ip '%s'\n", strerror(errno), ip);
        return 1;      
    }

    if( connect(fd, (struct sockaddr*)&saddr, sizeof(saddr))<0 ) {
        fprintf(stderr, "connect to %s:%d error: %s\n", ip, port, strerror(errno));
        return 1;
    }

    if( enable_raw_mode() ) {
        fprintf(stderr, "set raw mode to stdin error\n");
        return 1;
    }

    /* Notice:
     *  We are now in raw mode.
     *  We have to print '\r' to left edge manually.
     */
    fprintf(stderr, "command 'h' for help.\r\n");

    char buf[1024] = {0};
    ssize_t bytes;
    while(1) {
        char c;
        fprintf(stderr, "command > ");
        if( read(STDIN_FILENO, &c, 1)<0 ) {
            continue;
        }
        if( c=='h' ) {
            fprintf(stderr, "h | help\r\n");
            fprintf(stderr, "r | recv data\r\n");
            fprintf(stderr, "s | send data\r\n");
            fprintf(stderr, "c | close data\r\n");
            fprintf(stderr, "x | shutdown READ\r\n");
            fprintf(stderr, "y | shutdown WRITE\r\n");
        } else if( c=='r' ) {
            fprintf(stderr, "Recving ...\r\n");
            memset(buf, 0x00, sizeof(buf));
            bytes = recv(fd, buf, sizeof(buf), 0);
            if( bytes<0 ) {
                fprintf(stderr, "recv error: %s\r\n", strerror(errno));
            } else if( bytes==0 ) {
                fprintf(stderr, "other side closed\r\n");
            } else {
                printf("RECV %d bytes\r\n", strlen(buf));
                printbuf(stdout, buf, strlen(buf));
                printf("\r\n");
            }
        } else if( c=='s' ) {
            fprintf(stderr, "Sending ...\r\n");
            memset(buf, 0x00, sizeof(buf));
            char timeb[128] = {0};
            time_t now = time(0);
            ctime_r(&now, timeb); // ctime_r make ending with '\n'
            timeb[strlen(timeb)-1] = '\0'; // Strip '\n'
            sprintf(buf, "[%s] Hello due\n", timeb);
            bytes = send(fd, buf, strlen(buf), 0);
            if( bytes<0 ) {
                fprintf(stderr, "send error: %s\r\n", strerror(errno));
            } else {
                printf("SEND (%d) bytes\r\n", strlen(buf));
                printbuf(stdout, buf, strlen(buf));
                printf("\r\n");
            }
        } else if( c=='c' ) {
            fprintf(stderr, "Closing ...\r\n");
            close(fd);
        } else if ( c=='q' ) {
            fprintf(stderr, "Quit ...\r\n");
            break;
        } else if ( c=='x') {
            if( shutdown(fd, SHUT_RD)<0 ) {
                fprintf(stderr, "shutdown error: %s\r\n", strerror(errno));
            } else {
                fprintf(stderr, "shutdown READ succ\r\n");
            }
        } else if ( c=='y') {
            if( shutdown(fd, SHUT_WR)<0 ) {
                fprintf(stderr, "shutdown error: %s\r\n", strerror(errno));
            } else {
                fprintf(stderr, "shutdown WRITE succ\r\n");
            }
        } else {
            fprintf(stderr, "Unknown command '%c'\r\n", c);
            continue;
        }
    }
    fprintf(stderr, "Exiting ...\r\n");
    
    return 0;
}
#endif

static void completion(const char *buf, linenoiseCompletions *lc) {
    if (buf[0] == 'c') {
        linenoiseAddCompletion(lc,"connect");
    }
}

static char *hints(const char *buf, int *color, int *bold) {
    if ( strcasecmp(buf,"connect")==0 ) {
        *color = 35;
        *bold = 0;
        return " ip port";
    }
    return NULL;
}

typedef enum {
    s_closed = 0,
    s_connected = 1,
    s_shutdown_rd = 2,
    s_shutdown_wr = 3
} state_e;

typedef struct {
    state_e state; 
    int cfd;
} client_s;

// Global client
static client_s client;

enum {
    k_space = 32,
    k_tab = 9
};

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

#define MAX_ARG_SIZE 20

// TODO: Move test to test file
int test_parse_args() {
    struct test_data {
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
    
    int size = sizeof(tests)/sizeof(struct test_data);
    for( int i=0; i<size; i++ ) {
        printf("Test ==> [%d]\n", i);
        struct test_data *t = &tests[i];
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

typedef int (*do_command)(int argc, char **args);

int do_command_connect(int argc, char **args) {
    if( argc<3 ) {
        printf("args error\n");
        printf("Usage: %s ip port\n", args[0]);
        return 1;
    }

    printf("do_command_connect\n");
    const char *ip = args[1];
    short port = atoi(args[2]);

    client.cfd = socket(AF_INET, SOCK_STREAM, 0);
    if( client.cfd<0 ) {
        fprintf(stderr, "create socket error: %s\n", strerror(errno));
        return 1;      
    }
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    if( inet_pton(AF_INET, ip, &saddr.sin_addr)<0 ) {
        fprintf(stderr, "inet_pton error: %s, ip '%s'\n", strerror(errno), ip);
        return 1;      
    }

    if( connect(client.cfd, (struct sockaddr*)&saddr, sizeof(saddr))<0 ) {
        fprintf(stderr, "connect to %s:%d error: %s\n", ip, port, strerror(errno));
        return 1;
    }

    printf("connect to %s:%d success\n", ip, port);
    return 0;
}

typedef struct {
    const char *name;
    do_command handler;
} command_s; 

command_s commands[] = {
    {"connect", do_command_connect}
};

#define HISTORY_LOG ".chistory.txt"

int main(int argc, char **argv) {
    linenoiseSetCompletionCallback(completion);
    linenoiseSetHintsCallback(hints);
    linenoiseHistoryLoad(HISTORY_LOG);

    char *line;
    int largc = 0;
    char *largs[MAX_ARG_SIZE];
    while( (line=linenoise("client > "))!=NULL ) {
        largc = parse_args(line, largs, MAX_ARG_SIZE);
        if( largc<0 ) {
            fprintf(stderr, "Parse line error: '%s'\n", line);
            continue;
        } else if( largc==0 ) {
            // empty line
            continue;
        }

        linenoiseHistoryAdd(line);
        linenoiseHistorySave(HISTORY_LOG);

        int i = 0;
        int cmd_n = sizeof(commands)/sizeof(command_s);
        for( ; i<cmd_n; i++ ) {
            if( strcmp(largs[0], commands[i].name)!=0 ) {
                continue;
            }
            commands[i].handler(largc, largs);
            break;
        }
        if( i==cmd_n ) {
            fprintf(stderr, "Command not found: '%s'\n", largs[0]);
        }

        #if 0
        /* Do something with the string. */
        if (line[0] != '\0' && line[0] != '/') {
            printf("echo: '%s'\n", line);
            linenoiseHistoryAdd(line); /* Add to the history. */
            linenoiseHistorySave("history.txt"); /* Save the history on disk. */
        }
        #endif

        free_args(largc, largs);
        free(line);
    }
    return 0;
}
