#include <stdio.h>
#include <string.h>
#include "mpi.h"
#include "strlib.h"
#include "master.h"
#include "worker.h"
#include "population.h"
#include "genetic.h"
#include <time.h>
#include <sys/utsname.h>
#define _GNU_SOURCE
#include <utmpx.h>
#include "iniparser.h"

#define MASTER_ID 0

/**
 * Carlos B.
 * NOTE: Define your own simulator ID and custom functions & structs
 * argv[1] --> Archivo de parámetros de la simulación
 * int simID=0;
 *
 * - argv[1] job id: Job identifier
 * - argv[2] data ini file: SPIF scenario file (ini extension)
 */
int main(int argc,char *argv[]) {

    int provided;
    int myID, ntasks;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &ntasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &myID);

    int namelen;
    double t1, t2;
    time_t start, end;
    double duration = 0;
    int FuelsN=0;
    printf("INFO: Main.main -> Start!\n");
    time(&start);
    struct utsname userinfo;
    int cpu;
    
    double origin = MPI_Wtime();
    
    int JobID;
    char * dataIniFile;
    JobID = atoi(argv[1]);
    dataIniFile = argv[2];

    t1 = MPI_Wtime();
    if(myID == MASTER_ID) {
        if(uname(&userinfo) >= 0) {
            cpu = sched_getcpu();
            printf("INFO: Main.main -> \n***** Master System Details ******\nSystem Name    : %s\nSystem Node    : %s\nSystem Release : %s\nSystem Version : %s\nSystem Machine : %s\nCPU:%d\n",
                userinfo.sysname, userinfo.nodename, userinfo.release, userinfo.version, userinfo.machine, cpu
            );
        } else {
            printf("WARNING: Main.main -> \nSystem details fetch failed..\n");
        }
        if (argc == 3) {
            switch(provided) {
                case MPI_THREAD_SINGLE:
                    printf("INFO: Main.main -> MPI_THREAD_SINGLE\n");
                    break;
                case MPI_THREAD_FUNNELED:
                    printf("INFO: Main.main -> MPI_THREAD_FUNNELED\n");
                    break;
                case MPI_THREAD_SERIALIZED:
                    printf("INFO: Main.main -> MPI_THREAD_SERIALIZED\n");
                    break;
                case MPI_THREAD_MULTIPLE:
                    printf("INFO: Main.main -> MPI_THREAD_MULTIPLE\n");
                    break;
            }
            master(dataIniFile, ntasks, JobID, origin);
        } else {
            printf("ERROR: Main.main -> program <initial_data_filename>");
        }
    }
    else { // worker
        if(uname(&userinfo) >= 0) {
            cpu = sched_getcpu();
            printf("INFO: Main.main -> \n***** Worker %d System Details ******\nSystem Name    : %s\nSystem Node    : %s\nSystem Release : %s\nSystem Version : %s\nSystem Machine : %s\nCPU:%d\n",
                myID, userinfo.sysname, userinfo.nodename, userinfo.release, userinfo.version, userinfo.machine, cpu
            );
        } else {
            printf("INFO: Main.main -> \nSystem details fetch failed..\n");
        }
        worker(myID, dataIniFile, JobID, origin);
    }
    printf("INFO: Main.main -> MPI thread %d waits and end.\n", myID);
    MPI_Barrier(MPI_COMM_WORLD);
    sleep(2);
    if (myID == 0) {
        time(&end);
        duration = difftime(end, start);
        printf("INFO: Main.main -> *************SPIF total time in C: %1.2f**************\n",duration);
    }

    MPI_Finalize();
    MPI_Abort(MPI_COMM_WORLD, 0);

    return 0;
}
