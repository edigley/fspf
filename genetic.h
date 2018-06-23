#ifndef _GENETIC_H_
#define _GENETIC_H_

#include "population.h"

int GENETIC_Init(int eli, double crossP, double mutaP, char * fRange, char * fBests, int guidedMutation, int guidedEllitism, int doCompu);
int GENETIC_Init_Farsite(int eli, double crossP, double mutaP, char * fRange, char * fBests, int guidedMutation, int guidedEllitism, int doCompu,int nFuels);
int GENETIC_InitComputacional(double wndDirComp, double wndSpdComp, double valorD, double valorV);
int GENETIC_Algorithm(POPULATIONTYPE * p, char* outputFilename, int nFuels);
int GENETIC_Algorithm_Farsite(POPULATIONTYPE * p, char* outputFilename, int nFuels, int pend);
int GENETIC_Crossover(INDVTYPE p1, INDVTYPE p2, INDVTYPE * c1, INDVTYPE * c2);
int GENETIC_Crossover_Farsite(INDVTYPE_FARSITE pin1, INDVTYPE_FARSITE pin2, INDVTYPE_FARSITE * c1, INDVTYPE_FARSITE * c2);
int GENETIC_Mutation(INDVTYPE * indv, int guidedMutaFlag, double wnddir, double wndvel);
int GENETIC_Mutation_Farsite(INDVTYPE_FARSITE * child);

#endif //_GENETIC_H_
