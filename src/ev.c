#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <time.h>

#include "ev.h"
#include "ev_epoll.h"

EventLoop* evCreateEventLoop(int setsize) {
    evEventLoop* eventloop;
    int i;

    if ((eventloop = malloc(sizeof(*eventloop))) == NULL) goto err;

    eventloop->events = malloc(sizeof(IOEvents) * setsize);
    eventloop->fired = malloc(sizeof(FiredEvents) * setsize);
    if (eventloop->events == NULL || eventloop->fired == NULL) goto err;
    eventloop->setsize = setsize;

    eventloop->stop = 0;
    eventloop->maxfd = -1;
    eventloop->beforesleep = NULL;
    if (evPollCreate(eventloop) == -1) goto err;

    for (i = 0; i < setsize; i++) {
        eventloop->events[i].mask = EV_NONE;
    }

    return eventloop;

err:
    if (eventloop) {
        free(eventloop->events)
        free(eventloop->fired);
        free(eventloop);
    }
    return NULL;
}

void evDeleteEventLoop(evEventLoop* eventloop) {
    evPollFree(eventloop->poll);
    free(eventloop->events);
    free(eventloop->fired);
    free(eventloop);
}

void evStop(evEventLoop* eventloop) {
    eventloop->stop = 1;
}

int evCreateIOEvent(evEventloop* eventloop, int fd, int mask, IOProcecc* proc, void* clientData) {
    if (fd >= eventloop->setsize) return EV_ERR;
    IOEvent *io = &eventloop->events[fd];

    if (evPollAddEvent(eventloop, fd, mask) == -1)
        return EV_ERR;

    //Add mask(event type) to io->mask
    io->mask |= mask;
    if (mask & EV_READABLE) io->ReadProcecc = proc;
    if (mask & EV_WRITABLE) io->WriteProcecc = proc;

    io->clientData = clientData;

    if (fd > eventloop->maxfd)
        eventloop->maxfd = fd;

    return EV_OK;
}

void evDelIOEvent(evEventLoop* eventloop, int fd, int mask) {
    if (fd >= eventloop->setsize) return;
        IOEvent *io = &eventloop->events[fd];

    if (io->mask == EV_NONE) return;
    //Set io->mask to mask
    io->mask = io->mask & (~mask);
    /*If current fd is the last one in the event list, and have no other event on it*/
    if (fd == eventloop->maxfd && fe->mask == EV_NONE) {
        /*Update the max fd*/
        int j;

        for (j = eventloop->maxfd - 1; j >= 0; j--) {
            if (eventloop->events[j].mask != EV_NONE) break;
        }
        eventloop->maxfd = j;
    }

    evPollDelEvent(eventloop, fd, mask);
}

int evGetIOEnvents(evEventLoop* eventloop, int fd) {
    if (fd >= eventloop->setsize) return 0;
    IOEvent *io = &eventloop->events[fd];

    return io->mask;
}

int evProcessEvents(evEventLoop* eventloop, int flags) {
    int processed = 0, numevents;

    if (!(flags & EV_IO_EVENTS)) return 0;

    if (eventloop != -1) {
        /*Get the number of fired events*/
        numevents = evPollLoop(eventloop);
        for (j = 0; j < numevents; j++) {
            IOEvents* io = &eventloop->events[eventloop->fired[j].fd];

            int mask = eventloop->fired[j].mask;
            int fd = eventloop->fired[j].fd;
            int rfired = 0;

            if (io->mask & mask & EV_READBLE) {
                rfired = 1;
                io->ReadProcecc(eventloop, fd, io->clientData, mask);
            }
            if (io->mask & mask & EV_WRITABLE) {
                if (!rfired || io->WriteProcess != io->ReadProcess) {
                    io->WriteProcess(eventloop, fd, io->clientData, mask);
                }
            }
            processed ++;
        }
    }
    /*return the number of processed events*/
    return processed;
}

void evMain(evEventLoop* eventloop) {
    eventloop->stop = 0;

    while (!eventloop->stop) {
        if (eventloop->beforesleep != NULL) {
            eventloop->beforesleep(eventloop);
        }
        evProcessEvents(eventLoop, EV_IO_EVENTS);
    }
}

void evSetBeforeSleepProc(evEventLoop* eventloop, evBeforeSleepProc* beforesleep) {
    eventloop->beforesleep = beforesleep;
}

char* evGetPollName(void) {
    return evPollName();
}
