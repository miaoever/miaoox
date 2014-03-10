#ifndef __MIAOOX__
#define __MIAOOX__

typedef struct Master {
    int max_work_num;
    int max_event_num;
    int alive_workers_num;
    int listenfd;
    int stop;
    int signal_pipefd[2];
    evEventLoop poll;
    Worker* workers;
}Master;

static void InitMasterConfig();
static void InitTCPServer();
void DispatchConn(EventLoop* eventloop, int fd, void* client_data, int mask);
static void SetNonBlocking(int fd);
static void SignalRegister(int signal, void (handler)(int), int restart);
static void DispatchSignal(int signal);
static void MasterSignalHandler();
static void InitMasterSignals();
static void CreateWorkers();
static void FreeMaster();
static void FreeWorkers();
int main(int argc, char** args);

#endif
