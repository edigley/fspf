#include "mpi.h"
#include "worker.h"
#include <stdio.h>
#include "population.h"
#include <stdlib.h>
#include "iniparser.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "farsite.h"
#include "MPIWrapper.h"
#include "windninja.h"
#include "myutils.h"
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
//#include <omp.h>

#define INFINITY 999999999.
#define PRINTRESULTS 1
#define PRINTDATOSREALES 1

char * TracePathFiles;
char *  simulator;
int 	numind;
float 	elitism;
float 	pMutation;
float 	pCrossover;
int	numGenerations;
int 	chunkSize;
int     doWindFields;
int     doMeteoSim;
char    *alg;
//int     myid;
int Trace;
int FireSimLimit;

char    *elevfilename;
char    *prjfilename;
char    *elevfilepath;
char    *wn_output_path;
char    *baseAtmFile, *atmFileNew, *atmFile;
char    *resolution;
char    *VGeneral;
char    *VGrid;
char    *windinit;
char    *vegetation;
int     wn_num_theads;
char    *wn_path;
char    *atm_file;
char *landscapeName;
char *landscapePath;
// poblacion
char * 	pobini;
char * final_popu;
char * bests_indv;

// mapas, ficheros que contienen las lineas de fuego
char * start_line;
char * real_line;
char * simulated_line;

// mapas
static double * real_map_t1;
static double * init_map_t0;
static double * ign_map;

// dimensiones de los mapas
static int Rows;
static int Cols;
static double CellWd;  // son cuadradas por el momento, leo solo 1 dimension
static double CellHt;

// tiempos t0 y t1
static double start_time;
static double final_time;

// analisis del mapa real, lo realiza cada worker y evito enviarlo por msg
static double direccion, velocidad, dist,fit, elapsedTime;
static double noIgnValue,distFeets, error;
static int p1x, p1y, p2x, p2y; // puntos de max propagacion del mapa real para enviar a runsim

// estas sirven para los metodos analitico y computacional propagacion real!!
static double dirPropagacionReal;
static double distanciaRealFeets;
static double velocidadRealFeets;
// especifico del computacional
static int doComputacional;
static char * fTabla;
static int armarTabla;  // 0: NO ARMAR  1:ARMAR NUEVA  2: AGREGAR EN TABLA YA CREADA
// esto lo necesito para el COMPUTACIONAL para armar las tablas
static int Model;
static double Slope;

double comm_time = 0;
int FuelsUsed[259];
char *FuelsToCalibrateFileName;
int CalibrateAdjustments;
int nFuelsW=0;

//prototipos (si cal)
int  getFinalFireLine(char * real_line, int Rows, int Cols, double * realMap);
static int PrintMap (double * map, char *fileName, int Rows, int Cols);


/*************************************************************************************/
// ninicializa los datos del worker, mapas y tiempos
/*************************************************************************************/
int initWorker(char * datafile)
{
    dictionary * datos;
    datos 	= iniparser_load(datafile);
    int i=0;
    // fichero y size de individuos (conj de parametros)
    printf("%d\n",++i);
    //
//    CalibrateAdjustments = iniparser_getint(datos,"main:CalibrateAdjusments",0);
    printf("%d\n",++i);
    FuelsToCalibrateFileName	= iniparser_getstr(datos,"main:FuelsToCalibrate");
    printf("%d\n",++i);
    landscapeName = iniparser_getstr(datos,"farsite:landscapeName");
    printf("%d\n",++i);
    landscapePath = iniparser_getstr(datos,"farsite:landscapePath");
    printf("%d\n",++i);
    pobini  	= iniparser_getstr(datos, "main:initial_population");
    printf("%d\n",++i);
    final_popu 	= iniparser_getstr(datos, "main:final_population");
    printf("%d\n",++i);
    bests_indv 	= iniparser_getstr(datos, "main:bests_indv");
    printf("%d\n",++i);
    simulator   = iniparser_getstr(datos, "main:simulator");
    printf("%d\n",++i);
    chunkSize   = iniparser_getint(datos, "main:chunkSize",1);
    printf("%d\n",++i);
    numind          = iniparser_getint(datos, "main:numind",1);
    doWindFields    = iniparser_getint(datos, "main:doWindFields", 0);
    doMeteoSim    = iniparser_getint(datos, "main:doMeteoSim", 0);
    TracePathFiles              = iniparser_getstr(datos, "main:TracePathFiles");
    Trace		= iniparser_getint(datos,"main:Trace",1);

    printf("%d\n",++i);
    elitism         = iniparser_getint(datos, "genetic:elitism",1);
    pMutation       = iniparser_getdouble(datos, "genetic:pMutation",1.0);
    pCrossover      = iniparser_getdouble(datos, "genetic:pCrossover",1.0);
    numGenerations  = iniparser_getint(datos, "genetic:numGenerations", 1);

    //lee todo lo que necesita del diccionario datos.ini
    real_line = iniparser_getstr(datos, "maps:real_fire_line");
    start_line = iniparser_getstr(datos, "maps:start_fire_line");
    simulated_line = iniparser_getstr(datos, "maps:sim_fire_line");

    // dimensiones mapas
    Rows = iniparser_getint(datos, "mapssize:rows", 100);
    Cols = iniparser_getint(datos, "mapssize:cols", 100);
    CellWd = iniparser_getdouble(datos, "mapssize:CellWd", 1.0);
    CellHt = iniparser_getdouble(datos, "mapssize:CellHt", 1.0);

    printf("%d\n",++i);
    // number of iterations (to evolve the population)
    alg     = iniparser_getstr(datos, "main:algorithm");

    // windninja
    elevfilename    = iniparser_getstr(datos, "windninja:elevfilename");
    prjfilename    = iniparser_getstr(datos, "windninja:prjfilename");
    wn_output_path  = iniparser_getstr(datos, "windninja:wn_output_path");
    elevfilepath   = iniparser_getstr(datos, "windninja:elevfilepath");
    atm_file   = iniparser_getstr(datos, "windninja:atmFile");
    FireSimLimit  = iniparser_getint(datos, "farsite:ExecutionLimit", 1);
    printf("%d\n",++i);
    if (CalibrateAdjustments)
    {
        FILE *FuelsToCalibrateFILE;
        int i,nFuel;
        nFuelsW=0;
        for (i=0; i<259; i++)
        {
            FuelsUsed[i]=0;
        }
        if ((FuelsToCalibrateFILE = fopen(FuelsToCalibrateFileName,"r"))==NULL)
        {
            printf("ERROR:Opening fuels used file.\n");
        }
        else
        {
            //printf("INFO:Used fuels--> ");
            while( (fscanf(FuelsToCalibrateFILE,"%d",&nFuel)!=EOF) || (nFuelsW == 257) )
            {
                FuelsUsed[nFuel-1]=1;
                nFuelsW++;
                //	printf("%d ",nFuel);
            }
            //printf("\n");
        }
        printf("FUELS:%d to calibrate.\n",nFuelsW);
        fclose(FuelsToCalibrateFILE);

    }
    else
    {
        nFuelsW = 0;
    }


    return (0);
}

//solo se lee el mapa real en t1 xq  el otro se lee en runsim
int getMaps()
{

    // aloco memoria para los mapas
    if ( (real_map_t1 = (double *)calloc(Rows*Cols, sizeof(double))) == NULL
            || (init_map_t0 = (double *)calloc(Rows*Cols, sizeof(double))) == NULL
            || ((ign_map = (double *)calloc(Rows*Cols, sizeof(double)))) == NULL )
    {
        printf("No aloca lugar para real_map, init_map o ign_map \n");
        return -1;
    }

// el mapa inicial se lee en fireSim, aca solo mapa real
    //printf("Leyendo el mapa real en t1 desde %s \n", real_line);
    getFinalFireLine(real_line, Rows, Cols, real_map_t1);

    PrintMap(real_map_t1, "mapaT1", Rows, Cols);

    return 1;
}



/**************************************************************************/
// PROCESAMIENTO DE UN INDIVIDUO llamado desde procesarBloque
/**************************************************************************/
int procesarIndividuo(INDVTYPE *individuo, char * nombre_init_map_t0, double start_time, double *  real_map_t1, double final_time, int Rows, int Cols)
{

    double wnddir, wndvel;
    int p1x, p1y, p2x, p2y;
    int p1xAux, p1yAux, p2xAux, p2yAux;
    double fit, error;

    print_indv_default(*individuo);

    // llamo al simulador con el individuo y

    // Carlos B. & Abel C
    // system(command); to call a external synchronous process

    //Carlos B.*****runsim(*individuo, ign_map, dirPropagacionReal, distanciaRealFeets, elapsedTime, p1x, p1y, p2x, p2y, &wnddir, &wndvel);
    // PrintMap(ign_map, "simulado.map", Rows, Cols);


// valor de celdas no quemadas en ignMap==INFINITY
    noIgnValue = 999999999.;
    // faltan declarar todas estas variables
    //Carlos B.*****dist = distanciaMaxPropagacionReal(ign_map, Rows, Cols, start_time, final_time, &direccion, &velocidad, noIgnValue, &elapsedTime, CellHt, &p1xAux, &p1yAux, &p2xAux, &p2yAux);
    //Carlos B.*****fit = fitnessYError(real_map_t1, ign_map, Rows, Cols, start_time, final_time, &error);
#if PRINTRESULTS
    printf("****************************** RESULTS ********************************** \n");
    printf("Distancia: %f feets  (%f metros) \n ", dist, (dist / 3.28083));
    printf("Direccion: %f \n ", direccion);
    printf("Velocidad: %f (feets/min) %f (metros/min)\n ", velocidad, (velocidad)/3.28083 );
    printf("Fitness mapas: real: %s, time: %f y  time: %f \n", real_line, start_time, final_time);
    printf("Fitness: %f -- Error: %f \n", fit, error);
    printf("*********************************************************************** \n");
#endif

    individuo->fit = fit;
    individuo->dir = direccion;
    individuo->dist = dist;
    individuo->vel = velocidad;
    individuo->error = error;
    individuo->wnddir = wnddir;
    individuo->wndvel = wndvel;

    return 1;
}

/**************************************************************************/
// PROCESAMIENTO DEL BLOQUE ENVIADO
/**************************************************************************/
void procesarBloque(INDVTYPE * individuos, int dimBloque)
{
    int i;

    for (i=0; i<dimBloque; i++)
    {
        procesarIndividuo(&individuos[i], start_line, start_time, real_map_t1, final_time, Rows, Cols);
        //   individuos[i].error = 1.0;
    }

}

void procesarBloqueFarsite(INDVTYPE_FARSITE * individuos, int numgen, char * datos, int myid,double Start,char * TracePathFiles, int JobID,int executed, int proc,int Trace,double AvailTime)
{
    int i;
    float v,d;
    double err;
    struct stat st;
    char * path_output;
    char * atmPath;
    char * buffer = (char*)malloc(sizeof(char) * 500);

    //printf("procesarBloqueFarsite::El chunksize es %d\n",chunkSize);
    for (i=0; i<chunkSize; i++)
    {
        //print_indv_farsite(individuos[i]);
        if (doWindFields == 1 && doMeteoSim == 0)
        {
            sprintf(buffer,"%d", individuos[i].id);
            path_output = str_replace(wn_output_path,"$1", buffer);

            if(stat(path_output, &st) == 0)
                deleteFilesFromFolder(path_output,"*");
            else
                createFolder(path_output);
            // link a elevfilename
            createLinkToFile(landscapeName,landscapePath, path_output);
            createLinkToFile(prjfilename, landscapePath, path_output);

            v = individuos[i].parameters[5];
            d = individuos[i].parameters[6];
            int idInd = individuos[i].id;

            atmPath = runWindNinja(path_output, v, d, idInd, datos);
        }
        //printf("WorkerAvailTime:%lf\n",AvailTime);
        //printf("Ruta trazas:%s\n",TracePathFiles);
        if(doMeteoSim == 0)
            runSimFarsite(individuos[i], "FARSITE", &err, numgen, atmPath, datos, myid, Start, TracePathFiles, JobID, executed, proc,Trace, nFuelsW, FuelsUsed,AvailTime);
        else
            runSimFarsite(individuos[i], "FARSITE", &err, numgen, atm_file, datos,myid, Start, TracePathFiles, JobID, executed, proc,Trace,nFuelsW,FuelsUsed,AvailTime);
        individuos[i].error = err;
        //printf("errorProcesa :%f\n",individuos[i].error);
        //printf("Worker:%d Error Individuo(%d): %1.4f\n", myid, individuos[i].id, individuos[i].error);

    }
    free(buffer);
}


/******************************************************************************/
// lee el mapa de fuego inicial desde fichero de COORDENADAS
/******************************************************************************/
int getInitFireLine(char * start_line, int Rows, int Cols, double * ignMap, double start_time)
{

    int cell=0, size;
    int x,y;

    FILE * fiche;

    if ((fiche = fopen(start_line, "r")) == NULL)
    {
        printf("getInitFireLine:: error al abrir el fichero %s \n ", start_line);
        return -1;
    }
    // leo la cant de celdas quemadas del fichero
    fscanf(fiche, "%d \n", &size);

    // leo la celda y le pongo el tiempo de inicio de fuego
    for (cell = 0; cell < size; cell ++)
    {
        fscanf(fiche, "%d %d \n", &x, &y);        //.coo = (x,y) por lo tanto, x==columna y==fila
        ignMap[(y-1) * Cols + (x-1)] = start_time;
    }
    fclose(fiche);
}


/******************************************************************************/
// lee el mapa de fuego final para comparar con la simulacion
/******************************************************************************/
int  getFinalFireLine(char * real_line, int Rows, int Cols, double * realMap)
{

    FILE * fil;
    int cell;

    if ((fil = fopen(real_line, "r")) == NULL)
    {
        printf("No se puede abrir el fichero de la linea de fuego real %s \n ", real_line);
        return 0;
    }

    for (cell = 0; cell < Rows*Cols; cell ++)
        fscanf(fil, "%lf", &realMap[cell]);

// PrintMap(realMap, "sale1", Rows, Cols);
    fclose(fil);
    return 1;
}


/******************************************************************************/
// imprime el mapa pasado como parametro map en fileName
/******************************************************************************/
static int PrintMap (double * map, char *fileName, int Rows, int Cols)
{
    FILE *fPtr;
    int cell, col, row;

    if ( (fPtr = fopen(fileName, "w")) == NULL )
    {
        printf("Unable to open output map \"%s\".\n", fileName);
        return (-1);
    }

//   fprintf(fPtr, "north: %1.0f\nsouth: %1.0f\neast: %1.0f\nwest: %1.0f\nrows: %d\ncols: %d\n",
//       (Rows*CellHt), 0., (Cols*CellWd), 0., Rows, Cols);
    for ( row = 0; row < Rows; row++ )
    {
        for ( cell = row*Cols, col=0; col<Cols; col++, cell++ )
        {
            fprintf(fPtr, " %1.2f", (map[cell]==INFINITY) ? 0.0 : map[cell]);
        }
        fprintf(fPtr, "\n");
    }
    fclose(fPtr);
    return (1);
}


/***************************************************************************/
// PROCESAMIENTO PRINCIPAL DEL WORKER
/***************************************************************************/
void old_worker(int taskid, char * datafile)
{

    int dimBloque, nroBloque;
    INDVTYPE *individuos;
    int i, seguir, myid;


    initWorker(datafile);
    getMaps();
    //Carlos B.*****analizarMapaReal();

    unsigned char *msgGrupo;
    INDVTYPE *grupoIndvs;
    MPI_Status status;


// METODO COMPUTACIONAL solo el worker==1 envia al master los datos del mapa real
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    if ((doComputacional) && (myid == 1))
    {
        double vec[3];

        vec[0] = dirPropagacionReal;
        vec[1] = velocidadRealFeets;
        vec[2] = distanciaRealFeets;

        MPI_Send(vec, 3, MPI_DOUBLE, MASTER_ID, WORKER_TO_MASTER_OK_TAG, MPI_COMM_WORLD);
    }


    // seguir flag para seguir procesando o no.... si recibo dimBloque = -1 es que tengo que parar
    seguir = 1;
    while (seguir)
    {
        // recibo la dimension del bloque (si es = -1 es el flag de fin
        MPI_Recv(&dimBloque, 1, MPI_INT, MASTER_ID, MASTER_TO_WORKER_OK_TAG, MPI_COMM_WORLD, &status);
        if (dimBloque < 0)
            seguir = 0;

        if (seguir)
        {
            // recibo el nro de bloque (dentro de la poblacion original)
            MPI_Recv(&nroBloque, 1, MPI_INT, MASTER_ID, MASTER_TO_WORKER_OK_TAG, MPI_COMM_WORLD, &status);
            // aloco memoria para el bloque y la subpoblacion
            msgGrupo = (unsigned char *)malloc(sizeof(INDVTYPE) * dimBloque);
            individuos = (INDVTYPE *)malloc(sizeof(INDVTYPE) * dimBloque);
            // recibo bloque
            MPI_Recv(msgGrupo, sizeof(INDVTYPE)*dimBloque, MPI_UNSIGNED_CHAR, MASTER_ID, MASTER_TO_WORKER_OK_TAG, MPI_COMM_WORLD, &status);

            individuos = (INDVTYPE*) msgGrupo;

            //printf("soy el worker id:%d y recibi dimBloque: %d y nroBloque:%d \n", taskid, dimBloque, nroBloque);

            // PROCESAMIENTO!!!!!!!!!!!!!!
            //   for (i=0; i<dimBloque; i++)
            //     individuos[i].error = 1.0;
            procesarBloque(individuos,dimBloque);


            msgGrupo = (unsigned char *) individuos;
            // envio los datos al MASTER
            MPI_Send(&dimBloque, 1, MPI_INT, MASTER_ID, WORKER_TO_MASTER_OK_TAG, MPI_COMM_WORLD);
            MPI_Send(&nroBloque, 1, MPI_INT, MASTER_ID, WORKER_TO_MASTER_OK_TAG, MPI_COMM_WORLD);
            MPI_Send(msgGrupo, sizeof(INDVTYPE) * dimBloque,MPI_UNSIGNED_CHAR, MASTER_ID, WORKER_TO_MASTER_OK_TAG, MPI_COMM_WORLD);
        } // if seguir
    } // while seguir

    free(real_map_t1);
    free(init_map_t0);
    free(ign_map);

}


void DeletePROC(int * vect,int pid,int size)
{
    int i;
    for (i=0; i<size; i++)
    {
        if (vect[i] == pid)
        {
            vect[i] = -1;
        }
    }
}



int DeletePID(int * vect,int pid,int size)
{
    int i;
    for (i=0; i<size; i++)
    {
        if (vect[i] == pid)
        {
            vect[i] = -1;
            return i;
        }
    }
    return 1;
}

int SearchPID(int * vect,int pid,int size)
{
    int i=-1;
    for (i=0; i<size; i++)
    {
        if (vect[i] == pid)
        {
            return i;
        }
    }
    return -1;
}

int SearchPOS(int * vect,int pid,int size,int threads)
{
    int i;
    int disp = 0;
    int pos = -1;
    int aux = 0;
    int totalfree = 0;
    for (i=0; i<size; i++)
    {
        if (vect[i] == pid)
        {
            if (disp==0)
            {
                pos = i;
            }
            disp++;
            if (disp == threads)
            {
                return pos;
            }
        }
        else
        {
            disp = 0;
        }
    }
    for (i=0; i<size; i++)
    {
        if (vect[i] == pid)
        {
            totalfree++;
        }
    }
    if (totalfree >= threads )
    {
        return size;
    }
    return -1;
}


int SearchUnicPID(int * vect,int size)
{
    int i;
    for (i=0; i<size; i++)
    {
        if (vect[i] != -1)
        {
            return vect[i];
        }
    }
    return -1;
}

int NewWait(int * pIDs,int childs)
{
    int i;
    int auxp = 0;
    int status = -1;
    for (i=0; i<childs; i++)
    {
        //if (pIDs[i] != 1){
        if (pIDs[i] != -1)
        {
            auxp = waitpid((pIDs[i]+1),&status,WNOHANG);
            if (auxp == -1)
            {
                return pIDs[i]+1;
            }//auxp = pIDs[i]+1;}
            //if ((auxp != 0)){
            //  if (auxp == 0){auxp == pIDs[i]+1;}
            //  return auxp;
            //}
            if (auxp > 0)
            {
                return auxp;
            }
        }
    }
    return auxp;
}

int NewWaitB(int * pIDs,int childs)
{
    int i=0;
    int auxp = 0;
    int status = -1;
    while(auxp==0)
    {

        if (pIDs[i] != -1)
        {
            auxp = waitpid((pIDs[i]+1),&status,WNOHANG);
            if (auxp == -1)
            {
                return (pIDs[i]+1);
            }//auxp = pIDs[i]+1;}

            if (auxp > 0)
            {
                return auxp;
            }
        }
        i = (i + 1) % childs;
        //sleep(1);
    }
    //return auxp;
}

void worker(int taskid, char * datosIni, int JobID, double Start)
{

    printf("It is going to launch worker for tarskid %d, datos %s, jobId %d, start %d \n", taskid, datosIni, JobID, Start);
    int stop = 0;
    int nroBloque;
    int num_generation;
    double t1,t2, t3, t4,t5,t6;
    int Childs = 0;
    int i=0;
    int status = -1;
    int avaibleCores = 8;
    int Cores = avaibleCores;
    int CoresIn = avaibleCores + 1;
    int received = 0;
    int * generationsCore = (int*)malloc(sizeof(int)*(avaibleCores));
    int * childs_vect = (int*)malloc(sizeof(int)*(avaibleCores));
    int * proc_vect = (int*)malloc(sizeof(int)*(avaibleCores + 1));
    double * proc_avail_time = (double *)malloc(sizeof(double)*(avaibleCores));
    double * start_time = (double *)malloc(sizeof(double)*(avaibleCores));
    int avaiblePosition = -1;
    int pID = -1;
    int executed = 0;
    int proc = -1;
    int fillin = 0;
    double availTime = 0;
    int numgen_ant=-1;
    double StartingTime=MPI_Wtime();
    int myid;
    int aux2=-9;
    INDVTYPE_FARSITE ** poblacion;
    //char TraceFileName[1000];
    //FILE * TraceFile;

//    printf("MiPID:%d:Rank:%d\n",getpid(),);

    int ** fd = (int **)malloc((Cores)*sizeof(int *));
    for (i=0; i<Cores; i++)
    {
        fd[i] = (int *) malloc((sizeof(int)*2));
    }
    //int fd[8][2];
    int aux=0;

    for (i=0; i<Cores; i++)
    {
        pipe(fd[i]);
        //close(fd[i][0]);close(fd[i][1]);
    }

    poblacion = (INDVTYPE_FARSITE**)malloc(sizeof(INDVTYPE_FARSITE*)*avaibleCores);
    for (i=0; i<Cores; i++)
    {
        poblacion[i] = (INDVTYPE_FARSITE*)malloc(sizeof(INDVTYPE_FARSITE));
    }

    for (i=0; i<Cores; i++)
    {
        childs_vect[i] = -1;
        proc_vect[i] = -1;
    }
    proc_vect[Cores + 1] = -1;

    //INDVTYPE_FARSITE * poblacion = (INDVTYPE_FARSITE*)malloc(chunkSize * sizeof(INDVTYPE_FARSITE)*8);
    printf("It's about to initWorker...\n");
    initWorker(datosIni);
    printf("Worker initiated...\n");
    double MaxAvaibleTime = FireSimLimit;
    double GenTime;
    for (i=0; i<Cores; i++)
    {
        proc_avail_time[i] = 0.0f;
        generationsCore[i] = 0;
    }


    //long long int *addd=0x1900000030;
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    //printf("Debug ->Worker %d &numind:%p %f %d %c %s\n",myid,&numind,(float)*(addd),(int)*(addd),(char)*(addd),(char *)*(addd));
    printf("MiPID:%d:Rank:%d\n",getpid(),myid);
    Childs = 0;
    do
    {
        //comm_time = 0;
        t1 = MPI_Wtime();
        t3 = t1;

        //printf("Worker %d WORKER JUSTO ANTES DE RECEPCIÓN avaibleCores:%d Cores:%d\n",myid,avaibleCores,Cores);
        //poblacion[Childs] = Worker_ReceivedMPI_SetOfIndividualTask(chunkSize, &nroBloque, &num_generation, numind, &stop,&MPIstatus,wait);


        if (avaibleCores == Cores)
        {
            /*
            for (i=0;i<Cores;i++){
              poblacion[i] = NULL;
            }
            */
            for (i=0; i<Cores; i++)
            {
                childs_vect[i] = -1;
            }
            avaiblePosition = SearchPID(childs_vect,-1,Cores);
            poblacion[avaiblePosition] = Worker_ReceivedMPI_SetOfIndividualTask(chunkSize, &nroBloque, &num_generation, numind, &stop,1,&received);

        }
        else
        {
            avaiblePosition = SearchPID(childs_vect,-1,Cores);
            poblacion[avaiblePosition] = Worker_ReceivedMPI_SetOfIndividualTask(chunkSize, &nroBloque, &num_generation, numind, &stop,0,&received);
            //MPI_Test(&MPIstatus, &flag, &Status);



        }
        // printf("Worker %d WORKER JUSTO Despues DE RECEPCIÓN avaibleCores:%d Cores:%d\n",myid,avaibleCores,Cores);

//      printf("Debug Worker %d avaiblePosition:%d\n",myid,avaiblePosition);
        if (received && (stop!=FINISH_SIGNAL))
        {
            printf("Debug Worker %d avaiblePosition:%d\n",myid,avaiblePosition);
            //printf("Worker %d avaiblePosition:%d\n",myid,avaiblePosition);
            //printf("Recibo Worker %d poblacion:%d posicion:%d\n",myid,poblacion[avaiblePosition]->threads,avaiblePosition);
            Childs++;
            received = 0;
            avaibleCores = avaibleCores - poblacion[avaiblePosition]->threads;
            //printf("WORKER(%d) RECEIVED SIGNAL: %d\n", myid, stop);
            //if (stop != FINISH_SIGNAL)
            //{
            executed++;
            proc = SearchPOS(proc_vect,-1,CoresIn,poblacion[avaiblePosition]->threads);
            if (proc != -1)
            {
                printf("Debug Worker %d proc:%d\n",myid,proc);
                if (numgen_ant < poblacion[avaiblePosition]->generation )
                {
                    GenTime=MPI_Wtime();
                    MaxAvaibleTime=((FireSimLimit*numGenerations)-(MPI_Wtime()-StartingTime))/(numGenerations-poblacion[avaiblePosition]->generation);
                    printf("MaxAvailableTime per generation=%f\n",MaxAvaibleTime);
                    numgen_ant = poblacion[avaiblePosition]->generation;

                }
                if (generationsCore[proc] < poblacion[avaiblePosition]->generation)
                {
                    for (i=proc; i<(proc+poblacion[avaiblePosition]->threads); i++)
                    {
                        proc_avail_time[i] = 0.0f + (MPI_Wtime()-GenTime);
                    }
                }

                availTime = MaxAvaibleTime-proc_avail_time[proc];
                if (availTime > 1080*numGenerations)
                {
                    availTime=(1080*numGenerations)-(MPI_Wtime()-StartingTime);
                }
                if (availTime<=1)
                {
                    availTime=1;
                }
                start_time[proc]=MPI_Wtime();
                printf("Worker %d ocupa core %d con tiempo disponible %f a tiempo MPI %f\n",myid,proc,availTime,proc_avail_time[proc]);
                childs_vect[avaiblePosition] = (fork()-1);
                if (childs_vect[avaiblePosition] == -1)
                {
                    procesarBloqueFarsite(poblacion[avaiblePosition],num_generation,datosIni,myid,Start,TracePathFiles,JobID,executed,proc,Trace,availTime);
                    //close(fd[avaiblePosition][0]);
                    open(fd[avaiblePosition][1]);
                    write(fd[avaiblePosition][1],&(poblacion[avaiblePosition]->error),sizeof(float));
                    //open(fd[avaiblePosition][0]);
                    printf("Worker %d acaba ejecucion y enviar error %f de individuo %d \n",myid,poblacion[avaiblePosition]->error,poblacion[avaiblePosition]->oldid);
                    //sleep(2);
                    _exit(2);
                }
                else
                {
                    for (fillin=0; fillin<poblacion[avaiblePosition]->threads && ( fillin < CoresIn  ) ; fillin++)
                    {
                        proc_vect[proc+fillin] = childs_vect[avaiblePosition];
                    }
                }
            }
            else
            {
                printf("Worker:%d Se proporciona posicion: %d!!!!!\n,Cores:%d threadsNeeded:%d\n",myid,proc,Cores,poblacion[avaiblePosition]->threads);
            }
        }

        if (stop != FINISH_SIGNAL)
        {
            //printf("Worker %d avaibleCores%d\n",myid,avaibleCores);
            if (avaibleCores == 0)
            {
                if (Childs > 0)
                {
                    //pID = waitpid(-1,&status,0);
                    printf("Worker %d espera con wait bloqueante\n",myid);
                    //pID = waitpid(0,&status,0);
                    pID = NewWaitB(childs_vect,Cores);
                    //pID = wait(&status);

                    //else{
                    //pID = waitpid((SearchUnicPID(childs_vect,Cores)+1),&status,0);
                    //}
                    if ((pID > 0))
                    {
                        //if (pID == -1){pID=SearchUnicPID(childs_vect,Cores)+1;}
                        printf("Worker  %d Acaba pID %d en posicion %d avaiblecores=0\n",myid,pID,SearchPID(childs_vect,pID-1,Cores));
                        if (SearchPID(childs_vect,pID-1,Cores) != -1  )
                        {
                            //sleep(2);
                            //close(fd[SearchPID(childs_vect,pID-1,Cores)][1]);
                            open(fd[SearchPID(childs_vect,pID-1,Cores)][0]);
                            aux = read(fd[SearchPID(childs_vect,pID-1,Cores)][0], &(poblacion[SearchPID(childs_vect,pID-1,Cores)]->error), sizeof(float));
                            //open(fd[SearchPID(childs_vect,pID-1,Cores)][1]);
                            Worker_SendMPI_IndividualError(poblacion[SearchPID(childs_vect,pID-1,Cores)], chunkSize);
                            avaibleCores = avaibleCores + poblacion[SearchPID(childs_vect,pID-1,Cores)]->threads;
                            proc=SearchPID(proc_vect,pID-1,CoresIn);
                            printf("Debug Worker %d pID%d proc%d\n",myid,pID,proc);
                            if (proc > -1 && (proc < Cores)  )
                            {
                                proc_avail_time[proc]=proc_avail_time[proc]+(MPI_Wtime()-start_time[proc]);
                                generationsCore[proc]=poblacion[SearchPID(childs_vect,pID-1,Cores)]->generation;
                                for (i=proc+1; i<(proc+poblacion[SearchPID(childs_vect,pID-1,Cores)]->threads); i++)
                                {
                                    proc_avail_time[i]=proc_avail_time[proc];
                                    generationsCore[i]=generationsCore[proc];
                                }
                                printf("Worker %d libera core %d con tiempo disponible %lf a tiempo MPI %lf:(%d,%d) error %f ind %d\n",myid,proc,availTime,proc_avail_time[proc],proc,proc+ poblacion[SearchPID(childs_vect,pID-1,Cores)]->threads,poblacion[SearchPID(childs_vect,pID-1,Cores)]->error,poblacion[SearchPID(childs_vect,pID-1,Cores)]->oldid);
                            }
                            if (poblacion[SearchPID(childs_vect,pID-1,Cores)]->generation > numgen_ant)
                            {
                                numgen_ant=poblacion[SearchPID(childs_vect,pID-1,Cores)]->generation;
                            }

                            DeletePID(childs_vect,pID-1,Cores);
                            DeletePROC(proc_vect,pID-1,CoresIn);
                            Childs = Childs - 1;
                        }
                    }
                }
            }
            else
            {
                if (Childs > 0)
                {
                    pID = NewWait(childs_vect,Cores);
                    //pID = waitpid(-1,&status,WNOHANG);
                    //pID = wait(&status);
                    if ((pID > 0))
                    {
                        if (SearchPID(childs_vect,pID-1,Cores) != -1   )
                        {
                            //sleep(2);
                            printf("Worker  %d Acaba pID %d en posicion %d avaiblecores !=0\n",myid,pID,SearchPID(childs_vect,pID-1,Cores));
                            //close(fd[SearchPID(childs_vect,pID-1,Cores)][1]);
                            open(fd[SearchPID(childs_vect,pID-1,Cores)][0]);
                            aux = read(fd[SearchPID(childs_vect,pID-1,Cores)][0], &(poblacion[SearchPID(childs_vect,pID-1,Cores)]->error), sizeof(float));
                            //open(fd[SearchPID(childs_vect,pID-1,Cores)][1]);
                            Worker_SendMPI_IndividualError(poblacion[SearchPID(childs_vect,pID-1,Cores)], chunkSize);
                            avaibleCores = avaibleCores + poblacion[SearchPID(childs_vect,pID-1,Cores)]->threads;
                            proc=SearchPID(proc_vect,pID-1,CoresIn);
                            printf("Debug Worker %d pID%d proc%d\n",myid,pID,proc);
                            if (proc > -1 && (proc < Cores)  )
                            {
                                proc_avail_time[proc]=proc_avail_time[proc]+(MPI_Wtime()-start_time[proc]);
                                generationsCore[proc]=poblacion[SearchPID(childs_vect,pID-1,Cores)]->generation;
                                for (i=proc+1; i<(proc+ poblacion[SearchPID(childs_vect,pID-1,Cores)]->threads); i++)
                                {
                                    proc_avail_time[i]=proc_avail_time[proc];
                                    generationsCore[i]=generationsCore[proc];
                                }
                                printf("Worker %d libera core %d con tiempo disponible %lf a tiempo MPI %lf:(%d,%d) error %f ind %d\n",myid,proc,availTime,proc_avail_time[proc],proc,proc+ poblacion[SearchPID(childs_vect,pID-1,Cores)]->threads,poblacion[SearchPID(childs_vect,pID-1,Cores)]->error,poblacion[SearchPID(childs_vect,pID-1,Cores)]->oldid);
                            }
                            if (poblacion[SearchPID(childs_vect,pID-1,Cores)]->generation > numgen_ant)
                            {
                                numgen_ant=poblacion[SearchPID(childs_vect,pID-1,Cores)]->generation;
                            }
                            DeletePID(childs_vect,pID-1,Cores);
                            DeletePROC(proc_vect,pID-1,CoresIn);
                            Childs = Childs - 1;
                        }
                    }
                }
            }
        }

    }
    while(stop != FINISH_SIGNAL);



    //char huder[2000];
    //sprintf(huder,"gcore %d -o core.%d_\n",childs_vect[avaiblePosition]+1,myid);
    //system(huder);
    /*
        int n=20;
        i=0;
        char* byte_array = 0x1900000030;
        double *cacad = 0x1900000030;
        float *cacaf = 0x1900000030;
        int *cacai = 0x1900000030;

        while (i < n)
         {
         printf("%02X",(int)byte_array[i]);
         i++;
         }
        printf("double:%lf\n",cacad);
         printf("float:%lf\n",cacaf);
         printf("int:%lf\n",cacai);
    */
    /*
    for (i=0;i<Cores;i++){
     close(fd[i][0]);
     close(fd[i][1]);
      free(fd[i]);
    }
    */
    //free(fd);
    printf("Worker %d acaba\n",myid);
    //free(proc_vect);
    //free(childs_vect);
    //printf("LA SIMULACIÓN DE FARSITE %d HA TERMINADO!\n",taskid);
}


