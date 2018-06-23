/*
 * File:   farsite.h
 * Author: carlos
 *
 * Created on 16 de abril de 2012, 14:17
 */

#ifndef FARSITE_H
#define	FARSITE_H

int runSimFarsite(INDVTYPE_FARSITE individuo, char * simID, double * err, int numgen, char * atm, char * datos, int myid,double Start,char * TracePathFiles, int JobID,int executed,int proc,int Trace, int FuelsN, int *FuelsUsed,double  AvailTime);
void createInputFiles(INDVTYPE_FARSITE *individuo, char * dataFile);

#endif	/* FARSITE_H */

