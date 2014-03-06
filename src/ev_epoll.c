#include <sys/epoll.h>

typedef strcut evPoll{
    int epfd;
    struct epoll_event* events;
}evPoll;

static int evPollCreate(evEventLoop *eventloop) {
    evPoll* poll = malloc(sizeof(Poll));

    if (!poll) return -1;

    poll->events = malloc(sizeof(struct epoll_event) * eventloop->setsize);
    if (!poll->events) {
        free(poll);
        return -1;
    }
    poll->epfd = epoll_create(1024);
    if (poll->epfd == -1) {
        free(poll->events);
        free(poll);
        return -1;
    }
    eventloop->poll = poll;
    return 0;
}

static void evPollFree(evEventLoop* eventloop) {
    evPoll* poll = eventloop->poll;

    close(poll->epfd);
    free(poll->events);
    free(poll);
}

static int evPollAddEvent(evEventLoop* eventloop, int fd, int mask){
    evPoll* poll = eventloop->poll;
    struct epoll_event ee;

    int op = eventloop->events[fd].mask == AE_NONE ? EPOLL_CTL_ADD : EPOLL_CTL_ADD;

    ee.events = 0;
    mask |= eventloop->events[fd].mask;
    if (mask & AE_READABLE) ee.events |= EPOLLIN;
    if (mask & AE_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.u64 = 0; /* avoid valgrind warning */
    ee.data.fd = fd;
    if (epoll_ctl(poll->epfd,op,fd,&ee) == -1) return -1;
    return 0;
}

static int evPollDelEvent(evEventLoop* eventloop, int fd, int delmask) {
    evPoll* poll = eventloop->poll;
    struct epoll_event ee;
    int mask = eventloop->events[fd].mask & (~delmask);

    ee.events = 0;
    if (mask & AE_READABLE) ee.events |= EPOLLIN;
    if (mask & AE_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.u64 = 0; /* avoid valgrind warning */
    ee.data.fd = fd;
    if (mask != AE_NONE) {
        epoll_ctl(poll->epfd,EPOLL_CTL_MOD,fd,&ee);
    } else {
        epoll_ctl(poll->epfd,EPOLL_CTL_DEL,fd,&ee);
    }
    return 0;
}

static int evPollLoop(evEventLoop* eventloop) {
    evPoll* poll = eventloop->poll;
    int retval, numevents = 0, j;

    retval = epoll_wait(poll->epfd, poll->events, eventloop->setsize, -1);
    if (retval > 0) {
        numevents = retval;
        for (j = 0; j < numevents; j++) {
            int mask = 0;
            struct epoll_event* e = poll->events[j];

            if (e->events & EPOLLIN) mask |= AE_READABLE;
            if (e->events & EPOLLOUT) mask |= AE_WRITABLE;
            if (e->events & EPOLLERR) mask |= AE_WRITABLE;
            if (e->events & EPOLLHUP) mask |= AE_WRITABLE;
            eventloop->fired[j].fd = e->data.fd;
            eventloop->fired[j].mask = mask;
        }
    }
    return numevents;
}

static char* evPollName(void) {
    return "epoll";
}




