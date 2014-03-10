#ifndef __WORKER_H__
#define __WORKER_H__

#define MAX_EVENT_NUM 1024

typedef struct Worker{
    int max_event_num;
    pid_t pid;
    int stop;
    EventLoop* poll;
    int connfd;
    int pipefd[2];
    int signal_pipefd[2];
} Worker;

Worker* worker;

static void AcceptClient();
static void SetNonBlocking(int fd);
static void Protocol_Dispatch(Eventloop* eventloop, int fd, void* client_data, int mask);
static void DispatchSignal(int signal);
static void SignalRegister(int signal, void (handler)(int), int restart);
static void InitWorkerSignals();
static void Handle_CHLD();
static void Handle_INT();
static void WorkerSignalHandler();
int InitWorker(Worker* w);

#endif
