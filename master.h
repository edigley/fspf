#define MASTER_TO_WORKER_OK_TAG 1
#define WORKER_TO_MASTER_OK_TAG 1
#define MASTER_TO_WORKER_FAILED_TAG 0
#define WORKER_TO_MASTER_FAILED_TAG 0
#define MASTER_ID 0

#include "population.h"

int master(char * datosIni, int ntasks, int JobID, double Start);
int defineBlockSize();
int definePredBlockSize();
int initMaster(char *a,int b);
void fin_workers(int nworkers);
int repartirPoblacion(POPULATIONTYPE *p, int numind, int  nworkers, int cantBloques, int rest, int chunkSize);
int repartirPoblacionFarsite(POPULATIONTYPE *p, int  nworkers);
int repartirPoblacionFarsite_Classes(POPULATIONTYPE *p, int  nworkers);
int prediccionPoblacionFarsite(POPULATIONTYPE *p, int  nworkers);
int get_population_farsite(POPULATIONTYPE * pobla, char * nombreInitSet);
int get_population_default(POPULATIONTYPE * pobla, char * nombreInitSet);
int evolucionarPoblacionInicial(POPULATIONTYPE *p, int numind, int numGeneraciones, int nworkers, int chunkSize);
int ClassifyPopulationFARSITE(POPULATIONTYPE * pobla, int numgen);
int NewClassifyPopulationFARSITE(POPULATIONTYPE * pobla, int numgen);
int SearchCoreClass(char let);

