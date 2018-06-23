/*
 * File:   farsite.c
 * Author: Carlos Brun
 *
 * Created on 16 de abril de 2012, 14:09
 */

#include "mpi.h"
//#include "worker.h"
#include <stdio.h>
#include "population.h"
#include <stdlib.h>
#include "iniparser.h"
#include <sys/types.h>
#include "fitness.h"
#include "myutils.h"

int FireSimLimit, numGenerations;
char * farsite_path, *windninja_path, *input_path, *output_path, * real_fire_map_t0, * real_fire_map_t1,*FuelsToCalibrateFileName,*CustomFuelFile,*real_fire_map_tINI;
char * landscapeFile, *ignitionFile, *ignitionFileType, *wndFile, *wtrFile, *adjFile, *fmsFile;
char * baseWndFile, *baseWtrFile, *baseFmsFile, *baseAdjFile;
char * RasterFileName, *shapefile, *VectorFileName, *doRaster, *doShape, *doVector;
char * ConditMonth, *ConditDay, *StartMonth, *StartDay, *StartHour, *StartMin, *EndMonth, *EndDay, *EndHour, *EndMin;
char * timestep, *visibleStep, *secondaryVisibleStep, *perimeterResolution,*distanceResolution;
char * fmsFileNew, * wndFileNew, * wtrFileNew, *adjFileNew, *atmPath;
char * RasterFileNameNew, FuelsUsedFileName;
//float m1, m10, m100, mherb, wnddir, wndvel, temp, hum, error;
int numgen=1, num_threads;
int start_time, end_time;
int     doWindFields;
int     doMeteoSim;
int CalibrateAdjustments;
int seguir = 1;
float TEMP_VARIATION, HUM_VARIATION;
int FuelsUs[256];
int TotalFuels;
//     runSimFarsite(individuos[i],            "FARSITE",     &err,         numgen,      atm_file, d   atos,       myid,       Start,        TracePathFiles,       JobID,  executed,      proc);
int runSimFarsite(INDVTYPE_FARSITE individuo, char * simID, double * err, int numgen, char * atm, char * datos, int myid,double Start,char * TracePathFiles, int JobID,int executed,int proc, int Trace, int Fuels, int * FuelsU,double AvailTime);
void initFarsiteVariables(char * filename, int numgen);
void createInputFiles(INDVTYPE_FARSITE individuo, char * dataFile);
void createSettingsFile(char * filename, int idInd, int gen,int res);
double getSimulationError(char * simulatedMap);
double getSimulationErrorOLD(char * simulatedMap);

int runSimFarsite(INDVTYPE_FARSITE individuo, char * simID, double * err, int numgene, char * atm, char * datos, int myid,double Start,char * TracePathFiles, int JobID,int executed,int proc,int Trace, int FuelsN, int * FuelsLoaded, double AvailTime)
{
    // printf("\n\nIndividuo que llega a FARSITE:\n");
    //printf("Ind.%d m1:%1.3f m10:%1.3f m100:%1.3f mherb:%1.3f ws:%1.0f wd:%1.0f temp:%1.0f hum:%1.0f error:%1.1f\n", individuo.id, individuo.m1,individuo.m10,individuo.m100,individuo.mherb,individuo.wndvel,individuo.wnddir, individuo.temp,individuo.hum,individuo.error);
    double error = 9999.99f;
    numgen = numgene;
    //TotalFuels = FuelsN;


//    printf("AvailTimeB:%f\n",AvailTime);
    //printf("---Inicio función: farsite.c::runSimFarsite\n");
    //Init variables
//    FuelsUsed = FuelsLoaded;
    if(doWindFields == 1)
    {
        atmPath = atm;
        //printf("ATM PATH: %s\n",atmPath);
    }
    /*
     m1      = individuo.m1;
     m10     = individuo.m10;
     m100    = individuo.m100;
     mherb   = individuo.mherb;
     wnddir  = individuo.wnddir;
     wndvel  = individuo.wndvel;
     temp    = individuo.temp;
     hum     = individuo.hum;
    */
    char settings_filename[2000];
    //char nt[100];
    char syscall[5000];
    double ti,te;
    double execuTime = 0.0f;
    //char *TraceBuffer = (char*)malloc(sizeof(char)*5000);
    char TraceBuffer[1000];
    sprintf(TraceBuffer,"");
    char TraceFileName[3000];
    FILE * TraceFile;
    sprintf(TraceFileName,"%sWorkerTrace_%d_%d_%d.dat",TracePathFiles,myid,JobID,executed);
    initFarsiteVariables(datos, numgen);
    //if (CalibrateAdjustments){FuelsUs = FuelsLoaded;}
    sprintf(settings_filename,"%ssettings_%d_%d.txt",output_path,numgen,individuo.id);

    createInputFiles(individuo, datos);
    int res=100;
    switch(individuo.class_ind)
    {
    case 'E':
        res=100;
        break;
    case 'D':
        res=100;
        break;
    default:
        res=100;
        break;
    }
    if (numgen==10)
    {
        AvailTime=3600.0;
        res=300;
        individuo.threads=8;
    };
    createSettingsFile(settings_filename, individuo.id,numgen,res);

    //./farsite4 settings_filename
    //sprintf(syscall, "%d", num_threads);
    //setEnvironmentVariable("OMP_NUM_THREADS", syscall);

//    sprintf(nt,"OMP_NUM_THREADS=%d",individuo.threads);
//    putenv(nt);
//    printf("Worker:%d:",myid);
    /*********** LLAMADA FARSITE PARALELO TOMAS ***********/
    //printf("%sfarsite4P -i %s -l %d -f %d -t 1 -g %d -n %d -w %d\n",farsite_path,settings_filename,FireSimLimit,individuo.threads,numgen,individuo.id,myid);
//    printf("timeout --signal=SIGXCPU %.0fs %sfarsite4P -i %s -f %d -t 1 -g %d -n %d -w %d -p %s\n",AvailTime,farsite_path,settings_filename,individuo.threads,numgen,individuo.id,myid,perimeterResolution);

//printf("AvailTime:%f\n",AvailTime);
    if (AvailTime<=1.0f)
    {
        AvailTime=1.0;
    };
    if (numgen==10)
    {
        AvailTime=3600.0;
        res=300;
        individuo.threads=8;
    };
    printf("timeout --signal=SIGXCPU %.0fs %sfarsite4P -i %s -f %d -t 1 -g %d -n %d -w %d -p %dm\n",AvailTime,farsite_path,settings_filename,individuo.threads,numgen,individuo.id,myid,res);

    //printf("Worker:%d:",myid);
    //sprintf(syscall,"%sfarsite4P -i %s -l %d -f %d -t 1 -g %d -n %d -w %d",farsite_path,settings_filename,FireSimLimit,individuo.threads,numgen,individuo.id,myid);
//    sprintf(syscall,"timeout --signal=SIGXCPU %.0f %sfarsite4P -i %s -f %d -t 1 -g %d -n %d -w %d -p %s",AvailTime,farsite_path,settings_filename,individuo.threads,numgen,individuo.id,myid,perimeterResolution);
    print_indv_farsite(individuo);
    sprintf(syscall,"/usr/bin/time --format \"%d %d %%e %%M %%O %%P %%c %%x\" -a --output=time_output.txt timeout --signal=SIGXCPU %.0f %sfarsite4P -i %s -f %d -t 1 -g %d -n %d -w %d -p %dm",numgen,individuo.id,AvailTime,farsite_path,settings_filename,individuo.threads,numgen,individuo.id,myid,res);

    /*********** LLAMADA FARSITE ORIGINAL ***********/
    //sprintf(syscall,"%sfarsite4 %s",farsite_path,settings_filename);
    //printf("farsite:Worker %d en procesa %d\n",myid,proc);
    //printf("%s\n", syscall);
    ti = MPI_Wtime();
    int err_syscall = system(syscall);
    te = MPI_Wtime();
    execuTime = te-ti;

    if (err_syscall == 0)
    {
        //printf("ACABA:%sfarsite4P -i %s -l %d -f %d -t 1 -g %d -n %d -w %d\n",farsite_path,settings_filename,FireSimLimit,individuo.threads,numgen,individuo.id,myid);
        char sim_fire_line[5000];
        sprintf(sim_fire_line,"%s%s.toa", output_path, RasterFileNameNew);
        error = getSimulationError(sim_fire_line);
        *err = error;
        //printf("Error:%f\n",error);
        sprintf(TraceBuffer,"Worker%d %1.2f %1.2f %d %d %d %d\n",myid,ti-Start,te-Start,myid,individuo.threads,proc,1);
    }
    else
    {
        //printf("NOACABA:%sfarsite4P -i %s -l %d -f %d -t 1 -g %d -n %d -w %d\n",farsite_path,settings_filename,FireSimLimit,individuo.threads,numgen,individuo.id,myid);
        //    printf("FARSITE:%d_%d_%d_%f_%f\n",generation,n,cores,TimeCounter::Instance()->getTimeS(),TimeCounter::Instance()->getTime());
        printf( "FARSITE:%d:%d_%d_%d_%f_%f_%d_%s\n",myid,numgen,individuo.id,individuo.threads,(float)AvailTime,(float)AvailTime,myid,perimeterResolution);
        *err = 9999.99f;
        sprintf(TraceBuffer,"Worker%d %1.2f %1.2f %d %d %d %d\n",myid,ti-Start,te-Start,myid,individuo.threads,proc,0);
    }


    if (Trace)
    {
        if (( TraceFile = fopen(TraceFileName, "w")) == NULL)
        {
            printf("Error opening file 'w' %s\n",TraceFileName);
        }
        else
        {
            fprintf(TraceFile,"%s",TraceBuffer);
            fclose(TraceFile);
        }
    }
    //free(TraceBuffer);
    return (EXIT_SUCCESS);

}

void initFarsiteVariables(char * filename, int numgen)
{
    //printf("---Inicio función: farsite.c::initFarsiteVariables\n");
    dictionary * datos;
    datos 	= iniparser_load(filename);

    numGenerations   = iniparser_getint(datos, "genetic:numGenerations",1);
    num_threads      = iniparser_getint(datos, "main:num_threads",1);
    doWindFields    	= iniparser_getint(datos, "main:doWindFields", 0);
    doMeteoSim    	= iniparser_getint(datos, "main:doMeteoSim", 0);
    CalibrateAdjustments = iniparser_getint(datos,"main:CalibrateAdjustments",0);
    FuelsToCalibrateFileName	= iniparser_getstr(datos,"main:FuelsToCalibrate");

    CustomFuelFile	= iniparser_getstr(datos,"farsite:CustomFuelFile");
    farsite_path        = iniparser_getstr(datos, "farsite:farsite_path");
    input_path          = iniparser_getstr(datos, "farsite:input_path");
    output_path         = iniparser_getstr(datos, "farsite:output_path");
    landscapeFile       = iniparser_getstr(datos, "farsite:landscapeFile");
    adjFile      = iniparser_getstr(datos, "farsite:adjFile");
    ignitionFile        = iniparser_getstr(datos, "farsite:ignitionFile");
    ignitionFileType    = iniparser_getstr(datos, "farsite:ignitionFileType");
    wndFile             = iniparser_getstr(datos, "farsite:wndFile");
    wtrFile             = iniparser_getstr(datos, "farsite:wtrFile");
    fmsFile             = iniparser_getstr(datos, "farsite:fmsFile");
    baseWndFile         = iniparser_getstr(datos, "farsite:baseWndFile");
    baseWtrFile         = iniparser_getstr(datos, "farsite:baseWtrFile");
    baseFmsFile         = iniparser_getstr(datos, "farsite:baseFmsFile");
    baseAdjFile		= iniparser_getstr(datos,"farsite:baseAdjFile");
    RasterFileName      = iniparser_getstr(datos, "farsite:RasterFileName");
    shapefile           = iniparser_getstr(datos, "farsite:shapefile");
    VectorFileName      = iniparser_getstr(datos, "farsite:VectorFileName");
    doRaster            = iniparser_getstr(datos, "farsite:doRaster");
    doShape             = iniparser_getstr(datos, "farsite:doShape");
    doVector            = iniparser_getstr(datos, "farsite:doVector");
    if(numgen == numGenerations)
    {
        ignitionFile        = iniparser_getstr(datos, "prediction:PignitionFile");
        ignitionFileType    = iniparser_getstr(datos, "prediction:PignitionFileType");
        ConditMonth         = iniparser_getstr(datos, "prediction:PConditMonth");
        ConditDay           = iniparser_getstr(datos, "prediction:PConditDay");
        StartMonth          = iniparser_getstr(datos, "prediction:PStartMonth");
        StartDay            = iniparser_getstr(datos, "prediction:PStartDay");
        StartHour           = iniparser_getstr(datos, "prediction:PStartHour");
        StartMin            = iniparser_getstr(datos, "prediction:PStartMin");
        EndMonth            = iniparser_getstr(datos, "prediction:PEndMonth");
        EndDay              = iniparser_getstr(datos, "prediction:PEndDay");
        EndHour             = iniparser_getstr(datos, "prediction:PEndHour");
        EndMin              = iniparser_getstr(datos, "prediction:PEndMin");
        start_time          = iniparser_getint(datos, "prediction:Pstart_time",1);
        end_time           = iniparser_getint(datos, "prediction:Pend_time",1);
        real_fire_map_t0    = iniparser_getstr(datos, "prediction:Preal_fire_map_t0");
        real_fire_map_t1    = iniparser_getstr(datos, "prediction:Preal_fire_map_t1");
        real_fire_map_tINI  = iniparser_getstr(datos, "prediction:Preal_fire_map_tINI");
    }
    else
    {
        ignitionFile        = iniparser_getstr(datos, "farsite:ignitionFile");
        ignitionFileType    = iniparser_getstr(datos, "farsite:ignitionFileType");
        ConditMonth         = iniparser_getstr(datos, "farsite:ConditMonth");
        ConditDay           = iniparser_getstr(datos, "farsite:ConditDay");
        StartMonth          = iniparser_getstr(datos, "farsite:StartMonth");
        StartDay            = iniparser_getstr(datos, "farsite:StartDay");
        StartHour           = iniparser_getstr(datos, "farsite:StartHour");
        StartMin            = iniparser_getstr(datos, "farsite:StartMin");
        EndMonth            = iniparser_getstr(datos, "farsite:EndMonth");
        EndDay              = iniparser_getstr(datos, "farsite:EndDay");
        EndHour             = iniparser_getstr(datos, "farsite:EndHour");
        EndMin              = iniparser_getstr(datos, "farsite:EndMin");
        start_time          = iniparser_getint(datos, "farsite:start_time",1);
        end_time           = iniparser_getint(datos, "farsite:end_time",1);
        real_fire_map_t0    = iniparser_getstr(datos, "farsite:real_fire_map_t0");

        real_fire_map_t1    = iniparser_getstr(datos, "farsite:real_fire_map_t1");
        real_fire_map_tINI  = iniparser_getstr(datos, "farsite:real_fire_map_tINI");
    }

    timestep            = iniparser_getstr(datos, "farsite:timestep");
    visibleStep         = iniparser_getstr(datos, "farsite:visibleStep");
    secondaryVisibleStep= iniparser_getstr(datos, "farsite:secondaryVisibleStep");
    perimeterResolution = iniparser_getstr(datos, "farsite:perimeterResolution");
    distanceResolution  = iniparser_getstr(datos, "farsite:distanceResolution");
    TEMP_VARIATION      = iniparser_getdouble(datos, "farsite:TEMP_VARIATION",1.0);
    HUM_VARIATION       = iniparser_getdouble(datos, "farsite:HUM_VARIATION",1.0);
    FireSimLimit  = iniparser_getint(datos, "farsite:ExecutionLimit", 1);

    if (CalibrateAdjustments)
    {
        FILE *FuelsToCalibrateFILE;
        int i,nFuel;

        for (i=0; i<256; i++)
        {
            FuelsUs[i]=0;
        }
        if ((FuelsToCalibrateFILE = fopen(FuelsToCalibrateFileName,"r"))==NULL)
        {
            printf("ERROR:Opening fuels used file.\n");
        }
        else
        {
            //printf("INFO:Used fuels--> ");
            while(fscanf(FuelsToCalibrateFILE,"%d",&nFuel)!=EOF)
            {
                FuelsUs[nFuel-1]=1;
                //	printf("%d ",nFuel);
            }
            //printf("\n");
        }
    }

}

void createInputFiles(INDVTYPE_FARSITE individuo, char * dataFile)
{
    //printf("---Inicio función: farsite.c::createInputFiles\n");
    ///initFarsiteVariables(dataFile);
    //printf("CreatingFilesFor:");
    //print_indv_farsite(individuo);
    char * line = (char*)malloc(sizeof(char) * 200);
    char * newline= (char*)malloc(sizeof(char) * 200);
    char * buffer= (char*)malloc(sizeof(char) * 200);
    //char buffer[100];

    fmsFileNew = (char*)malloc(sizeof(char) * 200);
    adjFileNew = (char*)malloc(sizeof(char) * 200);

    if(doMeteoSim == 0)
    {
        wndFileNew = (char*)malloc(sizeof(char) * 200);
        wtrFileNew = (char*)malloc(sizeof(char) * 200);
    }
    FILE * fFMS, *fWND, *fWTR, *fADJ, *fFMSnew, *fWNDnew, *fWTRnew,*fADJnew;
    char * tmp = (char*)malloc(sizeof(char) * 400);
    // Create corresponding fms,wnd & wtr filename for each individual
    sprintf(tmp,"%d",numgen);

    //printf("Número de generaciones:%d\n", numgen);

    fmsFileNew = str_replace(fmsFile, "$1", tmp);

    if (CalibrateAdjustments)
    {
        //printf("Calibrate adjustments!\n");
        adjFileNew = str_replace(adjFile, "$1", tmp);
    }

    if(doMeteoSim == 0)
    {
        wndFileNew = str_replace(wndFile,"$1", tmp);
        wtrFileNew = str_replace(wtrFile,"$1", tmp);
    }
    sprintf(tmp,"%d",individuo.id);
    fmsFileNew = str_replace(fmsFileNew, "$2", tmp);
    if (CalibrateAdjustments)
    {
        adjFileNew = str_replace(adjFileNew, "$2", tmp);
    }


    if(doMeteoSim == 0)
    {
        wndFileNew = str_replace(wndFileNew,"$2", tmp);
        wtrFileNew = str_replace(wtrFileNew,"$2", tmp);
    }
    //printf("FMS: %s-\n", fmsFileNew);
    //printf("WND: %s-\n", wndFileNew);
    //printf("WTR: %s-\n", wtrFileNew);

    if ((fFMS = fopen(baseFmsFile, "r")) == NULL)
    {
        printf("Unable to open FMS file");
        seguir = 0;

    }
    if(doMeteoSim == 0)
    {
        if(((fWND = fopen(baseWndFile, "r")) == NULL) || ((fWTR = fopen(baseWtrFile, "r")) == NULL) )
        {
            printf("Unable to open WND or WTR files\n");
            seguir = 0;
        }
    }
    if(seguir == 1)
    {
        if((fFMSnew = fopen(fmsFileNew, "w")) == NULL)
        {
            printf("Unable to create FMS temp file\n");
            seguir = 0;
        }
        if(doMeteoSim == 0)
        {
            if(((fWNDnew = fopen(wndFileNew, "w")) == NULL) || ((fWTRnew = fopen(wtrFileNew, "w")) == NULL) )
            {
                printf("Unable to create WND or WTR temp files\n");
                seguir = 0;
            }
        }
        if(seguir == 1)
        {
            //printf("VALORES DE LAS HUMEDADES: m1:%1.3f m10:%1.3f m100:%1.3f mherb:%1.3f\n", individuo.m1, individuo.m10, individuo.m100, individuo.mherb);
            while(fgets( line, 100, fFMS ) != NULL)
            {
                sprintf(buffer,"%1.0f",individuo.parameters[0]);
                newline = str_replace(line, "1h", buffer);
                sprintf(buffer,"%1.0f",individuo.parameters[1]);
                newline = str_replace(newline, "10h", buffer);
                sprintf(buffer,"%1.0f",individuo.parameters[2]);
                newline = str_replace(newline, "100h", buffer);
                sprintf(buffer,"%1.0f",individuo.parameters[3]);
                newline = str_replace(newline, "herb", buffer);
                fprintf(fFMSnew,"%s", newline);
            }
            fclose(fFMSnew);

            if(CalibrateAdjustments)
            {
                //printf("adjFileNew:%s\nbaseAdjFile%s\n",adjFileNew,baseAdjFile);
                if(((fADJnew = fopen(adjFileNew, "w")) == NULL) || (fADJ = fopen(baseAdjFile, "r")) == NULL)
                {
                    printf("Unable to create ADJ temp file\n");
                    seguir = 0;
                }
                int nfuel,param=9;
                float adjust=0.0f;
//	while(fgets( line, 100, fADJ ) != NULL)
                while(fscanf(fADJ,"%d %f",&nfuel,&adjust)!= EOF )
                {
                    if (FuelsUs[nfuel-1])
                    {
//                      	sprintf(buffer,"%1.6f",individuo.parameters[param]);
//                	        newline = str_replace(line, "-9999.000000", buffer);
                        fprintf(fADJnew,"%d %1.6f\n",nfuel,individuo.parameters[param]);
                        param++;
                    }
                    else
                    {
                        fprintf(fADJnew,"%d 1.000000\n",nfuel);
                    }

                }
                fclose(fADJnew);
                fclose(fADJ);
            }

            if(doMeteoSim == 0)
            {
                fgets( line, 100, fWND );
                fprintf(fWNDnew,"%s", line);
                while(fgets( line, 100, fWND ) != NULL)
                {
                    sprintf(buffer,"%1.0f",individuo.parameters[5]);
                    newline = str_replace(line, "ws", buffer);
                    sprintf(buffer,"%1.0f",individuo.parameters[6]);
                    newline = str_replace(newline, "wd", buffer);
                    sprintf(buffer,"%d",0);
                    newline = str_replace(newline, "wc", buffer);
                    fprintf(fWNDnew,"%s", newline);
                }
                if(doMeteoSim == 0) fclose(fWNDnew);

                fgets( line, 100, fWTR );
                fprintf(fWTRnew,"%s", line);
                float tl = individuo.parameters[7] - TEMP_VARIATION;
                float hl = individuo.parameters[8] - HUM_VARIATION;

                while(fgets( line, 100, fWTR ) != NULL)
                {
                    sprintf(buffer,"%1.0f",tl);
                    newline = str_replace(line, "tl", buffer);

                    sprintf(buffer,"%1.0f",individuo.parameters[6]);
                    newline = str_replace(newline, "th", buffer);

                    sprintf(buffer,"%1.0f", individuo.parameters[7]);
                    newline = str_replace(newline, "hh", buffer);

                    sprintf(buffer,"%1.0f",hl);
                    newline = str_replace(newline, "hl", buffer);

                    fprintf(fWTRnew,"%s", newline);
                }
                if(doMeteoSim == 0) fclose(fWTRnew);
            }
        }
        fclose(fFMS);
        if(doMeteoSim == 0)
        {
            fclose(fWND);
            fclose(fWTR);
        }
    }
    //free(line);
    //free(newline);
    //free(buffer);
}

void createSettingsFile(char * filename, int idInd,int generation,int res)
{
    char * shapefileNew = (char*)malloc(sizeof(char) * 400);
    RasterFileNameNew = (char*)malloc(sizeof(char) * 400);
    char * VectorFileNameNew = (char*)malloc(sizeof(char) * 400);
    char * tmp = (char*)malloc(sizeof(char) * 400);

    FILE * file;
    //int res=10+numgen*30;


    if ( (file = fopen(filename, "w")) == NULL )
    {
        printf("Unable to open settings file");
    }
    else
    {
        sprintf(tmp,"%d",numgen);
        shapefileNew = str_replace(shapefile, "$1", tmp);
        RasterFileNameNew = str_replace(RasterFileName,"$1", tmp);
        VectorFileNameNew = str_replace(VectorFileName,"$1", tmp);
        sprintf(tmp,"%d",idInd);
        shapefileNew = str_replace(shapefileNew, "$2", tmp);
        RasterFileNameNew = str_replace(RasterFileNameNew,"$2", tmp);
        VectorFileNameNew = str_replace(VectorFileNameNew,"$2", tmp);

        fprintf(file,"version = 43\n");
        // FILES
//        fprintf(file,"fuelmodelfile = /scratch/078-hpca4se-comp/tomas/TOPTEN/Data/AuxFiles/CustomFuel.fmd\n");
        fprintf(file,"landscapeFile = %s\n", landscapeFile);
        fprintf(file,"FUELMOISTUREFILE =  %s\n", fmsFileNew);
        if (CustomFuelFile != NULL)
        {
            fprintf(file,"fuelmodelfile = %s\n",CustomFuelFile);
        }
        if(doWindFields == 0)
        {
            if(doMeteoSim == 0)
            {
                //printf("windFile0 =  %s\n", wndFileNew);
                fprintf(file,"windFile0 =  %s\n", wndFileNew);
            }
            else
                fprintf(file,"windFile0 =  %s\n", wndFile);
        }
        else
        {
            if(doMeteoSim == 0)
                fprintf(file,"windFile0 =  %s\n", atmPath);
            if(doMeteoSim == 1)
                fprintf(file,"windFile0 =  %s\n", wndFile);
        }
        //printf("CalibrateAdjustments:%d\n",CalibrateAdjustments);
        if(CalibrateAdjustments)
        {
            fprintf(file,"adjustmentFile = %s\n", adjFileNew);
        }
        else
        {
            fprintf(file,"adjustmentFile = %s\n", baseAdjFile);
        }
        if(doMeteoSim == 0)
        {
            fprintf(file,"weatherFile0 = %s\n", wtrFileNew);
        }
        else
        {
            fprintf(file,"weatherFile0 = %s\n", wtrFile);
        }
        // OUTPUTS CREATION
        fprintf(file,"vectMake = %s\n", doVector);
        fprintf(file,"rastMake = %s\n", doRaster);
        fprintf(file,"shapeMake = %s\n", doShape);
        // RESOLUTION
        fprintf(file,"timestep = %s\n", timestep);
        fprintf(file,"visibleStep = %s\n", visibleStep);
        fprintf(file,"secondaryVisibleStep = %s\n", secondaryVisibleStep);
        //fprintf(file,"perimeterResolution = %s\n", perimeterResolution);
        fprintf(file,"perimeterResolution = %dm\n", res);
        fprintf(file,"distanceResolution = %s\n", distanceResolution);
        // IGNITION DATA & TYPE
        fprintf(file,"ignitionFile = %s\n", ignitionFile);
        fprintf(file,"ignitionFileType = %s\n", ignitionFileType);
        if(strcmp(doShape,"true") == 0)
            fprintf(file,"shapefile=%s%s.shp\n", output_path, shapefileNew);
        if(strcmp(doRaster,"true") == 0)
            fprintf(file,"RasterFileName=%s%s\n", output_path, RasterFileNameNew);
        if(strcmp(doVector,"true") == 0)
            fprintf(file,"VectorFileName=%s%s\n", output_path, VectorFileNameNew);

        fprintf(file,"enableCrownfire = false\n");
        fprintf(file,"linkCrownDensityAndCover = false\n");
        fprintf(file,"embersFromTorchingTrees = false\n");
        fprintf(file,"enableSpotFireGrowth = false\n");
        fprintf(file,"nwnsBackingROS = false\n");
        fprintf(file,"fireacceleration=false\n");
        fprintf(file,"accelerationtranstion=1m\n");
        fprintf(file,"distanceChecking = fireLevel\n");
        fprintf(file,"simulatePostFrontalCombustion = false\n");
        fprintf(file,"fuelInputOption = absent\n");
        fprintf(file,"calculationPrecision = normal\n");

        fprintf(file,"useConditioningPeriod = false\n");
        fprintf(file,"ConditMonth = %s\n",ConditMonth);
        fprintf(file,"ConditDay = %s\n", ConditDay);
        fprintf(file,"StartMonth = %s\n", StartMonth);
        fprintf(file,"StartDay = %s\n", StartDay);
        fprintf(file,"StartHour = %s\n", StartHour);
        fprintf(file,"StartMin = %s\n", StartMin);
        fprintf(file,"EndMonth = %s\n", EndMonth);
        if (generation==10)
        {
            fprintf(file,"EndDay = %d\n", atoi(StartDay)+1);
        }
        else
        {
            fprintf(file,"EndDay = %s\n", EndDay);
        }
        fprintf(file,"EndHour = %s\n", EndHour);
        fprintf(file,"EndMin = %s\n", EndMin);

        fprintf(file,"rast_arrivaltime = true\n");
        fprintf(file,"rast_fireIntensity = false\n");
        fprintf(file,"rast_spreadRate = false\n");
        fprintf(file,"rast_flameLength = false\n");
        fprintf(file,"rast_heatPerArea = false\n");
        fprintf(file,"rast_crownFire = false\n");
        fprintf(file,"rast_fireDirection = false\n");
        fprintf(file,"rast_reactionIntensity = false\n");

        fclose(file);
    }
}

double getSimulationError(char * simulatedMap)
{
    //Args: mode mapaRealFileName mapaSimFileName t1 t2
    //ambos mapas -> rutas absolutas al fichero .toa tal cual sale del farsite
    FILE *fd,*fd2;
    char tmp;
    char name[20];
    int i,n,j,srows,scols,rrows=0,rcols=0,aux;
    double *mapaReal, *mapaSim,fitness, error = 9999.0, doubleValue,val;

    if(((fd = fopen(real_fire_map_t1, "r")) == NULL) || ((fd2 = fopen(real_fire_map_tINI, "r")) == NULL) )
    {
        printf("Unable to open real map file");
    }
    else
    {
        fscanf(fd,"%7s%f\n%7s%f\n%7s%f\n%7s%f\n%7s%d\n%7s%d\n",name,&val,name,&val,name,&val,name,&val,name,&rrows,name,&rcols);
        fscanf(fd2,"%7s%f\n%7s%f\n%7s%f\n%7s%f\n%7s%d\n%7s%d\n",name,&val,name,&val,name,&val,name,&val,name,&aux,name,&aux);
        //printf("\n>>>>>>>> %s %d <<<<<<<<<<\n",tmp,rrows);
        //fscanf(fd,"%s\n%d\n",&tmp,&rrows);
        //fscanf(fd,"%s\n%d\n",&tmp,&rcols);
        //fscanf(fd2,"%s\n%d\n",&tmp,&aux);
        //printf("\n>>>>>>>> %d %d <<<<<<<<<<\n",rrows,rcols);
        //printf("rrows: %d rcols: %d\n",rrows,rcols);
        mapaReal=(double *)calloc(rrows*rcols,sizeof(double));

        for(i=0; i<rrows*rcols; i++)
        {
            fscanf(fd2,"%lf\n",&doubleValue);
            if(doubleValue==1.0f)
            {
                fscanf(fd,"%lf\n",&mapaReal[i]);
                mapaReal[i]=-1.0f;
            }
            else
            {
                fscanf(fd,"%lf\n",&mapaReal[i]);
            }
        }
        fclose(fd);
        fclose(fd2);

        if((fd=fopen(simulatedMap,"r")) == NULL)
        {
            printf("Unable to open simulated map file");
        }
        else
        {
            fscanf(fd,"%7s%f\n%7s%f\n%7s%f\n%7s%f\n%7s%d\n%7s%d\n",name,&val,name,&val,name,&val,name,&val,name,&srows,name,&scols);
            //fscanf(fd,"%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",&tmp,&tmp,&tmp,&tmp,&tmp,&tmp,&tmp,&tmp,&tmp);
            //fscanf(fd,"%s\n%d\n",&tmp,&srows);
            //fscanf(fd,"%s\n%d\n",&tmp,&scols);
            //printf("srows: %d scols: %d\n",srows,scols);
            mapaSim=(double *)calloc(srows*scols,sizeof(double));

            if( (srows!=rrows) || (scols!=rcols) )
            {
                printf("ERROR: Different map dimensions!! Real: %dx%d Simulated: %dx%d\n",rrows,rcols,srows,scols);
            }
            else
            {
                for(i=0; i<srows*scols; i++)
                {
                    //for (j=0;j<scols;j++){
                    fscanf(fd,"%lf\n",&mapaSim[i]);
                    //if (mapaSim[i]!=(1.0) || mapaSim[i]!=(-1.0) ){printf("Error reading simulated map\n");}
                    //}
                    //fscanf("\n")
                }

                fclose(fd);

                //printf("Cálculo del error: mapa real -> %s mapa sim-> %s rows->%d cols->%d t1->%d t2->%d error->%1.2f\n",real_fire_map_t0,simulatedMap, rrows, rcols, start_time, end_time, error);

//		#ifdef OPENMP
//			threads=omp_get_max_threads();
//			printf("num_threads:%d\n",num_threads);
//			omp_set_num_threads(num_threads);
//		#endif


                fitness=fitnessYError(mapaReal,mapaSim,rrows,rcols,start_time,end_time,&error);

                free(mapaReal);
                free(mapaSim);
                //printf("ERROR ENTRE MAPAS: %f\n",error);
            }
        }
    }
    //fclose(fd);
    return error;
}

double getSimulationErrorOLD(char * simulatedMap)
{
    //Args: mode mapaRealFileName mapaSimFileName t1 t2
    //ambos mapas -> rutas absolutas al fichero .toa tal cual sale del farsite
    FILE *fd;
    char tmp[32];
    int i,n,j,srows,scols,rrows=0,rcols=0;
    double *mapaReal, *mapaSim, fitness, error = 9999.0;

    if((fd = fopen(real_fire_map_t0, "r")) == NULL)
    {
        printf("Unable to open real map file");
    }
    else
    {
        fscanf(fd,"%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s%d\n",&tmp,&tmp,&tmp,&tmp,&tmp,&tmp,&tmp,&tmp,&tmp,&rrows);
        //printf("\n>>>>>>>> %s %d <<<<<<<<<<\n",tmp,rrows);
        fscanf(fd,"%s\n%d\n",&tmp,&rcols);
        //printf("\n>>>>>>>> %s %d <<<<<<<<<<\n",tmp,rcols);
        mapaReal=(double *)calloc(rrows*rcols,sizeof(double));

        for(i=0; i<rrows*rcols; i++)
            fscanf(fd,"%lf\n",&mapaReal[i]);

        fclose(fd);
        if((fd=fopen(simulatedMap,"r")) == NULL)
        {
            printf("Unable to open simulated map file");
        }
        else
        {
            fscanf(fd,"%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s%d\n",&tmp,&tmp,&tmp,&tmp,&tmp,&tmp,&tmp,&tmp,&tmp,&srows);
            fscanf(fd,"%s\n%d\n",&tmp,&scols);
            mapaSim=(double *)calloc(srows*scols,sizeof(double));

            if( (srows!=rrows) || (scols!=rcols) )
            {
                printf("\nERROR: Different map dimensions!! Real: %dx%d Simulated: %dx%d\n",rrows,rcols,srows,scols);
            }
            else
            {
                for(i=0; i<srows*scols; i++)
                {
                    //for (j=0;j<scols;j++){
                    fscanf(fd,"%lf\n",&mapaSim[i]);
                    //if (mapaSim[i]!=(1.0) || mapaSim[i]!=(-1.0) ){printf("Error reading simulated map\n");}
                    //}
                    //fscanf("\n")
                }

                fclose(fd);

                //printf("Cálculo del error: mapa real -> %s mapa sim-> %s rows->%d cols->%d t1->%d t2->%d error->%1.2f\n",real_fire_map_t0,simulatedMap, rrows, rcols, start_time, end_time, error);

//		#ifdef OPENMP
//			threads=omp_get_max_threads();
//			printf("num_threads:%d\n",num_threads);
//			omp_set_num_threads(num_threads);
//		#endif


                fitness=fitnessYError(mapaReal,mapaSim,rrows,rcols,start_time,end_time,&error);

                //free(mapaReal);
                //free(mapaSim);
                //printf("ERROR ENTRE MAPAS: %f\n",error);
            }
        }
    }
    return error;
}
