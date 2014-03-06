#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include "net.h"


static void netPrintError(char *err, const char *format, ...) {
    va_list ap;
    if (!err) return;
    va_start(ap, format);
    vsnprintf(err, 256, format, ap);
    va_end(ap);
}

int netSetNonBlock(char *err, int fd) {
    int flags;
    /*Get origin file flag*/

    if ((flags = fcntl(fd, F_GETFL)) == -1) {
        netPrintError(err, "fcntl(F_GETFL): %s", strerror(errno));
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) ==  -1) {
        netPrintError(err, "fcntl(F_SETFL,O_NONBLOCK): %s", strerror(errno));
        return -1;
    }
    return 0;
}

int netCreateSocket(char *err, int domain) {
    int so, on = 1;
    if (( so = socket(domain, SOCK_STREAM, 0)) == -1) {
        netPrintError(err, "creating socket: %s", strerror(errno));
        return -1;
    }
    /*address reuse*/
    if (setsockopt(so, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
        netPrintError(err, "seting address reuse: %s", strerror(errno));
        return -1;
    }

    return so;
}

int netTcpConnect(char *err, char *addr, int port){
    int so;
    struct sockaddr_in sa;

    if ((so = netCreateSocket(err, AF_INET)) == -1)
        return -1;

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (inet_aton(addr, &sa.sin_addr) == 0) {
        struct hostent *he;
        he = gethostbyname(addr);
        if (he == NULL) {
            netPrintError(err, "can't resolve: %s", addr);
            close(so);
            return -1;
        }
        memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
    }
    if (netSetNonBlock(err, so) == -1) {
        return -1;
    }
    if (connect(so, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
        if (errno = EINPROGRESS)
            return so;
        netPrintError(err, "connect: %s", strerror(errno));
        close(so);
        return -1;
    }
    return so;
}

int netBind(char *err, int fd, struct sockaddr *sa) {
    socklen_t len = sizeof(struct sockaddr_in);
    if (bind(fd, sa, len) == -1) {
        netPrintError(err, "bind: %s", strerror(errno));
        close(fd);
        return -1;
    }
    return 0;
}

int netListen(char *err, int fd, struct sockaddr *sa) {
    socklen_t len = sizeof(struct sockaddr_in);
    if (listen(fd, 511) == -1) {
        netPrintError(err, "listen: %s", strerror(errno));
        close(fd);
        return -1;
    }
    return 0;
}

int netAccept(char *err, int fd, struct sockaddr *sa) {
    int so;
    socklen_t len = sizeof(struct sockaddr_in);
    while (1) {
        so = accept(fd, sa, &len);
        if (so == -1) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                break;
            else
                if (errno == EINTR)
                    continue;
                else {
                    netPrintError(err, "accept: %s", strerror(errno));
                    return -1;
                }
        }
        break;
    }
    return so;
}

int netRead(int fd, char *buf, int count) {
    int nread, total = 0;
    while (total != count) {
        nread = read(fd, buf, count - total);
        if (nread == 0) return total;
        if (nread == -1) {
            if (errno == EINTR)
                continue;
            else if (errno != EAGAIN)
                return -1;
        }
        total += nread;
        buf += nread;
    }
}

int netWrite(int fd, char *buf, int count) {
    int nwritten, total = 0;
    while (total != count) {
        nwritten = write(fd, buf, count - total);
        if (nwritten == 0) return total;
        if (nwritten == -1) {
            if (errno == EINTR)
                continue;
            else if (errno != EAGAIN)
                return -1;
        }
        total += nwritten;
        buf += nwritten;
    }
    return total;
}

int netStartServer(char *err, int port, char *bindaddr) {
    int so;
    struct sockaddr_in sa;
    /*struct sockaddr_in sa;*/

    if ((so = netCreateSocket(err, AF_INET)) == -1)
        return -1;
    //netSetSocketReuse(err, so);
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bindaddr && inet_aton(bindaddr, &sa.sin_addr) == 0) {
        netPrintError(err, "invalid bind address");
        close(so);
        return -1;
    }
    if (netBind(err, so, (struct sockaddr*)&sa) == -1)
        return -1;
    if (netListen(err, so, (struct sockaddr*)&sa) == -1)
        return -1;
    return so;
}
