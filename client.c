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

/* Format src buffer to be printable.
 * ascii => char
 * '\r'  => '\r'
 * '\n'  => '\n'
 * .... (to be added)
 */
static int format_buffer(char *dst, size_t dst_size, 
                const char *src, size_t src_size) {

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

    fprintf(stderr, "input commands:\r\n");
    fprintf(stderr, "h | help\r\n");

    char buf[1024] = {0};
    ssize_t bytes;
    while(1) {
        char c;
        if( read(STDIN_FILENO, &c, 1)<0 ) {
            continue;
        }
        fprintf(stderr, "command read '%c'\r\n", c);
        if( c=='h' ) {
            fprintf(stderr, "h | help\r\n");
            fprintf(stderr, "r | recv data\r\n");
            fprintf(stderr, "s | send data\r\n");
            fprintf(stderr, "c | close data\r\n");
            fprintf(stderr, "x | shutdown READ\r\n");
            fprintf(stderr, "y | shutdown WRITE\r\n");
        } else if( c=='r' ) {
            fprintf(stderr, "Recving ...\r\n");
            bytes = recv(fd, buf, sizeof(buf), 0);
            if( bytes<0 ) {
                fprintf(stderr, "recv error: %s\n", strerror(errno));
            } else if( bytes==0 ) {
                fprintf(stderr, "other side closed\n");
            } else {
                buf[bytes] = '\0';
                if( buf[bytes-1]=='\n' ) {
                    /* Strip '\n'
                     */
                    buf[bytes-1] = '\0';
                }
                printf("RECV (%d)'%s\\n'\r\n", strlen(buf)+1, buf);
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
                fprintf(stderr, "send error: %s\n", strerror(errno));
            } else {
                buf[strlen(buf)-1] = '\0'; // Strip '\n'
                printf("SEND (%d)'%s'\r\n", bytes, buf);
            }
        } else if( c=='c' ) {
            fprintf(stderr, "Closing ...\r\n");
            close(fd);
            break;
        } else if (c=='q') {
            fprintf(stderr, "Quit ...\r\n");
            break;
        }else {
            fprintf(stderr, "Unknown command '%c'\r\n", c);
            continue;
        }
    }
    fprintf(stderr, "Exiting ...\r\n");
    
    return 0;
}
