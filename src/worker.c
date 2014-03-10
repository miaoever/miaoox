#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "net.h"
#include "ev.h"


static void AcceptClient() {
    char err[255];
    worker->connfd = netAccept(err, listenfd, &worker->cli_addr, &worker->cli_addr_len);
    if (worker->connfd < 0) {
        mLog(MSG_WARNING, "Accept client failed.", err);
        return;
    }
    evCreateIOEvent(worker->poll, worker->connfd, EV_READABLE, Protocol_Dispatch, NULL);
}

static void SetNonBlocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
}

static void Protocol_Dispatch(Eventloop* eventloop, int fd, void* client_data, int mask) {
    //Worker* worker = (worker*)client_data;
    //port = get_port(worker->cli_addr);
    //addr = get_addr(worker->cli_addr);
    //handler = proto[addr][port];
    //handler();
}

static void DispatchSignal(int signal) {
    int err = errno;
    int msg = signal;
    send(signal_pipefd[1], (char*)&msg, 1, 0);
    errno = err;
}

static void SignalRegister(int signal, void (handler)(int), int restart) {
    struct sigaction act, old_act;
   // memeset(&act, '\0', sizeof(struct sigaction));
    sigemptyset(%act.sa_mask);
    act.sa_handler = handler;
    if (restart == 1) {
        act.sa_flags |= SA_RESTART;
    }
    sigfillset(&act.sa_mask);
    assert(sigaction(sig, &act, &old_act));
}

static void InitWorkerSignals() {
    struct sigaction act, oact;
    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, signal_pipefd);
    SetNonBlocking(signal_pipefd[1]);
    assert(ret != -1);

    evCreateIOEvent(worker->poll, signal_pipefd[0], EV_READABLE, WorkerSignalHandler, NULL);

    SignalRegister(SIGCHLD, DispatchSignal, 1);
    SignalRegister(SIGTERM, DispatchSignal, 1);
    SignalRegister(SIGINT, DispatchSignal, 1);
    SignalRegister(SIGPIPE, SIG_IGN, 1)
}

static void Handle_CHLD() {
    pid_t pid;
    int stat, j;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        return;
    }
}

static void Handle_INT() {
    worker->stop = 1;
}

static void WorkerSignalHandler() {
    int i;
    char signals[1024];
    int ret = recv(signal_pipefd[0], signals, sizeof(signals), 0);
    if (ret <= 0)
        return;
    for (i = 0; i < ret; i++) {
        switch(signals[i]) {
            case SIGCHLD: {
                Handle_CHLD();
                break;
            }
            case SIGTERM:
            case SIGINT: {
                Handle_INT();
                break;
            }
            default: {
                break;
            }
        }
    }
}

int InitWorker(Worker* w) {
    worker = w;
    worker->stop = 0;
    worker->max_event_num = MAX_EVENT_NUM;

    if (worker == NULL){
        mLog(MSG_WARNING, "Memmory allocate failed when creating Worker.");
        return -1;
    }
    if ((worker->poll = evCreateEventLoop(worker->max_event_num)) == NULL) {
        mLog(MSG_WARNING, "Memmory allocate failed when creating Event-loop.");
        return -1;
    }
    AcceptClient();
    InitWorkerSignals();

    while (!worker->stop) {
        evProcessEvents(worker->poll, EV_IO_EVENTS);
    }

    return 0;
}
