#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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
    saddr.sin_port = port;
    if( inet_pton(AF_INET, ip, &saddr.sin_addr)<0 ) {
        fprintf(stderr, "inet_pton error: %s, ip '%s'\n", strerror(errno), ip);
        return 1;      
    }

    if( connect(fd, (struct sockaddr*)&saddr, sizeof(saddr))<0 ) {
        fprintf(stderr, "connect to %s:%d error: %s\n", ip, port, strerror(errno));
        return 1;
    }

    char rbuf[1024];
    ssize_t rbytes;
    while(1) {
        rbytes = recv(fd, rbuf, sizeof(rbuf), 0);
        if( rbytes<0 ) {
            if( errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR ) {
                continue;
            }
            fprintf(stderr, "recv error: %s\n", strerror(errno));
            break;
        } else if( rbytes==0 ) {
            fprintf(stderr, "other side closed\n");
            continue;
        }
        rbuf[rbytes] = '\0';
        printf("RECV '%s'\n", rbuf);
    }
    fprintf(stderr, "closing now...\n");
    close(fd);
    return 0;
}
