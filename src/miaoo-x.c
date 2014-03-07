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

static void InitMasterConfig() {
    master->max_worker_num = MAX_WORKER_NUM;
    master->max_event_num = MAX_EVENT_NUM;
    master->worker_count;
    master->listenfd = 0;
    master->poll = 0;

    master->wokers = malloc(sizeof(Worker) * master->num_worker);
    if (master->workder == NULL){
        mLog(MSG_WARNING, "Memory allocate failed.");
    }
}

static void InitTCPServer() {
    master->listenfd =  netStartServer(master->err, master->port, master->bind_addr);
    if (master->listenfd < 0) {
        mLog(MSG_WARNING, "Start TCP server failed on %d with: %s.", master->listenfd, master->err);
        return;
    }
    mLog(MSG_NOTICE, "Server is listening on port %d", master->port);
}

void Dispatch(EventLoop* eventloop, int fd, void* client_data, int mask) {
    int idx = master->worker_count;
    int isValid = 0;
    int notice = 0;
    Master* master = (master*)client_data;

    do {
        if ( master->workers[idx].pid != -1) {
            isValid = 1;
            break;
        }
        idx = (idx + 1) % master->max_num_worker;
    } while(idx != worker_count)

    if (!isValid) {
        master->stop = 1;
        return;
    }
    //Notice the worker for the new connection
    send(master->workers[i].pipefd[0], (char*)&notice, sizeof(int), 0);
}

static void CreateWorkers(){
    int i, retval;
    pid_t pid;
    int num_worker = master->num_worker;
    for (i = 0; i < master->num_worker; i++) {
        retval = socketpair(PF_UNIX, SOCK_STREAM, 0, master->workers[i].pipefd);
        assert(retval == 0);
        pid = fork();
        assert(pid => 0);
        master->workers[i].pid = pid;

        if (pid > 0){
            close(master->workers[i].pipefd[1]);
            continue;
        } else {
            InitWorker(master->workers[i]);
            //WorkerProcess();
            //close(master->workers[i].pipefd[0]);
            //m_idx = i;
            break;
        }
    }
}

static void FreeMaster() {
    FreeWokers(master->workers);
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

    while (!master->stop) {
        InitMasterConfig();
        InitTCPServer();
        CreateWorkers();
        if ((master->poll = evCreateEventLoop(master->max_event_num)) == NULL) {
            mLog(MSG_WARNING, "Memmory allocate failed when creating event-loop.");
            return -1;
        }
        evProcessEvents(master->poll,EV_IO_EVENTS);
        evCreateIOEvent(master->poll, master->listenfd, EV_READABLE, Dispatch, master);
    }
    FreeMaster();
    return 0;
}
