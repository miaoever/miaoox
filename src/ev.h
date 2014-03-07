#ifndef __EV_H__
#define __EV_H__

#define EV_OK 0
#define EV_ERR -1

#define EV_NONE 0
#define EV_READABLE 1
#define EV_WRITABLE 2

#define EV_IO_EVENTS 1

struct evEventLoop;

typedef void IOPrecess(struct evEventLoop* eventloop, int fd, void* clientData, int mask);
typedef int evBeforesleepProc(struct evEventLoop* eventloop);

typedef struct evIOEvent {
    int mask;
    IOProcess* ReadProcess;
    IOProcess* WriteProcess;
    void* clientData;
}IOEvent;

typedef struct FiredEvent {
    int fd;
    int mask;//AE_WRITABLE or AE_READABLE
}FiredEvent;

typedef struct evEventLoop {
    int maxfd;
    int setsize;
    IOEvent* events;
    FireeEvent* fired;
    int stop;
    void* polldata;

    evBeforeSleepProc* beforesleep;
}evEventLoop;

EventLoop* evCreateEventLoop(int setsize);
void evDeleteEventLoop(evEventLoop* eventloop);
void evStop(evEventLoop* eventloop);
int evCreateIOEvent(evEventloop* eventloop, int fd, int mask, IOProcecc* proc, void* clientData);
void evDelIOEvent(evEventLoop* eventloop, int fd, int mask);
int evGetIOEnvents(evEventLoop* eventloop, int fd);
int evProcessEvents(evEventLoop* eventloop, int flags);
void evMain(evEventLoop* eventloop);
void evSetBeforeSleepProc(evEventLoop* eventloop, evBeforeSleepProc* beforesleep);

#endif
