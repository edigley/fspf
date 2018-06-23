#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <mpi.h>
#include "population.h"
#include "MPIWrapper.h"

#define WORKTAG 4
#define DIETAG 5
#define RESULTAG 6

void SendMPI_Finish_Signal(int DEST)
{
    char  buffer[TAM_BUFFER];
    int posicion = 0;
    int signal = FINISH_SIGNAL;

    //printf("MASTER ENVIA FINISH SIGNAL TO %d\n", DEST);
    MPI_Pack(&signal, 1, MPI_INT,buffer,TAM_BUFFER,&posicion,MPI_COMM_WORLD);
    MPI_Send(buffer, TAM_BUFFER, MPI_PACKED, DEST, DIETAG, MPI_COMM_WORLD);
}

void Master_SendMPI_SetOfIndividualTask(int DEST, int chunkSize, int nroBloque, int num_generation, int num_individuals, INDVTYPE_FARSITE * poblacion)
{
    //printf("Master Send to %d with chunkSize:%d nroBloque:%d num_generation:%d num_individuals:%d\n", DEST, chunkSize, nroBloque, num_generation, num_individuals);

    char buffer[TAM_BUFFER];
    int posicion = 0;
    int signal = 0;



    MPI_Pack(&signal, 1, MPI_INT, buffer, TAM_BUFFER, &posicion, MPI_COMM_WORLD);
    MPI_Pack(&nroBloque, 1, MPI_INT, buffer, TAM_BUFFER, &posicion, MPI_COMM_WORLD);
    MPI_Pack(&num_generation, 1, MPI_INT, buffer, TAM_BUFFER, &posicion, MPI_COMM_WORLD);
    poblacion[nroBloque].generation=num_generation;
    poblacion[nroBloque].oldid=nroBloque;

    MPI_Send(buffer, posicion, MPI_PACKED, DEST,DIETAG, MPI_COMM_WORLD);

    /*
        printf("Tamaño de la estructura:%d\n", sizeof(INDVTYPE_FARSITE));

        printf("POBLACIÓN MASTER\n");
    */
    int pos = nroBloque*chunkSize;
    /*
          int i;
          for(i=0;i<chunkSize;i++)
          {
            printf("Ind.%d m1:%1.3f m10:%1.3f m100:%1.3f mherb:%1.3f ws:%1.0f wd:%1.0f temp:%1.0f hum:%1.0f error:%1.1f\n", poblacion[pos+i].id, poblacion[pos+i].m1,poblacion[pos+i].m10,poblacion[pos+i].m100,poblacion[pos+i].mherb,poblacion[pos+i].wndvel,poblacion[pos+i].wnddir, poblacion[pos+i].temp,poblacion[pos+i].hum,poblacion[pos+i].error);
          }
    */
    INDVTYPE_FARSITE * temp = (INDVTYPE_FARSITE *)malloc(sizeof(INDVTYPE_FARSITE )*chunkSize);
    memcpy(temp,&(poblacion[nroBloque]),sizeof(INDVTYPE_FARSITE)*chunkSize);
    //printf("Master envia a worker %d %d individuo que necesita %d threads\n",DEST,chunkSize,poblacion[nroBloque*chunkSize].threads);

    MPI_Send((temp), sizeof(INDVTYPE_FARSITE) * chunkSize, MPI_UNSIGNED_CHAR, DEST, WORKTAG, MPI_COMM_WORLD);
    //sleep(5);
}

INDVTYPE_FARSITE  * Worker_ReceivedMPI_SetOfIndividualTask(int chunkSize, int * nroBloque, int * num_generation, int num_individuals, int * signal, int wait,int * received)
{
    char  buffer[TAM_BUFFER];
    //char * buffer2 = (char *)malloc(sizeof(char) * TAM_BUFFER);
    //printf("RMPI:in\n");
    INDVTYPE_FARSITE * poblacion = NULL;
    int posicion = 0;
    char * poblaci = NULL;
    char *msgGrupo;
    MPI_Status  status;
    int sig_value = -1;
    int flag = 0;
    int myid;
    MPI_Request request;


    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    ////printf("COMM:Recibo no bloqueante de Worker %d a 0.\n",myid);
    //MPI_Recv(buffer2, TAM_BUFFER,MPI_PACKED,0,DIETAG,MPI_COMM_WORLD, &Status);

    if (wait)
    {
        MPI_Recv(buffer, TAM_BUFFER,MPI_PACKED,0,DIETAG,MPI_COMM_WORLD, &status);
        flag=1;
    }
    else
    {
    #ifdef MPE_ACTIVE
        MPE_Stop_log();
    #endif
        //sleep(1);
        MPI_Iprobe(0,DIETAG,MPI_COMM_WORLD,&flag,&status);
    #ifdef MPE_ACTIVE
        MPE_Start_log();
    #endif
    }


    /*
    MPI_Irecv(buffer,TAM_BUFFER,MPI_PACKED,0,DIETAG,MPI_COMM_WORLD, &request);

    if (wait){
      //printf("COMM:Espera bloqueante de Worker %d a 0.\n",myid);
      MPI_Wait(&request, &status);
      //printf("RMPI:Error1\n");
      flag = 1;
      //MPI_Cancel(request);
      //MPI_Request_free(&request);
    }else{
      //printf("request:%d\n",(request));
      ////printf("COMM:Espera no bloqueante de Worker %d a 0.\n",myid);
      MPI_Test(&request, &flag, &status);
      //MPI_Cancel(&request);
      //printf("FAIL\n");
    }
    */

    if (flag != 0)
    {
        if (!wait)
        {
            MPI_Recv(buffer, TAM_BUFFER,MPI_PACKED,0,DIETAG,MPI_COMM_WORLD, &status);
        }
        //printf("Recibo!");
        *received = 1;
        MPI_Unpack(buffer, TAM_BUFFER, &posicion, &sig_value, 1, MPI_INT, MPI_COMM_WORLD);
        //printf("COMM:Recibo de 0 a Worker %d con senal %d FINISH:%d.\n",myid,sig_value,FINISH_SIGNAL);
        *signal = sig_value;

        if (!(sig_value == FINISH_SIGNAL))
        {
            MPI_Unpack(buffer, TAM_BUFFER, &posicion, nroBloque, 1, MPI_INT, MPI_COMM_WORLD);
            MPI_Unpack(buffer, TAM_BUFFER, &posicion, num_generation, 1, MPI_INT, MPI_COMM_WORLD);
            msgGrupo = (char *)malloc(sizeof(INDVTYPE_FARSITE) * chunkSize);
            //printf("COMM:Espera bloqueante Worker %d.\n",myid);
            MPI_Recv(msgGrupo, sizeof(INDVTYPE_FARSITE) * chunkSize, MPI_UNSIGNED_CHAR, 0, WORKTAG, MPI_COMM_WORLD, &status);
            poblaci = (char *)malloc(chunkSize * sizeof(INDVTYPE_FARSITE));
            //poblaci = (INDVTYPE_FARSITE*) msgGrupo;
            memcpy(poblaci,msgGrupo,sizeof(INDVTYPE_FARSITE) * chunkSize);
            poblacion = (INDVTYPE_FARSITE*) poblaci;
            //printf("COMM:Recibo de Master Worker %d individuo %d con %d threads.\n",myid,poblacion->id,poblacion->threads);

            //printf("From Master TO Worker:%d with chunkSize:%d nroBloque:%d Num_gen:%d num_individuals:%d\n", myid, chunkSize, *nroBloque, *num_generation, num_individuals);
        }
        /*
          if ((!wait) && !(flag)){
        MPI_Cancel(&request);
          }
        */
    }
    // printf("RMPI:out\n");
    //free(buffer2);

    return poblacion;
}

void Worker_SendMPI_IndividualError(INDVTYPE_FARSITE * poblacion, int chunk_size)
{
    //int myid;
    //MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    //printf("COMM:Envio error de Worker %d de individuo %d con error %f.\n",myid,poblacion->id, poblacion->error);
    INDVTYPE_FARSITE temp;
    memcpy(&temp,poblacion,sizeof(INDVTYPE_FARSITE));
    //MPI_Ssend((char*)(&temp), sizeof(INDVTYPE_FARSITE) * chunk_size, MPI_UNSIGNED_CHAR, 0, RESULTAG, MPI_COMM_WORLD);
    MPI_Send((char*)(&temp), sizeof(INDVTYPE_FARSITE), MPI_UNSIGNED_CHAR, 0, RESULTAG, MPI_COMM_WORLD);
    //sleep(20);
    //printf("Envio");
    //MPI_Send((char*)poblacion, sizeof(INDVTYPE_FARSITE) * chunk_size, MPI_UNSIGNED_CHAR, poblacion->id, TAG, MPI_COMM_WORLD);
}

int Master_ReceiveMPI_IndividualError(int block_count, INDVTYPE_FARSITE * poblacion, int num_individuals, int chunkSize,int * individual,int currentgen, int *pend)
{
    int i;
    //printf("Comienzo a recibir de un worker\n");
    MPI_Status  status;
    char  buffer[TAM_BUFFER];
    int flag = 0;
    int indv;
    int found=0;

    #ifdef MPE_ACTIVE
        //MPE_Stop_log();
    #endif
        //sleep(1);
    MPI_Iprobe(MPI_ANY_SOURCE,RESULTAG,MPI_COMM_WORLD,&flag,&status);
    #ifdef MPE_ACTIVE
        //MPE_Start_log();
    #endif

    if (flag != 0)
    {

        unsigned char * msgGrupo;
        msgGrupo = (unsigned char *)malloc(sizeof(INDVTYPE_FARSITE) * chunkSize);

        MPI_Recv(msgGrupo, sizeof(INDVTYPE_FARSITE) * chunkSize, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE, RESULTAG, MPI_COMM_WORLD, &status);
        INDVTYPE_FARSITE * temp = (INDVTYPE_FARSITE *)malloc(sizeof(INDVTYPE_FARSITE) * chunkSize);
        memcpy(temp,msgGrupo,sizeof(INDVTYPE_FARSITE) * chunkSize);


        //memcpy(&(poblacion[temp[i]),&msgGrupo,sizeof(INDVTYPE_FARSITE) * chunkSize);
        //free(msgGrupo);
        for(i = 0; i < chunkSize; i++)
        {
            //poblacion[block_count*chunkSize + i].error = temp[i].error;
            //printf("Recibido ind:%d del worker:%d con error:%1.4f\n", temp[i].id, status.MPI_SOURCE, temp[i].error);
            if (temp[i].generation==currentgen)
            {
                poblacion[temp[i].id].error = temp[i].error;
                //*individual = temp[i].id;
                memcpy(individual,&(temp[i].id),sizeof(int));
            }
            else
            {
                *pend=*pend-1;
                printf("Recibo individiuo pendiente id:%d gen:%d\n",temp[i].id,temp[i].generation);
                printf("Individuals:%d Generation:%d\n",num_individuals,temp[i].generation);
                printf("Pending actualizado:%d\n",*pend);
                for(indv=0; indv<num_individuals; indv++)
                {
                    if (poblacion[indv].generation==temp[i].generation && poblacion[indv].oldid == temp[i].id)
                    {
                        poblacion[indv].error = temp[i].error;
                        //*individual = temp[i].id;
                        memcpy(individual,&(indv),sizeof(int));
                        found=1;
                    }//else{
                    //printf("FATAL ERROR:not identified individual error received\n");
                    //}
                }
                if (!found)
                {
                    printf("FATAL ERROR:not identified individual %d gen %d error %1.6f  received\n",temp[i].id,temp[i].generation,temp[i].error);
                }
            }
        }
        //free(temp);
        //(poblacion+(block_count*chunkSize)) = (INDVTYPE_FARSITE*) msgGrupo;

        //MPI_Recv(poblacion+(block_count*chunkSize), sizeof(INDVTYPE_FARSITE) * chunkSize, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE, TAG, MPI_COMM_WORLD, &status);
        //printf("Recibo del worker %d\n", status.MPI_SOURCE);
    }
    else
    {
        return -1;
    }

    return status.MPI_SOURCE;

}

