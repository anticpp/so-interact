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
        } } fprintf(stderr, "Exiting ...\r\n"); return 0; }
#endif

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

/* Commands */
typedef int (*do_command)(int argc, char **args);

static int do_command_help(int argc, char **args);
static int do_command_state(int argc, char **args);
static int do_command_connect(int argc, char **args);
static int do_command_read(int argc, char **args);
static int do_command_write(int argc, char **args);
static int do_command_close(int argc, char **args);

typedef struct {
    const char *name;
    int argc; // Arguments required

    /* For help message */
    const char *help_args;   // 0 or "" if no additional arguments
    const char *help_message;

    do_command handler;
} command_s; 

command_s commands[] = {
    {"help", 0, 0, "Help message", do_command_help},
    {"state", 0, 0, "Show connection state", do_command_state},
    {"connect", 2, "`ip` `port`", "Make connection", do_command_connect},
    {"read", 0, 0, "Read data from connection", do_command_read},
    {"write", 1, "`message`", "Write `message` to connection", do_command_write},
    {"close", 0, 0, "Close connection", do_command_close},
    {"shutdown", 1, "[read|write]", "Shutdown connection", 0},
};

static void completion(const char *buf, linenoiseCompletions *lc) {
    int n = strlen(buf);
    for(int i=0; i<sizeof(commands)/sizeof(command_s); i++) {
        command_s *cmd = &commands[i];
        if( strncasecmp(buf, cmd->name, n)==0 ) {
            linenoiseAddCompletion(lc, cmd->name);
        }  
    }
}

static char *hints(const char *buf, int *color, int *bold) {
    *color = 35;
    *bold = 0;

    int tmp_size = 256;
    char *tmp = malloc(tmp_size);
    for(int i=0; i<sizeof(commands)/sizeof(command_s); i++) {
        command_s *cmd = &commands[i];
        if( strcasecmp(buf, cmd->name)==0 &&
                        cmd->help_args!=0 && 
                        strlen(cmd->help_args)!=0 ) {
            snprintf(tmp, tmp_size, " %s", cmd->help_args); // " " + cmd->help_args
            return tmp;
        }
    }
    return NULL;
}

typedef enum {
    s_closed = 0,
    s_connected = 1,
} state_e;

const char *state_str[] = {"s_closed", "s_connected"};

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

int do_command_connect(int argc, char **args) {
    const char *ip = args[1];
    short port = atoi(args[2]);

    if( client.state!=s_closed ) {
        printf("Error state: `%s`\n", state_str[client.state]);
        printf("Already connected.\n");
        printf("Close it before making a new connection.\n");
        return 1;
    }

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

    client.state = s_connected;
    printf("Connected %s:%d \n", ip, port);
    return 0;
}

int do_command_read(int argc, char **args) {
    if( client.state!=s_connected ) {
        printf("Error state: `%s`\n", state_str[client.state]);
        printf("Can't read on closed connection.\n");
        return 1;
    }
    printf("Reading ...\n");

    char buf[1024] = {0};
    size_t bytes = recv(client.cfd, buf, sizeof(buf), 0);
    if( bytes<0 ) {
        printf("recv error: %s\r\n", strerror(errno));
        return 1;
    } else if( bytes==0 ) {
        printf("other side closed\r\n");
        return 1;
    }
    printf("(%d)\'", bytes);
    printbuf(stdout, buf, strlen(buf));
    printf("\'\n");
}

int do_command_write(int argc, char **args) {
    if( client.state!=s_connected ) {
        printf("Error state: `%s`\n", state_str[client.state]);
        printf("Can't write on closed connection.\n");
        return 1;
    }
    printf("Writing ...\n");

    char *sbuf = args[1];
    size_t bytes = send(client.cfd, sbuf, strlen(sbuf), 0);
    if( bytes<0 ) {
        fprintf(stderr, "send error: %s\r\n", strerror(errno));
        return 1;
    }
    printf("(%d)\'%s\'\n", bytes, sbuf);
}

int do_command_close(int argc, char **args) {
    if( client.state!=s_connected ) {
        printf("Error state: `%s`.\n", state_str[client.state]);
        printf("Not connected.\n");
        return 1;
    }
    close(client.cfd);
    client.state = s_closed;
    printf("Closed\n");
}

int do_command_state(int argc, char **args) {
    printf("%s\n", state_str[client.state]);
}

int do_command_help(int argc, char **args) {
    char tmp[256];
    for(int i=0; i<sizeof(commands)/sizeof(command_s); i++) {
        command_s *cmd = &commands[i];
        if( cmd->help_args==0 || strlen(cmd->help_args)==0 ) {
            snprintf(tmp, sizeof(tmp), "%s", cmd->name); 
        } else {
            snprintf(tmp, sizeof(tmp), "%s %s", cmd->name, cmd->help_args);
        }
        printf("%-30s%s\n", tmp, cmd->help_message);
    }
    return 0;
}


#define HISTORY_LOG ".chistory.txt"
#define PROMPT "client > "

int main(int argc, char **argv) {
    /* Init client */
    client.state = s_closed;
    client.cfd = 0;

    linenoiseSetCompletionCallback(completion);
    linenoiseSetHintsCallback(hints);
    linenoiseHistoryLoad(HISTORY_LOG);

    char *line;
    int largc = 0;
    char *largs[MAX_ARG_SIZE];
    while( (line=linenoise(PROMPT))!=NULL) {
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
            command_s *cmd = &commands[i];
            if( strcmp(largs[0], cmd->name)!=0 ) {
                continue;
            }
            if( largc<cmd->argc+1 ) {
                fprintf(stderr, "arguments error\n");
                fprintf(stderr, "usage: %s %s\n", cmd->name, cmd->help_args);
                break;
            } 
            if( cmd->handler ) {
                cmd->handler(largc, largs);
            }
            break;
        }
        if( i==cmd_n ) {
            fprintf(stderr, "Command not found: '%s'\n", largs[0]);
        }

        free_args(largc, largs);
        free(line);
    }
    return 0;
}
