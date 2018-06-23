#define MASTER_TO_WORKER_OK_TAG 1
#define WORKER_TO_MASTER_OK_TAG 1
#define MASTER_TO_WORKER_FAILED_TAG 0
#define WORKER_TO_MASTER_FAILED_TAG 0
#define MASTER_ID 0

void worker(int taskid, char * datosIni,int JobID, double Start);
