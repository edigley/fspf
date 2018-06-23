#include "master.h"
#include <stdio.h>
#include "mpi.h"
#include "iniparser.h"
#include "population.h"
#include "mpi.h"
#include "MPIWrapper.h"
#include "myutils.h"
#include "genetic.h"

#define SIM_DEFAULT 0
#define SIM_FARSITE 1
#define SIM_SIM2 2
#define SIM_SIM3 3
#define NUM_CLASSES 5

// variables
char * TracePathFiles;
char *  simulator;
char *  range_file;
char * GenAlErroPath;
int 	numind;
int 	elitism;
float 	pMutation;
float 	pCrossover;
int     numGenerations;
int 	chunkSize;
int     ntasks;
int     cantGrupos;
int     rest;
int     numgen;
int 	crossMethod;
int  worker_busy[32];
int popusizeM=0;
int RecyclePopulations = 0;
int ClassBasedSched = 0;
char * ClassesFile;

// mapas, ficheros que contienen las lineas de fuego
char * start_line;
char * real_line;
char * simulated_line;
char * ClassesLabel;
int  * ClassesCores;
int nClasses = 0;
int CoresXMPIThread = 1;

// dimensiones de los mapas
int Rows;
int Cols;
double CellWd;  // son cuadradas por el momento, leo solo 1 dimension
double CellHt;

// poblacion
char * ClassToReplace;
char * pobini;
char * population_error;
char * final_popu;
char * bests_indv;
char * TrainingDataBasePath;
char * WekaPath;
char * ClassifyPATH;

POPULATIONTYPE p;

static int guidedEllitism;
static int guidedMutation;
// PREDICCIÓN

int num_ind_pred;
int PchunkSize;

// METODO COMPUTACIONAL: 0:NO ARMO TABLAS, 1: ARMO NUEVAS  2: AGREGO EN TABLA EXISTENTE
int armarTabla;
char * fTabla ;
double valorDireccion;  // valores utilizados como amplitud del nuevo subrango
double valorVelocidad;
int doComputacional;
char *alg; // tipo de algoritmo de optimizacion, hasta hoy solo genetico
//int FuelsUsed[260];
char *FuelsToCalibrateFileName;
int CalibrateAdjustments;
int nFuels;
int pending;

struct Popu_Element {
    INDVTYPE_FARSITE * Ind;
    struct Popu_Element * Next;
};
typedef struct Popu_Element element;

struct Classified_Population {
    element * Classes;
};
typedef struct Classified_Population ClassPopu;
ClassPopu * Classified;

/********************************************************************/
// determino como se repartira la poblacion entre los workers
/********************************************************************/

int defineBlockSize() {

    if (chunkSize == 0) {
        chunkSize = (int)(numind / (ntasks - 1));
        cantGrupos = (ntasks - 1);
    } else {
        cantGrupos = (int)(numind / chunkSize);
    }

    rest = numind % (ntasks - 1);

    return (0);

}

int definePredBlockSize(int numIndsPred) {

    if (PchunkSize == 0) {
        PchunkSize = (int)(numIndsPred / (ntasks - 1));
        cantGrupos = (ntasks - 1);
    } else {
        cantGrupos = (int)(numIndsPred / PchunkSize);
    }

    rest = numIndsPred % (ntasks - 1);

    return (0);

}

/********************************************************************/
// MASTER MAIN FUNCTION
/********************************************************************/
int master(char * datosIni, int ntareas, int JobID, double Start) {
    char gen_str[3];
    char new_gen_str[3];
    char *pob_str;
    char *pob_str_errors;
    char *new_pob_str;
    char TraceFileName[500];
    FILE * TraceFile;

    char ErrorFileName[500];
    FILE * ErrorFile;

    numgen = 0;
    double t1,t2,t3,t4,ti,te,te2;
    int nextgen = 0;

    ntasks = ntareas;

    int nworkers = ntasks -1;

    initMaster(datosIni,nworkers);
    sprintf(ErrorFileName,"%sGenAlError_%d.dat",GenAlErroPath,JobID);
    //sprintf(TraceFileName,"%sMasterTrace_%d.dat",TracePathFiles,JobID);
    defineBlockSize();
    t3 = MPI_Wtime();
    printf("Master -Elitism:%d\n",elitism);
    char *ErrorBuffer = (char*)malloc(sizeof(char)*100*numGenerations);
    sprintf(ErrorBuffer,"");
    // char *TraceBuffer = (char*)malloc(sizeof(char)*100*numGenerations);
    // sprintf(TraceBuffer,"");
    init_population(&p, popusizeM,nFuels);
    while(numgen < numGenerations) {
        printf("Master -Elitism:%d\n",elitism);
        t1 = MPI_Wtime();
        sprintf(gen_str,"%d",numgen);
        nextgen = numgen + 1;
        sprintf(new_gen_str,"%d",nextgen);
        pob_str = str_replace(pobini, "$1", gen_str);
        new_pob_str = str_replace(pobini, "$1", new_gen_str);
        pob_str_errors = str_replace(population_error, "$1", gen_str);

        get_population_farsite(&p, pob_str);
        printf("Before genetic\n");
        print_population_farsite(p);

        if (ClassBasedSched) {
            ti = MPI_Wtime();
            NewClassifyPopulationFARSITE(&p,numgen);
            te2 = MPI_Wtime();
            //sprintf(TraceBuffer,"%sMaster %1.2f %1.2f %d %d %d C\n",TraceBuffer,ti-Start,te2-Start,0,8,0);
            ti = MPI_Wtime();
            repartirPoblacionFarsite_Classes(&p, nworkers);
            te = MPI_Wtime();
            //sprintf(TraceBuffer,"%sMaster %1.2f %1.2f %d %d %d D\n",TraceBuffer,ti-Start,te-Start,0,8,0);
        }
        if (!ClassBasedSched) {
            NewClassifyPopulationFARSITE_FAKE(&p,numgen);
            repartirPoblacionFarsite_Classes(&p, nworkers);
        }

        sortPopulationByErrorFarsite(&p);
        t4 = MPI_Wtime();
        sprintf(ErrorBuffer,"%s%f\t%f\n",ErrorBuffer,t4-t3,p.popu_fs[0].error);
        print_population_farsite(p);

        save_population_farsite(p, pob_str_errors);

        t2 = MPI_Wtime();

        if (!(RecyclePopulations)) {
            if(GENETIC_Init_Farsite(elitism,pCrossover,pMutation,range_file,bests_indv,1,0,crossMethod,nFuels)<1) {
                printf("\nERROR Initializing Genetic Algorithm! Exiting...\n");
                return -1;
            }

            if(GENETIC_Algorithm_Farsite(&p, new_pob_str,nFuels,pending)<1) {
                printf("\nERROR Running Genetic Algorithm! Exiting...\n");
                return -1;
            }
        }

        numgen++;

    } //END WHILE

    // PREDICTION STAGE

    get_population_farsite(&p, pob_str_errors);
    definePredBlockSize(num_ind_pred);

    NewClassifyPopulationFARSITE_FAKE(&p,numgen);
    //numind=num_ind_pred;
    //repartirPoblacionFarsite_Classes(&p, nworkers);
    prediccionPoblacionFarsite(&p, nworkers);

    sprintf(gen_str,"%d",numgen);
    pob_str_errors = str_replace(population_error, "$1", gen_str);
    printf("Población predicción final\n");
    print_population_farsite(p);

    save_population_farsite(p, pob_str_errors);

    // END PREDICTION STAGE
    /*
    	if (( TraceFile = fopen(TraceFileName, "w")) == NULL){
    	  	printf("Error opening file 'w' %s\n",TraceFileName);
      	}
    	else{
    		fprintf(TraceFile,"%s",TraceBuffer);
    	}
      */

    if (( ErrorFile = fopen(ErrorFileName, "w")) == NULL)
    {
        printf("Error opening file 'w' %s\n",ErrorFileName);
    }
    else
    {
        fprintf(ErrorFile,"%s",ErrorBuffer);
    }

    //fclose(TraceFile);
    fclose(ErrorFile);
    //free(ErrorBuffer);
    fin_workers(nworkers);
}

int initMaster(char * filename, int nworkers)
{

    FILE * ClassF;

    dictionary * datos;
    datos 	= iniparser_load(filename);

    // fichero y size de individuos (conj de parametros)
    popusizeM = iniparser_getint(datos,"main:population_size",0);
    CalibrateAdjustments = iniparser_getint(datos,"main:CalibrateAdjustments",0);
    FuelsToCalibrateFileName	= iniparser_getstr(datos,"main:FuelsToCalibrate");
    pobini              = iniparser_getstr(datos, "main:initial_population");
    population_error    = iniparser_getstr(datos, "main:population_error");
    range_file          = iniparser_getstr(datos, "main:range_file");
    final_popu          = iniparser_getstr(datos, "main:final_population");
    bests_indv          = iniparser_getstr(datos, "main:bests_indv");
    simulator           = iniparser_getstr(datos, "main:simulator");
    chunkSize           = iniparser_getint(datos, "main:chunkSize",1);
    numind              = iniparser_getint(datos, "main:numind",1);
    GenAlErroPath              = iniparser_getstr(datos, "main:GenAlErroPath");
    TrainingDataBasePath	= iniparser_getstr(datos, "main:TrainingDataBasePath");
    WekaPath			= iniparser_getstr(datos, "main:WekaPath");
    ClassifyPATH		= iniparser_getstr(datos, "main:ClassifyPATH");

    elitism         = iniparser_getint(datos, "genetic:elitism",1);
    pMutation       = iniparser_getdouble(datos, "genetic:pMutation",1.0);
    pCrossover      = iniparser_getdouble(datos, "genetic:pCrossover",1.0);
    numGenerations  = iniparser_getint(datos, "genetic:numGenerations", 1);
    crossMethod 	  = iniparser_getint(datos, "genetic:crossover_method", 1);
    RecyclePopulations = iniparser_getint(datos, "main:RecyclePopulations",1);
    ClassBasedSched		= iniparser_getint(datos, "main:ClassBasedSched",1);
    ClassesFile		= 	iniparser_getstr(datos, "main:ClassesFile");
    CoresXMPIThread		= iniparser_getint(datos, "main:CoresXMPIThread",1);
    TracePathFiles              = iniparser_getstr(datos, "main:TracePathFiles");
    ClassToReplace		= 	iniparser_getstr(datos, "main:ClassToReplace");

    if (ClassBasedSched)
    {
        chunkSize = 1;
    }

    //lee todo lo que necesita del diccionario datos.ini
    real_line = iniparser_getstr(datos, "maps:real_fire_line");
    start_line = iniparser_getstr(datos, "maps:start_fire_line");
    simulated_line = iniparser_getstr(datos, "maps:sim_fire_line");

    //parámetros predicción

    num_ind_pred = iniparser_getint(datos, "prediction:num_ind_pred", 1);
    PchunkSize = iniparser_getint(datos, "prediction:PchunkSize", 1);

    // dimensiones mapas
    Rows = iniparser_getint(datos, "mapssize:rows", 100);
    Cols = iniparser_getint(datos, "mapssize:cols", 100);
    CellWd = iniparser_getdouble(datos, "mapssize:CellWd", 1.0);
    CellHt = iniparser_getdouble(datos, "mapssize:CellHt", 1.0);

    // number of iterations (to evolve the population)
    alg     = iniparser_getstr(datos, "main:algorithm");
    //if (ClassBasedSched){
    //resource * Resources = (resource *)malloc(sizeof(resource)*nworkers);
    if ( (ClassF=fopen(ClassesFile,"r")) == NULL )
    {
        printf("Error loading Classes file %s.\n",ClassesFile);
        exit(0);
    }
    else
    {
        fscanf(ClassF,"NumClasses:%d\n",&nClasses);
        if ((nClasses < 0 || nClasses > 100))
        {
            printf("Invalid classes file.\n");
            exit(0);
        }
        Classified = (ClassPopu *)malloc(sizeof(ClassPopu)*nClasses);
        ClassesLabel = (char *)malloc(sizeof(char)*nClasses);
        ClassesCores = (int *)malloc(sizeof(int)*nClasses);
        int i;
        printf("INFO: Loaded Classes:\n");
        for (i=0; i<nClasses; i++)
        {
            fscanf(ClassF,"%c:%d\n",&(ClassesLabel[i]),&(ClassesCores[i]));
            printf("%c:%d\n",ClassesLabel[i],ClassesCores[i]);
            Classified[i].Classes=NULL;
        }
        fclose(ClassF);
    }


    nFuels=0;
    printf("Calibrate Adjustments:%d \n",CalibrateAdjustments);
    if (CalibrateAdjustments)
    {
        FILE *FuelsToCalibrateFILE;
        int i,nFuel;

        //for (i=0;i<259;i++){FuelsUsed[i]=0;}
        if ((FuelsToCalibrateFILE = fopen(FuelsToCalibrateFileName,"r"))==NULL)
        {
            printf("ERROR:Opening fuels used file.\n");
        }
        else
        {
            //printf("INFO:Used fuels--> ");
            while((fscanf(FuelsToCalibrateFILE,"%d",&nFuel)!=EOF) || (nFuels==257))
            {
                //FuelsUsed[nFuel-1]=1;
                nFuels++;
                //	printf("%d ",nFuel);
            }
            //printf("\n");
        }
        printf("FUELS:%d to calibrate.\n",nFuels);
        fclose(FuelsToCalibrateFILE);
    }
    else
    {
        nFuels=0;
    }

    return (0);
}

/********************************************************************/
// END workers
/********************************************************************/
void fin_workers(int nworkers)
{
    printf("STOP signal to workers.\n");
    int worker_counter = 1;

    for (worker_counter = 1; worker_counter <= nworkers; worker_counter++)
        SendMPI_Finish_Signal(worker_counter);

    //MPI_Send(&flag, 1, MPI_INT, worker, MASTER_TO_WORKER_OK_TAG, MPI_COMM_WORLD);
}

int getAvailCore(int * worker_busy, int size, int core)
{
    int i = 0;
    int min = CoresXMPIThread;
    int pos = -1;
    while ((i < size) && (pos == -1))
    {
        if ((CoresXMPIThread-worker_busy[i]) >= core)
        {
            if (((CoresXMPIThread-worker_busy[i])<=min) )
            {
                min = CoresXMPIThread-worker_busy[i];
                pos = i;
            }
            else
            {
                i++;
            }
        }
        else
        {
            i++;
        }
    }
    //POR AQUI, REVISAR Y PROBAR FUNCION
    if (pos == -1)
    {
        return -1;
    }
    return i;
}

int getAvailWorker(int * worker_busy, int size)
{
    int pos = -1;
    int i = 0;
    while ((i < size) && (pos == -1))
    {
        if (worker_busy[i] == 0)
            pos = i;
        else
            i++;
    }
    return pos;
}

int SearchClassIndex(char let)
{
    int i;
    for (i=0; i<NUM_CLASSES; i++)
    {
        if (ClassesLabel[i] == let)
        {
            return i;
        }
    }
    printf("[W]WARNING:----->Error Class not found!<----\n");
    return 1;
}

int repartirPoblacionFarsite_Classes(POPULATIONTYPE *p, int  nworkers)
{
    int ind_counter = 0;
    //int * worker_busy = (int*)malloc(nworkers*sizeof(int));
    //int worker_busy[nworkers];
    int pos = 0, i,c;
    element * it;
    int block_sended = 0;
    int block_received = 0;
    int restSend = numind;
    int restReceive = numind;
    int individualID = 2;
    int go = 1;
    int id_count = -1;
    int goon = 1;
    int received = 0;
    double StartingTime=MPI_Wtime();
    double NowTime;
    INDVTYPE_FARSITE * poblacion = NULL;
    int allsend=0;
    int indv;
    int para = 1;
    int staticPending=pending;
    if (numgen==0)
    {
        //worker_busy = (int*)malloc(nworkers*sizeof(int));
        for (i=0; i<nworkers; i++)
        {
            worker_busy[i]=0;
        }
    }
    poblacion = p->popu_fs;
    c=nClasses-1;
    printf("Classe mas pesada:%c\n",ClassesLabel[c]);
    c=nClasses-1;
    while ((c>=0) && go)
    {
        if (Classified[SearchClassIndex(ClassesLabel[c])].Classes != NULL)
        {
            it = Classified[SearchClassIndex(ClassesLabel[c])].Classes;
            go = 0;
        }
        else
        {
            c--;
        }
    }
    // printf("Master Pending:%d elitism:%d\n",pending,elitism);
    while ( ((block_received < (numind-(elitism+staticPending))) && (numgen!=0)) || ((block_received < numind) && (numgen==0) && para ) )
    {
    //printf("Master Pending:%d elitism:%d\n",pending,elitism);
    //printf("received:%d block_received:%d block_sended:%d\n",received, block_received,block_sended);
        while ( (((block_sended < numind) && numgen==0 ) && goon) ||  (((block_sended < (numind-(elitism+staticPending))) && numgen!=0 ) && goon)  )
        {
            //if (pos != -1)
            //{
            if (c >= 0)
            {
                received = 1;
                id_count= (it->Ind)->id;
                if (poblacion[id_count].executed == 0)
                {
                    pos = getAvailCore(worker_busy, nworkers,poblacion[id_count].threads);
                    printf("Master tiene posicion %d para %d individuo con %d threads\n",pos,id_count,poblacion[id_count].threads);
                    if (pos != -1)
                    {
                        id_count= (it->Ind)->id;
                        //printf("Master id_count:%d\n",id_count);
                        Master_SendMPI_SetOfIndividualTask(pos + 1, chunkSize, id_count, numgen, numind, poblacion);
                        worker_busy[pos] = worker_busy[pos] + poblacion[id_count].threads;
                        printf("Se envia al Worker %d que ocupa %d cores, %d.\n",pos+1,poblacion[id_count].threads,id_count);
                        block_sended++;
                        printf("Master block_sended:%d\n",block_sended);
                        if (it->Next == NULL)
                        {
                            go = 1;
                            c--;
                            while ((c >= 0) && go)
                            {
                                if (Classified[SearchClassIndex(ClassesLabel[c])].Classes != NULL)
                                {
                                    it = Classified[SearchClassIndex(ClassesLabel[c])].Classes;
                                    printf("Pasamos a clase:%c c=%d SearchCI:%d\n",ClassesLabel[c],c,SearchClassIndex(ClassesLabel[c]));
                                    go = 0;
                                }
                                else
                                {
                                    c--;
                                }
                            }

                        }
                        else
                        {
                            it = it->Next;
                        }
                    }
                    else
                    {
                        //printf("Master says Stop\n");
                        goon = 0;
                    }
                }
                else
                {
                    printf("Master ya ejecutado, paso al siguiente\n");
                    if (it->Next == NULL)
                    {
                        go = 1;
                        c--;
                        while ((c >= 0) && go)
                        {
                            if (Classified[SearchClassIndex(ClassesLabel[c])].Classes != NULL)
                            {
                                it = Classified[SearchClassIndex(ClassesLabel[c])].Classes;
                                printf("Pasamos a clase:%c c=%d SearchCI:%d\n",ClassesLabel[c],c,SearchClassIndex(ClassesLabel[c]));
                                go = 0;
                            }
                            else
                            {
                                c--;
                            }
                        }

                    }
                    else
                    {
                        it = it->Next;
                    }

                }
            }
            //}

        }
        // printf("Cantidad de grupos %d, block_sended %d\n",cantGrupos,block_sended);
        // printf("problema:%d: %d == %d - (%d + %d + 1)\n",numgen,block_sended,numind,elitism,pending);
        if ( (( block_sended == (numind-(elitism+staticPending))) && (numgen!=0)) || ((block_sended == numind) && (numgen==0)) )
        {
            received = 1;
            allsend=1;
        }
        //printf("received:%d block_received:%d block_sended:%d\n",received, block_received,block_sended);
        while((received && (block_received < (block_sended + pending ))) || !goon  )
        {
            //printf("Master espera resultados  numgen:%d.\n",numgen);
            //AQUI DEBO MIRAR CADA x SEGUNDOS SI RECIBO, SI PASA UN LIMITE DE TIEMPO Y NO RECIBO NADA, DEBO RELLENAR EL RESTO DE INDIVIDUOS NO ACABADOS CON
            //INFORMACION QUE INDIQUE QUE NO SE DEBE TENER EN CUENTA EN EL ARLGORITMO GENETICO
            //COMPROBAR SI LLEVO MAS 1080 secuencial
            if ((MPI_Wtime() - StartingTime) > 1300.0)
            {
                printf("Generation Time1:%f\n",MPI_Wtime() - StartingTime);
                block_received = numind;
                received = 0;
                //PONER INDIVIDUOS NO RECIBIDOS A -1
                for(indv=0; indv<numind; indv++)
                {
                    if (poblacion[indv].executed == 0)
                    {
                        poblacion[indv].executed = 2;
                        pending+=1;
                        poblacion[indv].error = 9999.999f;
                        //poblacion[indv].oldid = indv;
                        //poblacion[indv].generation = numgen;
                    }
                }
                printf("Pendientes, pending, para la siguiente generacion:%d. %d Individuos ejecutados en gen:%d\n",pending,block_received,numgen);
                goon = 1;
            }
            else
            {
                pos = Master_ReceiveMPI_IndividualError(block_received, p->popu_fs, numind, chunkSize, &individualID, numgen,&pending);
                if (pos != -1)
                {
                    printf("Generation Time1:%f\n",MPI_Wtime() - StartingTime);
                    //SI RECIBO INDIVIDUO EJECUTADO
                    //---->SI RECIBO DEBO MIRAR SI NO PERTENECEN A LA EJECUCION DE ESTA GENERACION, SI NO ES ASI DEBO GUARDAR EN OTRA POBLACION Y JUNTAR LAS DOS POBLACIONES
                    //AL FINAL DE LA EJECUCION DE ESTA FUNCION
                    worker_busy[pos-1] = worker_busy[pos-1] - poblacion[individualID].threads;
                    poblacion[individualID].executed = 1;
                    printf("Se libera de Worker %d %d cores ejecucion individuo %d con error %1.6f .\n",pos,p->popu_fs[individualID].threads,individualID,p->popu_fs[individualID].error);
                    if (p->popu_fs[individualID].error==0.000000f)
                    {
                        p->popu_fs[individualID].error=9999.99f;
                    }
                    if (poblacion[individualID].generation==numgen)
                    {
                        block_received++;
                    }
                    goon = 1;
                    received = 0;

                }
                else
                {
                    if ( block_received >= (numind-(elitism+staticPending) ) )
                    {
                        goon = 1;
                    }
                }
            }
            //SI SE PASA DE 1080: CONTABILIZAR LOS INDIVIDUOS PENDIENTES, MARCARLOS CON -1 EN EXECUTED DE TAL MANERA QUE BLOCK_RECEIVED SERA = BLOCK_SENDED
        }

    }
    //free(worker_busy);

    printf("TERMINO MASTER\n");
}

int repartirPoblacionFarsite(POPULATIONTYPE *p, int  nworkers)
{
    int ind_counter = 0;
    int * worker_busy = (int*)calloc(nworkers, sizeof(int));
    int pos = 0, i;
    int block_sended = 0;
    int block_received = 0;
    int restSend = numind;
    int restReceive = numind;
    int individualID = -1;

    INDVTYPE_FARSITE * poblacion = NULL;

    poblacion = p->popu_fs;

    while ((pos = getAvailWorker(worker_busy, nworkers)) != -1)
    {
        if(restSend >= chunkSize)
        {
            Master_SendMPI_SetOfIndividualTask(pos + 1, chunkSize, block_sended, numgen, numind, poblacion);
            restSend -= chunkSize;
        }
        else
        {
            Master_SendMPI_SetOfIndividualTask(pos + 1, restSend, block_sended, numgen, numind, poblacion);
            restSend = 0;
        }
        worker_busy[pos] = 1;
        block_sended++;
    }
    while(block_sended < cantGrupos)
    {
        if(restReceive >= chunkSize)
        {
            pos = Master_ReceiveMPI_IndividualError(block_received, p->popu_fs, numind, chunkSize, &individualID,numgen,&pending);
            restReceive -= chunkSize;
        }
        else
        {
            pos = Master_ReceiveMPI_IndividualError(block_received, p->popu_fs, numind, restReceive, &individualID,numgen,&pending);
            restReceive = 0;
        }
        worker_busy[pos - 1] = 0;
        block_received++;

        if(restSend >= chunkSize)
        {
            Master_SendMPI_SetOfIndividualTask(pos, chunkSize, block_sended, numgen, numind, poblacion);
            restSend -= chunkSize;
        }
        else
        {
            Master_SendMPI_SetOfIndividualTask(pos, restSend, block_sended, numgen, numind, poblacion);
            restSend = 0;
        }
        block_sended++;
    }
    while (block_received < cantGrupos)
    {
        if(restReceive >= chunkSize)
        {
            pos = Master_ReceiveMPI_IndividualError(block_received, p->popu_fs, numind, chunkSize, &individualID,numgen,&pending);
            restReceive -= chunkSize;
        }
        else
        {
            pos = Master_ReceiveMPI_IndividualError(block_received, p->popu_fs, numind, restReceive, &individualID,numgen,&pending);
            restReceive = 0;
        }
        worker_busy[pos - 1] = 0;
        block_received++;
    }
    printf("TERMINO MASTER\n");
}

int prediccionPoblacionFarsite(POPULATIONTYPE *p, int  nworkers)
{
    printf("EMPIEZO PREDICCIÓN...\n");
    int ind_counter = 0;
    //int * worker_busy = (int*)malloc(nworkers*sizeof(int));
    int worker_busy[nworkers];
    int pos = 0, i,c;
    element * it;
    int block_sended = 0;
    int block_received = 0;
    int restSend = num_ind_pred;
    int restReceive = num_ind_pred;
    int individualID = 2;
    int go = 1;
    int id_count = -1;
    int goon = 1;
    int received = 0;
    pending=0;
    INDVTYPE_FARSITE * poblacion = NULL;
    poblacion = p->popu_fs;

    for (i=0; i<numind; i++)
    {
        poblacion[i].executed = 0;
        poblacion[i].error = 0;
        poblacion[i].id = i;
    }

    for (i=0; i<nworkers; i++)
    {
        worker_busy[i]=0;
    }

    c=nClasses-1;
    printf("Classe mas pesada:%c\n",ClassesLabel[c]);
    c=nClasses-1;
    while ((c>=0) && go)
    {
        if (Classified[SearchClassIndex(ClassesLabel[c])].Classes != NULL)
        {
            it = Classified[SearchClassIndex(ClassesLabel[c])].Classes;
            go = 0;
        }
        else
        {
            c--;
        }
    }

    while (block_received < num_ind_pred)
    {
        while ( (block_sended < num_ind_pred) && goon)
        {
            if (c >= 0)
            {
                received = 1;
                id_count= (it->Ind)->id;
                if (poblacion[id_count].executed == 0)
                {
                    pos = getAvailCore(worker_busy, nworkers,poblacion[id_count].threads);
                    printf("Master tiene posicion %d para %d individuo con %d threads\n",pos,id_count,poblacion[id_count].threads);
                    if (pos != -1)
                    {
                        id_count= (it->Ind)->id;
                        //printf("Master id_count:%d\n",id_count);
                        Master_SendMPI_SetOfIndividualTask(pos + 1, chunkSize, id_count, numgen, numind, poblacion);
                        worker_busy[pos] = worker_busy[pos] + poblacion[id_count].threads;
                        printf("Se envia al Worker %d que ocupa %d cores, %d.\n",pos+1,poblacion[id_count].threads,id_count);
                        block_sended++;
                        if (it->Next == NULL)
                        {
                            go = 1;
                            c--;
                            while ((c >= 0) && go)
                            {
                                if (Classified[SearchClassIndex(ClassesLabel[c])].Classes != NULL)
                                {
                                    it = Classified[SearchClassIndex(ClassesLabel[c])].Classes;
                                    printf("Pasamos a clase:%c c=%d SearchCI:%d\n",ClassesLabel[c],c,SearchClassIndex(ClassesLabel[c]));
                                    go = 0;
                                }
                                else
                                {
                                    c--;
                                }
                            }

                        }
                        else
                        {
                            it = it->Next;
                        }
                    }
                    else
                    {
                        //printf("Master says Stop\n");
                        goon = 0;
                    }
                }
                else
                {
                    printf("Master ya ejecutado, paso al siguiente\n");
                    if (it->Next == NULL)
                    {
                        go = 1;
                        c--;
                        while ((c >= 0) && go)
                        {
                            if (Classified[SearchClassIndex(ClassesLabel[c])].Classes != NULL)
                            {
                                it = Classified[SearchClassIndex(ClassesLabel[c])].Classes;
                                printf("Pasamos a clase:%c c=%d SearchCI:%d\n",ClassesLabel[c],c,SearchClassIndex(ClassesLabel[c]));
                                go = 0;
                            }
                            else
                            {
                                c--;
                            }
                        }
                    }
                    else
                    {
                        it = it->Next;
                    }

                }
            }

        }

        //printf("Cantidad de grupos %d, block_sended %d\n",cantGrupos,block_sended);
        if ( block_sended < (num_ind_pred+1))
        {
            received = 1;
        }
        while(received && (block_received < block_sended))
        {
            //printf("Master espera resultados  numgen:%d.\n",numgen);
            pos = Master_ReceiveMPI_IndividualError(block_received, p->popu_fs, num_ind_pred, chunkSize, &individualID,numgen,&pending);
            if (pos != -1)
            {
                printf("received: %d block_received :%d block_sended:%d\n",received,block_received,block_sended);
                worker_busy[pos-1] = worker_busy[pos-1] - poblacion[individualID].threads;
                poblacion[individualID].executed = 1;
                printf("Se libera de Worker %d %d cores ejecucion individuo %d con error %1.6f .\n",pos,p->popu_fs[individualID].threads,individualID,p->popu_fs[individualID].error);
                block_received++;
                goon = 1;
                received = 0;
            }
            else
            {
                sleep(1);
            }
        }
    }

    // free(worker_busy);

    printf("TERMINO MASTER\n");
    printf("ACABO PREDICCIÓN...\n");

    return 0;
}

/********************************************************************/
// Distribute individuals in workers
/********************************************************************/
int repartirPoblacion(POPULATIONTYPE *p, int numind, int  nworkers, int cantBloques, int rest, int chunkSize)
{

    double a=1.0;
    int worker,j;
    int nroBloque, bloquesRecibidos, bloquesEnviados;
    int dimBloqueR, nroBloqueR;

    MPI_Status status;

    INDVTYPE *poblacion, *poblacionProcesada;

    poblacion = (INDVTYPE *)malloc(sizeof(INDVTYPE)*numind);
    // para almacenar los bloques que recibo desde los workers
    poblacionProcesada = (INDVTYPE *)malloc(sizeof(INDVTYPE)*numind);

    // poblacion sin procesar
    poblacion = p->popu;

    // primer bucle = todos los nodos libres, asigno 1 grupo por nodo de forma secuencial
    nroBloque=0;
    bloquesEnviados = 0;
    worker = 1; // comienzo desde el primer worker
    // mientras haya workers o bloques para distribuir
    while((worker <= nworkers) && (nroBloque < cantBloques))
    {
        MPI_Send(&chunkSize, 1, MPI_INT, worker, MASTER_TO_WORKER_OK_TAG, MPI_COMM_WORLD);
        MPI_Send(&nroBloque, 1, MPI_INT, worker, MASTER_TO_WORKER_OK_TAG, MPI_COMM_WORLD);
        MPI_Send((char *)(poblacion+(nroBloque*chunkSize)), sizeof(struct indvtype)*chunkSize, MPI_UNSIGNED_CHAR, worker, 1, MPI_COMM_WORLD);
        nroBloque++;
        worker++;
        bloquesEnviados ++;
    }

    // el siguiente while es para un futuro, donde cantBloques > nworkers y los sigo repartiendo "bajo demanda"
    bloquesRecibidos = 0;
    while (bloquesRecibidos < cantBloques)
    {
        // recibo un bloque procesado desde cualquier worker
        MPI_Recv(&dimBloqueR, 1, MPI_INT, MPI_ANY_SOURCE, WORKER_TO_MASTER_OK_TAG, MPI_COMM_WORLD, &status);
        MPI_Recv(&nroBloqueR, 1, MPI_INT, MPI_ANY_SOURCE, WORKER_TO_MASTER_OK_TAG, MPI_COMM_WORLD, &status);
        MPI_Recv((poblacionProcesada+(bloquesRecibidos*chunkSize)), sizeof(struct indvtype)*dimBloqueR, MPI_UNSIGNED_CHAR, status.MPI_SOURCE, WORKER_TO_MASTER_OK_TAG, MPI_COMM_WORLD, &status);
        bloquesRecibidos++;

        //printf("MASTER recibi desde worker %d el bloque %d con dimension %d \n", status.MPI_SOURCE, nroBloqueR, dimBloqueR);

        // si quedan bloques por enviar...
        if (bloquesEnviados < cantBloques)
        {
            MPI_Send(&chunkSize, 1, MPI_INT, worker, MASTER_TO_WORKER_OK_TAG, MPI_COMM_WORLD);
            MPI_Send(&nroBloque, 1, MPI_INT, worker, MASTER_TO_WORKER_OK_TAG, MPI_COMM_WORLD);
            // Carlos B. & Abel C. (worker<-->status.MPI_SOURCE)
            MPI_Send((char *)(poblacion+(nroBloque*chunkSize)), sizeof(struct indvtype)*chunkSize, MPI_UNSIGNED_CHAR, status.MPI_SOURCE, 1, MPI_COMM_WORLD);
            nroBloque++;
            //worker++;
            bloquesEnviados ++;
        }

    } //while bloquesRecibidos < cantBloques == queden bloques por recibir desde los workers

    //printf("fin procesamiento de la generacion numero....\n");

    p->popu = poblacionProcesada;

    free(poblacion);
    free(poblacionProcesada);

}

/*******************************************************************************/
// aplica algoritmo evolutivo sobre la poblacion  por si agrego mas algoritmos
/*******************************************************************************/
int apply_EVOLUTE(POPULATIONTYPE * P)
{

    if (strcmp(alg, "GA") == 0)
    {
        printf("Aplicando GENETICO\n");
        //GENETIC_Algorithm(&p);
    }
}

// me comunico con el worker 0 para que retorne los datos del mapa real
void obtenerValoresPropagacionReal(double * direccionReal, double * velocidadReal, double * distanciaReal)
{

    MPI_Status status;
    int idPrimerWorker = 1; // pues los ids van de 0 a n-1 hence worker0 tiene id=1

    double vec[3];
    // me comunico con el worker 0 para que me devuelva los datos de la propagacion real
    MPI_Recv(vec, 3, MPI_DOUBLE, idPrimerWorker, WORKER_TO_MASTER_OK_TAG, MPI_COMM_WORLD, &status);
    * direccionReal = vec[0];
    * velocidadReal = vec[1];
    * distanciaReal = vec[2];
}

/*****************************************************************************************/
// evoluciono la poblacion tantas veces como se quiera
/*****************************************************************************************/
int evolucionarPoblacionInicial(POPULATIONTYPE *p, int numind, int numGeneraciones, int nworkers, int chunkSize)
{
    int rest, cantBloques;
    int geneActual;

    // determino como se repartira la poblacion entre los workers
    defineBlockSize(nworkers, numind, &rest, &cantBloques, &chunkSize);

    // evoluciono la poblacion
    geneActual = 0;
    while (geneActual < numGeneraciones)
    {
        // en GENETIC_Algorithm escribo la poblacion en generacion.txt luego
        // de realizar la evolucion (seleccin, crossover, mutacion)
        if (geneActual != 0)
            get_population_farsite(p, "generacion.txt");

        repartirPoblacion( p, numind, nworkers, cantBloques, rest, chunkSize);
        save_bestIndv(p,bests_indv);

        // esto lo tendria que hacer pero no en la ultima vuelta??
        apply_EVOLUTE(p);

        // incremento el numero de generacion actual QUE NO IRIA ACA!!!
        p->currentGen = p->currentGen + 1;

        geneActual ++;
    }

    // printf("desde evolucionarPoblacionInicial \n");
    // print_populationScreen(*p);
    fin_workers(nworkers);

}

//FUNCION FAKE, mirar NewClassifyPopulationFARSITE_WORKS
int NewClassifyPopulationFARSITE_FAKE(POPULATIONTYPE * pobla, int numgen)
{
    int i;
    char nt[200];
    char syscall[1000];
    FILE * temp;
    FILE * cagonto;
    char * tempName = (char*)malloc(sizeof(char) * 500);
    printf("BEGIN:Classify population\n");
    /*Establecer la ruta de las clases de Weka*/
    //printf("export CLASSPATH=\"%s:%s:$CLASSPATH\"\n",WekaPath,ClassifyPATH);
    /*
     sprintf(nt,"");
      sprintf(nt,"export CLASSPATH=\"%s:%s:$CLASSPATH\"",WekaPath,ClassifyPATH);
      putenv(nt);
      sprintf(tempName,"");
      sprintf(tempName,"%spob_%d_tmp.arff",ClassifyPATH,numgen);
    */
    for (i=0; i<nClasses; i++)
    {
        Classified[i].Classes = NULL;
    }

    /*
    if ( !((temp = fopen(tempName, "w+")) == NULL  )){
      fprintf(temp,"@RELATION jonquera_biocluster\n\n@ATTRIBUTE WindSpeed NUMERIC\n@ATTRIBUTE WindDir NUMERIC\n@ATTRIBUTE M1 NUMERIC\n@ATTRIBUTE M10 NUMERIC\n@ATTRIBUTE M100 NUMERIC\n@ATTRIBUTE MHerb  NUMERIC\n@ATTRIBUTE tExec {A,B,C,D,E}\n\n@DATA\n");

      for (i = 0; i < pobla->popuSize; i++){ //write each individual
          printf("%1.2f,%1.2f,%1.2f,%1.2f,%1.2f,%1.2f,?\n",pobla->popu_fs[i].parameters[4],pobla->popu_fs[i].parameters[5],pobla->popu_fs[i].parameters[0],pobla->popu_fs[i].parameters[1],pobla->popu_fs[i].parameters[2],pobla->popu_fs[i].parameters[3]);
          fprintf(temp,"%1.2f,%1.2f,%1.2f,%1.2f,%1.2f,%1.2f,?\n",pobla->popu_fs[i].parameters[4],pobla->popu_fs[i].parameters[5],pobla->popu_fs[i].parameters[0],pobla->popu_fs[i].parameters[1],pobla->popu_fs[i].parameters[2],pobla->popu_fs[i].parameters[3]);
      }
      fclose(temp);
    }else{
     printf("Error opening file %s\n",tempName);
    }
    */
    //sprintf(syscall,"java Classifier %s %spob_%d_tmp.arff %spobClass_%d.txt %s A %d model.j48",TrainingDataBasePath,ClassifyPATH,numgen,ClassifyPATH,numgen,ClassToReplace,numgen);
    //printf("%s\n",syscall);
    //int err_syscall = system(syscall);
    //sprintf(tempName,"");
    //sprintf(tempName,"%spobClass_%d.txt",ClassifyPATH,numgen);

    //if ( !((cagonto = fopen(tempName, "r")) == NULL) ){
    for (i = 0; i < pobla->popuSize; i++)  //write each individual
    {
    // fscanf(cagonto,"%f,%f,%f,%f,%f,%f,%c\n",&pobla->popu_fs[i].parameters[4],&pobla->popu_fs[i].parameters[5],&pobla->popu_fs[i].parameters[0],&pobla->popu_fs[i].parameters[1],&pobla->popu_fs[i].parameters[2],&pobla->popu_fs[i].parameters[3],&pobla->popu_fs[i].class_ind);
        pobla->popu_fs[i].class_ind='A';
        pobla->popu_fs[i].threads = 1;
        pushIndividualInClass(&(pobla->popu_fs[i]));
        //printf("%d:--%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.4f %1.4f %c %d\n",pobla->popu_fs[i].id,pobla->popu_fs[i].parameters[0], pobla->popu_fs[i].parameters[1], pobla->popu_fs[i].parameters[2], pobla->popu_fs[i].parameters[3], pobla->popu_fs[i].parameters[4], pobla->popu_fs[i].parameters[5], pobla->popu_fs[i].parameters[6], pobla->popu_fs[i].parameters[7], pobla->popu_fs[i].error, pobla->popu_fs[i].errorc,pobla->popu_fs[i].class_ind,pobla->popu_fs[i].threads);
    }
    // fclose(cagonto);
    //}else{
    //printf("Error opening file %s\n",tempName);
    //}

    printf("END:Classify FAKE population\n");
    //free(tempName);

}

int NewClassifyPopulationFARSITE(POPULATIONTYPE * pobla, int numgen)
{
    int i;
    char nt[400];
    char syscall[1000];
    FILE * temp;
    FILE * cagonto;
    char * tempName = (char*)malloc(sizeof(char) * 2000);
    printf("BEGIN:Classify population\n");
    /*Establecer la ruta de las clases de Weka*/
    printf("export CLASSPATH=\"%s:%s:$CLASSPATH\"\n",WekaPath,ClassifyPATH);
    sprintf(nt,"");
    sprintf(nt,"export CLASSPATH=\"%s:%s:$CLASSPATH\"",WekaPath,ClassifyPATH);
    putenv(nt);
    sprintf(tempName,"");
    sprintf(tempName,"%spob_%d_tmp.arff",ClassifyPATH,numgen);

    for (i=0; i<nClasses; i++)
    {
        Classified[i].Classes = NULL;
    }

    /*Escribir poblacion en archivo con cabezeras para Weka*/
    if ( !((temp = fopen(tempName, "w+")) == NULL  ))
    {
        fprintf(temp,"@RELATION jonquera_biocluster\n\n@ATTRIBUTE WindSpeed NUMERIC\n@ATTRIBUTE WindDir NUMERIC\n@ATTRIBUTE M1 NUMERIC\n@ATTRIBUTE M10 NUMERIC\n@ATTRIBUTE M100 NUMERIC\n@ATTRIBUTE MHerb  NUMERIC\n@ATTRIBUTE tExec {A,B,C,D,E}\n\n@DATA\n");
        // fprintf(temp,"@RELATION jonquera_biocluster\n\n@ATTRIBUTE WindSpeed NUMERIC\n@ATTRIBUTE WindDir NUMERIC\n@ATTRIBUTE M1 NUMERIC\n@ATTRIBUTE M10 NUMERIC\n@ATTRIBUTE M100 NUMERIC\n@ATTRIBUTE MHerb  NUMERIC\n@ATTRIBUTE tExec {A,B,C,D,E}\n\n@DATA\n");
        for (i = 0; i < pobla->popuSize; i++)  //write each individual
        {
            //printf("%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.4f %1.4f\n",pobla->popu_fs[i].m1, pobla->popu_fs[i].m10, pobla->popu_fs[i].m100, pobla->popu_fs[i].mherb, pobla->popu_fs[i].wndvel, pobla->popu_fs[i].wnddir, pobla->popu_fs[i].temp, pobla->popu_fs[i].hum, pobla->popu_fs[i].error, pobla->popu_fs[i].errorc);
            //fprintf(temp,"%1.2f,%1.2f,%1.2f,%1.2f,%1.2f,%1.2f,?\n",pobla->popu_fs[i].wndvel, pobla->popu_fs[i].wnddir,pobla->popu_fs[i].m1, pobla->popu_fs[i].m10, pobla->popu_fs[i].m100, pobla->popu_fs[i].mherb);
            printf("%1.2f,%1.2f,%1.2f,%1.2f,%1.2f,%1.2f,?\n",pobla->popu_fs[i].parameters[4],pobla->popu_fs[i].parameters[5],pobla->popu_fs[i].parameters[0],pobla->popu_fs[i].parameters[1],pobla->popu_fs[i].parameters[2],pobla->popu_fs[i].parameters[3]);
            fprintf(temp,"%1.2f,%1.2f,%1.2f,%1.2f,%1.2f,%1.2f,?\n",pobla->popu_fs[i].parameters[4],pobla->popu_fs[i].parameters[5],pobla->popu_fs[i].parameters[0],pobla->popu_fs[i].parameters[1],pobla->popu_fs[i].parameters[2],pobla->popu_fs[i].parameters[3]);
        }
        fclose(temp);
    }
    else
    {
        printf("Error opening file %s\n",tempName);
    }
    /*Llamada al sistema para utilizar Weka*/

    sprintf(syscall,"java Classifier %s %spob_%d_tmp.arff %spobClass_%d.txt %s A %d model.j48",TrainingDataBasePath,ClassifyPATH,numgen,ClassifyPATH,numgen,ClassToReplace,numgen);
    //sprintf(syscall,"sh %s/replace_individuals.sh pob_tmp.txt %d %s %d",ClassifyPATH,numgen,ClassifyPATH,pobla->popuSize);
    printf("%s\n",syscall);
    int err_syscall = system(syscall);
    /*Se vuelve a cargar la poblacion a memoria*/
    sprintf(tempName,"");
    sprintf(tempName,"%spobClass_%d.txt",ClassifyPATH,numgen);

    if ( !((cagonto = fopen(tempName, "r")) == NULL) )
    {
        // fprintf(temp,"@RELATION jonquera_biocluster\n\n@ATTRIBUTE WindSpeed NUMERIC\n@ATTRIBUTE WindDir NUMERIC\n@ATTRIBUTE M1 NUMERIC\n@ATTRIBUTE M10 NUMERIC\n@ATTRIBUTE M100 NUMERIC\n@ATTRIBUTE MHerb  NUMERIC\n@ATTRIBUTE tExec {A,B,C,D,E}\n\n@DATA\n");
        for (i = 0; i < pobla->popuSize; i++)  //write each individual
        {
            //fscanf(cagonto,"%f,%f,%f,%f,%f,%f,%c\n",&pobla->popu_fs[i].wndvel, &pobla->popu_fs[i].wnddir,&pobla->popu_fs[i].m1, &pobla->popu_fs[i].m10, &pobla->popu_fs[i].m100, &pobla->popu_fs[i].mherb,&pobla->popu_fs[i].class_ind);
            fscanf(cagonto,"%f,%f,%f,%f,%f,%f,%c\n",&pobla->popu_fs[i].parameters[4],&pobla->popu_fs[i].parameters[5],&pobla->popu_fs[i].parameters[0],&pobla->popu_fs[i].parameters[1],&pobla->popu_fs[i].parameters[2],&pobla->popu_fs[i].parameters[3],&pobla->popu_fs[i].class_ind);
            //printf("--%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.4f %1.4f %c %d\n",pobla->popu_fs[i].m1, pobla->popu_fs[i].m10, pobla->popu_fs[i].m100, pobla->popu_fs[i].mherb, pobla->popu_fs[i].wndvel, pobla->popu_fs[i].wnddir, pobla->popu_fs[i].temp, pobla->popu_fs[i].hum, pobla->popu_fs[i].error, pobla->popu_fs[i].errorc,pobla->popu_fs[i].class_ind,pobla->popu_fs[i].threads);
            pobla->popu_fs[i].threads = SearchCoreClass(pobla->popu_fs[i].class_ind);
            //pobla->popu_fs[i].threads = 1;
            pushIndividualInClass(&(pobla->popu_fs[i]));
            printf("%d:--%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.4f %1.4f %c %d\n",pobla->popu_fs[i].id,pobla->popu_fs[i].parameters[0], pobla->popu_fs[i].parameters[1], pobla->popu_fs[i].parameters[2], pobla->popu_fs[i].parameters[3], pobla->popu_fs[i].parameters[4], pobla->popu_fs[i].parameters[5], pobla->popu_fs[i].parameters[6], pobla->popu_fs[i].parameters[7], pobla->popu_fs[i].error, pobla->popu_fs[i].errorc,pobla->popu_fs[i].class_ind,pobla->popu_fs[i].threads);
            //printf("%d:--%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.4f %1.4f %c %d\n",pobla->popu_fs[i].id,pobla->popu_fs[i].m1, pobla->popu_fs[i].m10, pobla->popu_fs[i].m100, pobla->popu_fs[i].mherb, pobla->popu_fs[i].wndvel, pobla->popu_fs[i].wnddir, pobla->popu_fs[i].temp, pobla->popu_fs[i].hum, pobla->popu_fs[i].error, pobla->popu_fs[i].errorc,pobla->popu_fs[i].class_ind,pobla->popu_fs[i].threads);
        }
        fclose(cagonto);
    }
    else
    {
        printf("Error opening file %s\n",tempName);
    }

    printf("END:Classify population\n");
    free(tempName);
    //if (numgen==0){exit(0);}
}

/*****************************************************************************************/
// Se classifica la poblacion en classes utilizando Weka(j48) y una base de entrenamiento.
// Se utilizará EL MISMO método y software (mismo script) que se ha utilizado en la tesis
// de Andrés aunque implique escritura a disco.
/*****************************************************************************************/
int ClassifyPopulationFARSITE_WORKS(POPULATIONTYPE * pobla, int numgen)
{
    int i;
    char nt[200];
    char syscall[1000];
    FILE * temp;
    FILE * cagonto;
    char * tempName = (char*)malloc(sizeof(char) * 500);
    printf("BEGIN:Classify population\n");
    /*Establecer la ruta de las clases de Weka*/
    printf("export CLASSPATH=\"%s:$CLASSPATH\"\n",WekaPath);
    sprintf(nt,"");
    sprintf(nt,"export CLASSPATH=\"%s:$CLASSPATH\"",WekaPath);
    putenv(nt);
    sprintf(tempName,"%s/pob_tmp.txt",ClassifyPATH);

    for (i=0; i<nClasses; i++)
    {
        Classified[i].Classes = NULL;
    }

    /*Escribir poblacion en archivo con cabezeras para Weka*/
    if ( !((temp = fopen(tempName, "w+")) == NULL  ))
    {
        // fprintf(temp,"@RELATION jonquera_biocluster\n\n@ATTRIBUTE WindSpeed NUMERIC\n@ATTRIBUTE WindDir NUMERIC\n@ATTRIBUTE M1 NUMERIC\n@ATTRIBUTE M10 NUMERIC\n@ATTRIBUTE M100 NUMERIC\n@ATTRIBUTE MHerb  NUMERIC\n@ATTRIBUTE tExec {A,B,C,D,E}\n\n@DATA\n");
        for (i = 0; i < pobla->popuSize; i++)  //write each individual
        {
            //printf("%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.4f %1.4f\n",pobla->popu_fs[i].m1, pobla->popu_fs[i].m10, pobla->popu_fs[i].m100, pobla->popu_fs[i].mherb, pobla->popu_fs[i].wndvel, pobla->popu_fs[i].wnddir, pobla->popu_fs[i].temp, pobla->popu_fs[i].hum, pobla->popu_fs[i].error, pobla->popu_fs[i].errorc);
            fprintf(temp,"%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.4f %1.4f\n",pobla->popu_fs[i].m1, pobla->popu_fs[i].m10, pobla->popu_fs[i].m100, pobla->popu_fs[i].mherb, pobla->popu_fs[i].wndvel, pobla->popu_fs[i].wnddir, pobla->popu_fs[i].temp, pobla->popu_fs[i].hum, pobla->popu_fs[i].error, pobla->popu_fs[i].errorc);
        }
        fclose(temp);
    }
    else
    {
        printf("Error opening file %s\n",tempName);
    }
    /*Llamada al sistema para utilizar Weka*/

    sprintf(syscall,"sh %s/replace_individuals.sh pob_tmp.txt %d %s %d",ClassifyPATH,numgen,ClassifyPATH,pobla->popuSize);
    printf("%s\n",syscall);
    int err_syscall = system(syscall);
    /*Se vuelve a cargar la poblacion a memoria*/
    sprintf(tempName,"");
    sprintf(tempName,"%s/Classified.tmp",ClassifyPATH);

    if ( !((cagonto = fopen(tempName, "r")) == NULL) )
    {
        // fprintf(temp,"@RELATION jonquera_biocluster\n\n@ATTRIBUTE WindSpeed NUMERIC\n@ATTRIBUTE WindDir NUMERIC\n@ATTRIBUTE M1 NUMERIC\n@ATTRIBUTE M10 NUMERIC\n@ATTRIBUTE M100 NUMERIC\n@ATTRIBUTE MHerb  NUMERIC\n@ATTRIBUTE tExec {A,B,C,D,E}\n\n@DATA\n");
        for (i = 0; i < pobla->popuSize; i++)  //write each individual
        {
            fscanf(cagonto,"%f %f %f %f %f %f %f %f %f %f %c \n",&(pobla->popu_fs[i].m1), &(pobla->popu_fs[i].m10), &(pobla->popu_fs[i].m100), &(pobla->popu_fs[i].mherb), &(pobla->popu_fs[i].wndvel), &(pobla->popu_fs[i].wnddir), &(pobla->popu_fs[i].temp), &(pobla->popu_fs[i].hum), &(pobla->popu_fs[i].error), &(pobla->popu_fs[i].errorc),&(pobla->popu_fs[i].class_ind));
            //printf("--%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.4f %1.4f %c %d\n",pobla->popu_fs[i].m1, pobla->popu_fs[i].m10, pobla->popu_fs[i].m100, pobla->popu_fs[i].mherb, pobla->popu_fs[i].wndvel, pobla->popu_fs[i].wnddir, pobla->popu_fs[i].temp, pobla->popu_fs[i].hum, pobla->popu_fs[i].error, pobla->popu_fs[i].errorc,pobla->popu_fs[i].class_ind,pobla->popu_fs[i].threads);
            pobla->popu_fs[i].threads = SearchCoreClass(pobla->popu_fs[i].class_ind);
            pushIndividualInClass(&(pobla->popu_fs[i]));
            //printf("%d:--%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.4f %1.4f %c %d\n",pobla->popu_fs[i].id,pobla->popu_fs[i].m1, pobla->popu_fs[i].m10, pobla->popu_fs[i].m100, pobla->popu_fs[i].mherb, pobla->popu_fs[i].wndvel, pobla->popu_fs[i].wnddir, pobla->popu_fs[i].temp, pobla->popu_fs[i].hum, pobla->popu_fs[i].error, pobla->popu_fs[i].errorc,pobla->popu_fs[i].class_ind,pobla->popu_fs[i].threads);
        }
        fclose(cagonto);
    }
    else
    {
        printf("Error opening file %s\n",tempName);
    }


    printf("END:Classify population\n");
    // free(tempName);
    //if (numgen==0){exit(0);}
}

int pushIndividualInClass(INDVTYPE_FARSITE * indv)
{
    int aux = 0;
    element * it;
    element * new = (element *)malloc(sizeof(element));
    new->Ind = indv;
    new->Next = NULL;
    if (Classified[SearchClassIndex(indv->class_ind)].Classes == NULL)
    {
        Classified[SearchClassIndex(indv->class_ind)].Classes = new;
        //printf("Individuo tipo %c insertado tras %d iteraciones. %f %f\n",indv->class_ind,aux,new->Ind->m1,new->Ind->wndvel);
        return 0;
    }
    /*
    for(it=(Classified[SearchClassIndex(indv.class_ind)].Classes);it->Next!=NULL;it = it->Next){
      aux++;
    }
    */
    it =  (Classified[SearchClassIndex(indv->class_ind)].Classes);
    while (it->Next != NULL)
    {
        it = it->Next;
    }
    //printf("Individuo tipo %c insertado tras %d iteraciones. %f %f\n",indv->class_ind,aux,new->Ind->m1,new->Ind->wndvel);
    it->Next = new;
    return 0;
}

int SearchCoreClass(char let)
{
    int i;
    for (i=0; i<NUM_CLASSES; i++)
    {
        if (ClassesLabel[i] == let)
        {
            return ClassesCores[i];
        }
    }
    printf("[W]WARNING:----->Error Class not found!<----\n");
    return 1;
}
