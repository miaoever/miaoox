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

#include "miaoo-x.h"
#include "net.h"
#include "ev.h"
#include "worker.h"

static void InitMasterConfig() {
    master->max_worker_num = MAX_WORKER_NUM;
    master->max_event_num = MAX_EVENT_NUM;
    master->alive_worker_num = 0;
    master->listenfd = 0;
    master->poll = 0;

    master->workers = malloc(sizeof(Worker) * master->max_worker_num);
    if (master->workers == NULL){
        mLog(MSG_WARNING, "Memory allocate failed.");
    }
}

static void InitTCPServer() {
    char err[255];
    master->listenfd =  netStartServer(err, master->port, master->bind_addr);
    listenfd = master->listenfd;
    if (master->listenfd < 0) {
        mLog(MSG_WARNING, "Start TCP server failed on %d with: %s.", master->listenfd, err);
        return;
    }
    mLog(MSG_NOTICE, "Server is listening on port %d", master->port);
}

static void DispatchConn(EventLoop* eventloop, int fd, void* client_data, int mask) {
    int idx = master->max_worker_num;
    int isValid = 0;
    int notice = 0;
    Master* master = (master*)client_data;

    do {
        if ( master->workers[idx].pid != -1) {
            isValid = 1;
            break;
        }
        idx = (idx + 1) % master->max_worker_num;
    } while(idx != master->max_worker_num)
    /*No valid worker*/
    if (!isValid) {
        master->stop = 1;
        return;
    }
    //Notice the worker for the new connection
    send(master->workers[i].pipefd[0], (char*)&notice, sizeof(int), 0);
}

static void SetNonBlocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
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

static void DispatchSignal(int signal) {
    int err = errno;
    int msg = signal;
    send(signal_pipefd[1], (char*)&msg, 1, 0);
    errno = err;
}

static void Handle_CHLD() {
    pid_t pid;
    int stat, j;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        /*If worker[j] exited, mark it*/
        for (j = 0; j < master->max_worker_num) {
            if (master->workers[j].pid == pid) {
                close(master->workers[j].pipefd[0]);
                master->alive_worker_num --;
                master->worker[j] = -1;
            }
        }
    }
    if (master->alive_worker_num == 0) {
        master->stop = 1;
    }
}

static void Handle_INT() {
    pid_t pid;
    int i;
    /*Kill all the workers*/
    for (i = 0; i < master->max_worker_num; i++) {
        pid = master->workers[i].pid;
        if (pid != -1) {
            kill(pid, SIGTERM);
        }
    }
}

static void MasterSignalHandler() {
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

/*Unified the signal processing to eventloop*/
static void InitMasterSignals() {
    struct sigaction act, oact;
    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, signal_pipefd);
    SetNonBlocking(signal_pipefd[1]);
    assert(ret != -1);

    evCreateIOEvent(master->poll, signal_pipefd[0], EV_READABLE, MasterSignalHandler, master);
    
    SignalRegister(SIGCHLD, DispatchSignal, 1);
    SignalRegister(SIGTERM, DispatchSignal, 1);
    SignalRegister(SIGINT, DispatchSignal, 1);
    SignalRegister(SIGPIPE, SIG_IGN, 1)
}

static void CreateWorkers() {
    int i, retval;
    pid_t pid;
    int num_worker = master->max_worker_num;
    for (i = 0; i < master->max_worker_num; i++) {
        retval = socketpair(PF_UNIX, SOCK_STREAM, 0, master->workers[i].pipefd);
        assert(retval == 0);
        master->workers[i] = malloc(sizeof(Worker));
        //assert((pid = fork()) => 0);
        pid = fork();
        if (pid > 0) {
            close(master->workers[i].pipefd[1]);
            master->workers[i].pid = pid;
            master->alive_worker_num ++;
            continue;
        } else if (pid == 0){
            close(master->workers[i].pipefd[0]);
            InitWorker(master->workers[i]);
            break;
        } else {
            mLog(MSG_WARNING, "Failed to create workers.");
            return;
        }
    }
}

static void FreeMaster() {
    FreeWokers(master->workers);
    evDeleteEventLoop(master->poll);
    free(master);
}

static void FreeWorkers() {
    int i;
    for (i = 0; i < master->max_num_worker; i++) {
        free(master->workers[i]);
    }
}

int main(int argc, char** args) {
    master = malloc(sizeof(Master));

    if (master == NULL){
        mLog(MSG_WARNING, "Memmory allocate failed when creating Master.");
        return -1;
    }
    InitMasterConfig();
    InitTCPServer();
    CreateWorkers();
    if ((master->poll = evCreateEventLoop(master->max_event_num)) == NULL) {
        mLog(MSG_WARNING, "Memmory allocate failed when creating Event-loop.");
        return -1;
    }
    InitMasterSignals();
    evCreateIOEvent(master->poll, master->listenfd, EV_READABLE, DispatchConn, master);

    while (!master->stop) {
        evProcessEvents(master->poll,EV_IO_EVENTS);
    }
    FreeMaster();
    return 0;
}
