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

//Carlos B.
//NOTE: Define your own simulator ID and custom functions & structs

//Carlos B.
//argv[1] --> Archivo de parámetros de la simulación

//int simID=0;





//Carlos B.
int main(int argc,char *argv[])
{
    /*
        char       dataIniFile[500];
        int  	myID, ntasks;
        int  	namelen;
    ///    char 	processor_name[MPI_MAX_PROCESSOR_NAME];
    	 double t1,t2;
    	 time_t start, end;
    	 double duration = 0;
        int JobID;
        int cpu;
        int FuelsN=0;
    */
    //double origin = MPI_Wtime();
    int provided;
    int         myID, ntasks;
    MPI_Init_thread(&argc,&argv, MPI_THREAD_SINGLE, &provided);
    MPI_Comm_size(MPI_COMM_WORLD,&ntasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&myID);

    char      * dataIniFile;
    //int         myID, ntasks;
    int         namelen;
///    char     processor_name[MPI_MAX_PROCESSOR_NAME];
    double t1,t2;
    time_t start, end;
    double duration = 0;
    int JobID;
    int FuelsN=0;
//    MPI_Init(&argc,&argv);
    printf("Start!\n");
    time(&start);
    //MPI_Comm_size(MPI_COMM_WORLD,&ntasks);
    //MPI_Comm_rank(MPI_COMM_WORLD,&myID);
    //char hostname[2048];
    struct utsname userinfo;
    int cpu;

    double origin = MPI_Wtime();
    JobID = atoi(argv[1]);
    dataIniFile = argv[2];
    //strcpy(dataIniFile,argv[2]);
    t1 = MPI_Wtime();
    //printf("EXECUTION STARTS!:%d\n",myID);
    //return 1;
    if(myID == MASTER_ID)
    {
        if(uname(&userinfo)>=0)
        {
            cpu = sched_getcpu();
            printf("\n***** Master System Details ******\nSystem Name    : %s\nSystem Node    : %s\nSystem Release : %s\nSystem Version : %s\nSystem Machine : %s\nCPU:%d\n",userinfo.sysname,userinfo.nodename,userinfo.release,userinfo.version,userinfo.machine,cpu);
        }
        else
            printf("\nSystem details fetch failed..\n");
        if (argc == 3)
        {
            switch(provided)
            {
            case MPI_THREAD_SINGLE:
                printf("MPI_THREAD_SINGLE\n");
                break;
            case MPI_THREAD_FUNNELED:
                printf("MPI_THREAD_FUNNELED\n");
                break;
            case MPI_THREAD_SERIALIZED:
                printf("MPI_THREAD_SERIALIZED\n");
                break;
            case MPI_THREAD_MULTIPLE:
                printf("MPI_THREAD_MULTIPLE\n");
                break;
            }
            //double origin = MPI_Wtime();
            master(dataIniFile, ntasks,JobID,origin);
        }
        else
        {
            printf("program <initial_data_filename>");
        }
    }
    else // worker
    {
//		printf("Soy el worker %d\n", myID);
        if(uname(&userinfo)>=0)
        {
            cpu = sched_getcpu();
            printf("\n***** Worker %d System Details ******\nSystem Name    : %s\nSystem Node    : %s\nSystem Release : %s\nSystem Version : %s\nSystem Machine : %s\nCPU:%d\n",myID,userinfo.sysname,userinfo.nodename,userinfo.release,userinfo.version,userinfo.machine,cpu);
        }
        else
        {
            printf("\nSystem details fetch failed..\n");
        }
        worker(myID, dataIniFile,JobID,origin);
    }
    //t2 = MPI_Wtime();
    // printf("*************SPIF total time: %1.2f**************\n",t2-t1);
    printf("MPI thread %d waits and end.\n",myID);
    MPI_Barrier(MPI_COMM_WORLD);
    sleep(2);
    if (myID == 0)
    {
        time(&end);
        duration = difftime(end, start);
        printf("*************SPIF total time in C: %1.2f**************\n",duration);
    }

    MPI_Finalize();
    MPI_Abort(MPI_COMM_WORLD,0);
    return 0;
}
