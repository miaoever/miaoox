#ifndef NET_H
#define NET_H

static void netPrintError(char *err, const char *format, ...);
int netSetNonBlock(char *err, int fd);
int netCreateSocket(char *err, int domain);
void netSetSocketReuse(char *err, int fd);
int netTcpConnect(char *err, char *addr, int port);
int netBind(char *err, int fd, struct sockaddr *sa);
int netListen(char *err, int fd, struct sockaddr *sa);
int netAccept(char *err, int fd, struct sockaddr *sa);
int netRead(int fd, char *buf, int count);
int netWrite(int fd, char *buf, int count);
int netStartServer(char *err, int port, char *bindaddr);

#endif
