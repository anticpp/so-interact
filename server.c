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
#include "define.h"
#include "args.c"
#include "printbuf.c"
#include "command.h"

/* Linenoise callback */
static void completion(const char *buf, linenoiseCompletions *lc);
static char *hints(const char *buf, int *color, int *bold);
/*
static int do_command_help(int argc, char **args);
static int do_command_state(int argc, char **args);
static int do_command_connect(int argc, char **args);
static int do_command_read(int argc, char **args);
static int do_command_write(int argc, char **args);
static int do_command_close(int argc, char **args);
static int do_command_shutdown(int argc, char **args);
*/
static int do_command_help(int argc, char **args);
static int do_command_state(int argc, char **args);
static int do_command_listen(int argc, char **args);
static int do_command_unlisten(int argc, char **args);
static int do_command_accept(int argc, char **args);
static int do_command_list(int argc, char **args);
static int do_command_close(int argc, char **args);

static command_t commands[] = {
    {"help", 0, NULL, "Help message", do_command_help},
    {"state", 0, NULL, "Show listen state", do_command_state},
    {"listen", 2, "`ip` `port`", "Listen on ip:port", do_command_listen},
    {"unlisten", 0, NULL, "Unlisten", do_command_unlisten},
    {"accept", 0, NULL, "Accept connection", do_command_accept},
    {"list", 0, NULL, "List connections", do_command_list},
    {"close", 1, " `cfd`|all", "Close connection", do_command_close},
    /*{"help", 0, 0, "Help message", do_command_help},
    {"state", }
    {"accept", }
    {"read", }
    {"write", }
    {"close", }
    {"shutdown", }*/
 /*   {"help", 0, 0, "Help message", do_command_help},
    {"connect", 2, "`ip` `port`", "Make connection", do_command_connect},
    {"read", 0, 0, "Read data from connection", do_command_read},
    {"write", 1, "`message`", "Write `message` to connection", do_command_write},
    {"close", 0, 0, "Close connection", do_command_close},
    {"shutdown", 1, "read|write", "Shutdown connection", do_command_shutdown},
    */
};
static int commands_n = sizeof(commands)/sizeof(command_t);

static inline command_t* mylookup(const char* name) {
    return lookup_command(name, commands, commands_n);
}

/* Connection state */
typedef enum {
    s_init = 0,
    s_listen = 1,
} state_t;

const char *state_str[] = {"s_init", "s_listen"};

#define MAX_CONN_N 100

/* server */
typedef struct {
    int cfd;
    struct sockaddr_in addr;
    socklen_t addr_len;
} conn_t;

typedef struct {
    state_t state; 
    int sfd;

    /* Connections */
    conn_t conns[MAX_CONN_N];
    int conns_n;
} server_t;

static server_t server;

#define HISTORY_LOG ".shistory.txt"
#define PROMPT "server > "

int main(int argc, char **argv) {
    /* Init server */
    server.state = s_init;
    server.sfd = 0;
    server.conns_n = 0;

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
        } else if( largc==0 ) {
            /* Empty line */
            /* Do nothing */
        } else {
            linenoiseHistoryAdd(line);
            linenoiseHistorySave(HISTORY_LOG);

            command_t *cmd = mylookup(largs[0]);
            if( !cmd ) {
                fprintf(stderr, "Command not found: '%s'\n", largs[0]);
            } else if ( largc<cmd->argc+1 ) {
                fprintf(stderr, "arguments error\n");
                fprintf(stderr, "usage: %s %s\n", cmd->name, cmd->help_args);
            } else if( cmd->handler ) {
                cmd->handler(largc, largs);
            }
        }

        free_args(largc, largs);
        free(line);
    }
    return 0;
}

void completion(const char *buf, linenoiseCompletions *lc) {
    /* We can not use mylookup() here, */
    /* because `buf` is not a complete command name. */
    /* We match prefix of buf with command name. */
    int n = strlen(buf);
    for(int i=0; i<commands_n; i++) {
        command_t *cmd = &commands[i];
        if( strncasecmp(buf, cmd->name, n)==0 ) {
            linenoiseAddCompletion(lc, cmd->name);
        }  
    }
}

char *hints(const char *buf, int *color, int *bold) {
    *color = 35;
    *bold = 0;

    static char tmp[256];
    command_t *cmd = mylookup(buf);
    if( !cmd ) {
        return NULL;
    }
    if( cmd->help_args!=NULL && 
            strlen(cmd->help_args)!=0 ) {
        snprintf(tmp, sizeof(tmp), " %s", cmd->help_args); // " " + cmd->help_args
        return tmp;
    }
    return NULL;
}

int do_command_help(int argc, char **args) {
    char tmp[256];
    for( int i=0; i<commands_n; i++ ) {
        command_t *cmd = &commands[i];
        if( cmd->help_args==NULL || strlen(cmd->help_args)==0 ) {
            snprintf(tmp, sizeof(tmp), "%s", cmd->name); 
        } else {
            snprintf(tmp, sizeof(tmp), "%s %s", cmd->name, cmd->help_args);
        }
        printf("%-30s%s\n", tmp, cmd->help_message);
    }
    return 0;
}

int do_command_state(int argc, char **args) {
    printf("%s\n", state_str[server.state]);
}

int do_command_listen(int argc, char **args) {
    const char *ip = args[1];
    unsigned short port = atoi(args[2]);

    if( server.state!=s_init ) {
        printf("Error state: `%s`\n", state_str[server.state]);
        printf("Already listen.\n");
        printf("Close current listen before next.\n");
        return 1;
    }

    server.sfd = socket(AF_INET, SOCK_STREAM, 0);
    if( server.sfd<0 ) {
        fprintf(stderr, "create socket error: %s\n", strerror(errno));
        return 1;      
    }

    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    if( inet_pton(AF_INET, ip, &saddr.sin_addr)<0 ) {
        printf("inet_pton error: %s, ip '%s'\n", strerror(errno), ip);
        close(server.sfd);
        return 1;      
    }

    int v = 1;
    if( setsockopt(server.sfd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) ) {
        printf("setsockopt(SO_REUSEADDR) error: %s\n", strerror(errno));
        close(server.sfd);
        return 1;      
    }

    if( bind(server.sfd, (struct sockaddr*)&saddr, sizeof(saddr))<0 ) {
        printf("bind error: %s\n", strerror(errno));
        close(server.sfd);
        return 1;      
    }

    if( listen(server.sfd, 100)<0 ) {
        printf("listen error: %s\n", strerror(errno));
        close(server.sfd);
        return 1;      
    } 

    server.state = s_listen;
    printf("Listen %s:%d succ \n", ip, port);
}

int do_command_accept(int argc, char **args) {
    if( server.state!=s_listen ) {
        printf("Error state: `%s`\n", state_str[server.state]);
        printf("Listen before accept.\n");
        return 1;
    }
    if( server.conns_n>= MAX_CONN_N ) {
        printf("Too much connections accepted, MAX_CONN_N %d\n", MAX_CONN_N);
        return 1;
    }

    printf("Accepting ...\n");
    conn_t *conn = &(server.conns[server.conns_n]);
    conn->addr_len = sizeof(conn->addr);

    conn->cfd = accept(server.sfd, (struct sockaddr*)&(conn->addr), &(conn->addr_len));
    if( conn->cfd<0 ) {
        printf("accept error: %s\n", strerror(errno));
        return 1;
    }

    char ip[32] = {0};
    unsigned short port = 0;
    if( inet_ntop(AF_INET, &(conn->addr.sin_addr), ip, conn->addr_len)<0 ) {
        printf("warning: inet_ntop error: %s\n", strerror(errno));
    }
    port = ntohs(conn->addr.sin_port);

    printf("Accepted connection %s:%d\n", ip, port);
    printf("Fd %d\n", conn->cfd);

    server.conns_n ++;
    return 0;
}

int do_command_list(int argc, char **args) {
    char ip[32] = {0};
    unsigned short port = 0;

    printf("Connections(%d)\n", server.conns_n);
    for( int i=0; i<server.conns_n; i++ ) {
        conn_t *conn = &(server.conns[i]);
        inet_ntop(AF_INET, &(conn->addr.sin_addr), ip, conn->addr_len);
        port = ntohs(conn->addr.sin_port);
        printf("[%d] %s:%d\n", conn->cfd, ip, port);
    }
    return 0;
}

int do_command_close(int argc, char **args) {
    if( strcasecmp(args[1], "all")==0 ) {
        /* CLose all */
        for( int i=0; i<server.conns_n; i++ ) {
            close(server.conns[i].cfd);
            printf("Connection %d closed\n", server.conns[i].cfd);
        }
        server.conns_n = 0;
        return 0;
    }

    int cfd = atoi(args[1]);
    int i = 0;
    conn_t *conn = NULL;
    for( ; i<server.conns_n; i++ ) {
        if( server.conns[i].cfd!=cfd ) {
            continue;
        }
        conn = &(server.conns[i]);
        break;
    }
    if( i==server.conns_n ) {
        printf("Connection not found, cfd %d\n", cfd);
        printf("Use `list` command to check cfd\n", cfd);
        return 1;
    }
    close(conn->cfd);

    if( i==server.conns_n-1 ) {
        /* Last element */
    } else {
        /* i<server.conns_n-1 */
        memmove( &(server.conns[i]), 
                 &(server.conns[i+1]),
                 sizeof(conn_t)*(server.conns_n-1-i) );
    }
    server.conns_n --;
    printf("Connection %d closed\n", cfd);
}

int do_command_unlisten(int argc, char **args) {
    if( server.state!=s_listen ) {
        printf("Error state: `%s`\n", state_str[server.state]);
        printf("Not listen state.\n");
        return 1;
    }

    /* Just close listen fd */
    close(server.sfd);

    server.sfd = 0;
    server.state = s_init;
    printf("Unlisten succ\n");
}
