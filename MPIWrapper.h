/*
 * File:   MPIWrapper.h
 * Author: carlos
 *
 * Created on 24 de abril de 2012, 15:40
 */
#include <stdio.h>
#include <mpi.h>
#include "population.h"

#ifndef MPIWRAPPER_H
#define	MPIWRAPPER_H

#define FINISH_SIGNAL 999
#define TASK_TO_DO 1
#define TAG 0
#define TAM_BUFFER 500

void SendMPI_Finish_Signal(int DEST);
void Master_SendMPI_SetOfIndividualTask(int DEST, int chunkSize, int nroBloque, int num_generation, int num_individuals, INDVTYPE_FARSITE *poblacion);
//INDVTYPE_FARSITE * Worker_ReceivedMPI_SetOfIndividualTask(int chunkSize, int * nroBloque, int * num_generation, int num_individuals, int * signal);
INDVTYPE_FARSITE * Worker_ReceivedMPI_SetOfIndividualTask(int chunkSize, int * nroBloque, int * num_generation, int num_individuals, int * signal, int wait,int * received);
void Worker_SendMPI_IndividualError(INDVTYPE_FARSITE * poblacion, int chunk_size);
int Master_ReceiveMPI_IndividualError(int block_count, INDVTYPE_FARSITE * poblacion, int num_individuals, int chunkSize, int * individual,int currentgen, int * pend);


#endif	/* MPIWRAPPER_H */

