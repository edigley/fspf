/*
 * File:   farsite.c
 * Author: Carlos Brun
 *
 * Created on 16 de abril de 2012, 14:09
 */

#include "mpi.h"
#include <stdio.h>
#include "population.h"
#include <stdlib.h>
#include "iniparser.h"
#include <sys/types.h>
#include "fitness.h"
#include "myutils.h"

#define ERROR_INFINITY 9999.99f

int FireSimLimit, numGenerations;
char * farsite_path, * windninja_path, * input_path, * output_path, * real_fire_map_t0, * real_fire_map_t1, * FuelsToCalibrateFileName, * CustomFuelFile, * real_fire_map_tINI;
char * landscapeFile, * ignitionFile, * ignitionFileType, * wndFile, * wtrFile, * adjFile, * fmsFile;
char * baseWndFile, * baseWtrFile, * baseFmsFile, * baseAdjFile;
char * RasterFileName, * shapefile, * VectorFileName, * doRaster, * doShape, * doVector;
char * ConditMonth, * ConditDay, * StartMonth, * StartDay, * StartHour, * StartMin, * EndMonth, * EndDay, * EndHour, * EndMin;
char * timestep, * visibleStep, * secondaryVisibleStep, * perimeterResolution, * distanceResolution;
char * fmsFileNew, * wndFileNew, * wtrFileNew, * adjFileNew, * atmPath;
char * RasterFileNameNew, FuelsUsedFileName;

int generation=1, num_threads;
int start_time, end_time;
int doWindFields;
int doMeteoSim;
int CalibrateAdjustments;
int seguir = 1;
float TEMP_VARIATION, HUM_VARIATION;
int FuelsUs[256];
int TotalFuels;

int runSimFarsite(INDVTYPE_FARSITE individual, char * simulationID, double * adjustmentError, int generation, char * atm, char * configurationFile, int myid,double Start,char * TracePathFiles, int JobID,int executed,int proc, int Trace, int Fuels, int * FuelsU,double AvailTime);
void initFarsiteVariables(char * filename, int generation);
void createInputFiles(INDVTYPE_FARSITE individual, char * configurationFile);
void createSettingsFile(char * settingsFile, int individualId, int generation, int perimeterResolution);
double getSimulationError(char * simulatedFireMap, char * realFireMap, char * ignitionFireMap, int start_time, int end_time);

void individualToString(INDVTYPE_FARSITE individual, char * pszIndividual, int buffersize) {
    if (!pszIndividual || buffersize<1) {
        *pszIndividual = '\0'; // return an 'empty' string 
    } else {
        sprintf(pszIndividual, "%d %d %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f", 
            generation, individual.id, 
            individual.parameters[0], individual.parameters[1], individual.parameters[2], individual.parameters[3],
            individual.parameters[4],
            individual.parameters[5], individual.parameters[6],
            individual.parameters[7], individual.parameters[8],
            individual.parameters[9], individual.parameters[10], individual.parameters[11], individual.parameters[12], individual.parameters[13], 
            individual.parameters[14], individual.parameters[15],
            individual.parameters[16], individual.parameters[17],
            individual.parameters[18], individual.parameters[19], individual.parameters[20]
        );
    }
    //pszIndividual[buffersize-1] = '\0'; // ensure a valid terminating zero! Many people forget this!
}

/**
 * - population (output): pointer to store the population
 * - populationFileName: path to the population file
 */
int readPopulation(POPULATIONTYPE * population, char * populationFileName) {
    
    FILE * populationFile;
    if ((populationFile = fopen(populationFileName, "r")) == NULL) {
        printf("ERROR: Farsite.readPopulation -> Population file can't be found or opened: %s \n", populationFileName);
        return -1;
    }

    //read the header: populationSize currentGeneration numberOfParams
    fscanf(populationFile,"%d %d %d\n", &population->popuSize, &population->currentGen, &population->nParams);

    population->popu_fs = (INDVTYPE_FARSITE *)malloc(sizeof(INDVTYPE_FARSITE) * population->popuSize);
    if( (population->nParams-2) > population->maxparams ) {
        printf("WARNING: Farsite.readPopulation -> The number of parameters specified in population file is greater than maxparams used in compilation.\n");
    };

    // read each individual
    int i,j;
    for (i = 0; i < population->popuSize; i++) {
        population->popu_fs[i].id = i;
        population->popu_fs[i].class_ind = 'A';
        population->popu_fs[i].threads = 1;
        for (j=0; j < (population->nParams-2); j++) {
            fscanf(populationFile,"%f ", &(population->popu_fs[i].parameters[j]));
        }
        population->popu_fs[i].nparams_farsite = population->nParams-2;
        fscanf(populationFile, "%f %f %d %d %d",
            &population->popu_fs[i].error, 
            &population->popu_fs[i].errorc,
            &population->popu_fs[i].executed,
            &population->popu_fs[i].oldid,
            &population->popu_fs[i].generation
        );
    }

    fclose(populationFile);

    return 0;
}

/**
 * - argv[1] file path: spif configuration file
 * - argv[2] file path: population file
 * - argv[3] int: identifier of the individual to be simulated. It should range between 0 to (population_size - 1).
 */
int main(int argc, char *argv[]) {

    if (argc < 3 ) {
        printf("ERROR: Farsite.main -> number of args invalid. Please inform at least a configuration and a population files. ");
        printf("You can optionally inform the individual to be executed.\n");
    }

    POPULATIONTYPE population;
    readPopulation(&population, argv[2]);
    //print_population_farsite(population);
    //print_individuo(0,population.popu_fs[0]);

    double adjustmentError;
    char * configurationFile = argv[1];
    char * atmPath;
    int generation = 0;
    int individuoId;
    int begin, end;
    char individualAsString[256]; // 256 bytes allocated here on the stack.
    char * adjustmentErrorsFileName = "output_individuals_adjustment_result.txt";
    FILE * adjustmentErrors;

    if (argc == 4 ) {
        individuoId = atoi(argv[3]);
        begin = individuoId;
        end = individuoId + 1;
    } else {
        begin = 0;
        end = population.popuSize;
    }

    if ((adjustmentErrors = fopen(adjustmentErrorsFileName, "w")) == NULL) {
        printf("ERROR: Farsite.main -> Error opening output adjustment errors file 'w' %s\n", adjustmentErrorsFileName);
    } else {
        int i;
        for (i=begin; i < end; i++) {
            printf("INFO: Farsite.main -> Going to start for individual (%d,%d)...\n", generation, i);

            individualToString(population.popu_fs[i], individualAsString, sizeof(individualAsString));
            printf("INFO: Farsite.main -> %s\n", individualAsString); // prints "Mar"

            runSimFarsite(population.popu_fs[i], "FARSITE", &adjustmentError, generation, atmPath, configurationFile, 99, 1, "/tmp/", 199, 7, 2, 1, 1,24,3600);

            printf("INFO: Farsite.main -> Finished for individual (%d,%d).\n", generation, i);

            printf("INFO: Farsite.main -> adjustmentError: (%d,%d): %f\n", generation, i, adjustmentError);
            printf("INFO: Farsite.main -> &adjustmentError: (%d,%d): %f\n", generation, i, &adjustmentError);

            fprintf(adjustmentErrors,"%s %f\n", individualAsString, adjustmentError);
        }   
        fclose(adjustmentErrors);
    }

}

int runSimFarsite(INDVTYPE_FARSITE individual, char * simulationID, double * adjustmentError, int generation, char * atm, char * configurationFile, int myid,double Start,char * TracePathFiles, int JobID,int executed,int proc,int Trace, int FuelsN, int * FuelsLoaded, double AvailTime) {
    //Init variables
    if(doWindFields == 1) {
        atmPath = atm;
    }
    char settings_filename[2000];
    char syscall[5000];
    double ti,te;
    double execuTime = 0.0f;
    char TraceBuffer[1000];
    sprintf(TraceBuffer,"");
    char TraceFileName[3000];
    FILE * TraceFile;
    sprintf(TraceFileName,"%sWorkerTrace_%d_%d_%d.dat",TracePathFiles,myid,JobID,executed);
    initFarsiteVariables(configurationFile, generation);
    sprintf(settings_filename,"%ssettings_%d_%d.txt",output_path,generation,individual.id);

    createInputFiles(individual, configurationFile);
    int resolution=100;
    switch(individual.class_ind) {
        case 'E':
            resolution=100;
            break;
        case 'D':
            resolution=100;
            break;
        default:
            resolution=100;
            break;
    }
    if (generation==10) {
        AvailTime=3600.0;
        resolution=30;
        individual.threads=8;
    };
    createSettingsFile(settings_filename, individual.id, generation, resolution);

    if (AvailTime<=1.0f) {
        AvailTime=1.0;
    };

    char * individualAsString[256];
    individualToString(individual, individualAsString, sizeof(individual));
    sprintf(syscall,"/usr/bin/time --format \"%s %%e %%M %%O %%P %%c %%x\" -a --output=time_output.txt timeout --signal=SIGXCPU %.0f %sfarsite4P -i %s -f %d -t 1 -g %d -n %d -w %d -p %dm",
        individualAsString,
        AvailTime, farsite_path, settings_filename, individual.threads, generation, individual.id, myid, resolution
    );

    /*********** LLAMADA FARSITE ORIGINAL ***********/
    ti = MPI_Wtime();
    int err_syscall = system(syscall);
    te = MPI_Wtime();
    execuTime = te-ti;

    if (err_syscall == 0) {
        char sim_fire_line[5000];
        sprintf(sim_fire_line,"%s%s.toa", output_path, RasterFileNameNew);
        *adjustmentError = getSimulationError(sim_fire_line, real_fire_map_t1, real_fire_map_tINI, start_time, end_time);
        sprintf(TraceBuffer,"Worker%d %1.2f %1.2f %d %d %d %d\n",myid,ti-Start,te-Start,myid,individual.threads,proc,1);
    } else {
        //printf( "FARSITE:%d:%d_%d_%d_%f_%f_%d_%s\n",myid,generation,individual.id,individual.threads,(float)AvailTime,(float)AvailTime,myid,perimeterResolution);
        *adjustmentError = ERROR_INFINITY;
        sprintf(TraceBuffer,"Worker%d %1.2f %1.2f %d %d %d %d\n",myid,ti-Start,te-Start,myid,individual.threads,proc,0);
    }

    if (Trace) {
        if (( TraceFile = fopen(TraceFileName, "w")) == NULL) {
            printf("ERROR: Farsite.runSimFarsite -> Error opening trace file 'w' %s\n", TraceFileName);
        } else {
            fprintf(TraceFile, "%s", TraceBuffer);
            fclose(TraceFile);
        }
    }
    return (EXIT_SUCCESS);
}

void initFarsiteVariables(char * configurationFile, int generation) {
    dictionary * configuration = iniparser_load(configurationFile);

    numGenerations           = iniparser_getint(configuration, "genetic:numGenerations",1);
    num_threads              = iniparser_getint(configuration, "main:num_threads",1);
    doWindFields    	     = iniparser_getint(configuration, "main:doWindFields", 0);
    doMeteoSim    	         = iniparser_getint(configuration, "main:doMeteoSim", 0);
    CalibrateAdjustments     = iniparser_getint(configuration, "main:CalibrateAdjustments",0);
    FuelsToCalibrateFileName = iniparser_getstr(configuration, "main:FuelsToCalibrate");

    CustomFuelFile   = iniparser_getstr(configuration, "farsite:CustomFuelFile");
    farsite_path     = iniparser_getstr(configuration, "farsite:farsite_path");
    input_path       = iniparser_getstr(configuration, "farsite:input_path");
    output_path      = iniparser_getstr(configuration, "farsite:output_path");
    landscapeFile    = iniparser_getstr(configuration, "farsite:landscapeFile");
    adjFile          = iniparser_getstr(configuration, "farsite:adjFile");
    ignitionFile     = iniparser_getstr(configuration, "farsite:ignitionFile");
    ignitionFileType = iniparser_getstr(configuration, "farsite:ignitionFileType");
    wndFile          = iniparser_getstr(configuration, "farsite:wndFile");
    wtrFile          = iniparser_getstr(configuration, "farsite:wtrFile");
    fmsFile          = iniparser_getstr(configuration, "farsite:fmsFile");
    baseWndFile      = iniparser_getstr(configuration, "farsite:baseWndFile");
    baseWtrFile      = iniparser_getstr(configuration, "farsite:baseWtrFile");
    baseFmsFile      = iniparser_getstr(configuration, "farsite:baseFmsFile");
    baseAdjFile	     = iniparser_getstr(configuration, "farsite:baseAdjFile");
    RasterFileName   = iniparser_getstr(configuration, "farsite:RasterFileName");
    shapefile        = iniparser_getstr(configuration, "farsite:shapefile");
    VectorFileName   = iniparser_getstr(configuration, "farsite:VectorFileName");
    doRaster         = iniparser_getstr(configuration, "farsite:doRaster");
    doShape          = iniparser_getstr(configuration, "farsite:doShape");
    doVector         = iniparser_getstr(configuration, "farsite:doVector");
    if (generation == numGenerations) {
        ignitionFile        = iniparser_getstr(configuration, "prediction:PignitionFile");
        ignitionFileType    = iniparser_getstr(configuration, "prediction:PignitionFileType");
        ConditMonth         = iniparser_getstr(configuration, "prediction:PConditMonth");
        ConditDay           = iniparser_getstr(configuration, "prediction:PConditDay");
        StartMonth          = iniparser_getstr(configuration, "prediction:PStartMonth");
        StartDay            = iniparser_getstr(configuration, "prediction:PStartDay");
        StartHour           = iniparser_getstr(configuration, "prediction:PStartHour");
        StartMin            = iniparser_getstr(configuration, "prediction:PStartMin");
        EndMonth            = iniparser_getstr(configuration, "prediction:PEndMonth");
        EndDay              = iniparser_getstr(configuration, "prediction:PEndDay");
        EndHour             = iniparser_getstr(configuration, "prediction:PEndHour");
        EndMin              = iniparser_getstr(configuration, "prediction:PEndMin");
        start_time          = iniparser_getint(configuration, "prediction:Pstart_time",1);
        end_time            = iniparser_getint(configuration, "prediction:Pend_time",1);
        real_fire_map_t0    = iniparser_getstr(configuration, "prediction:Preal_fire_map_t0");
        real_fire_map_t1    = iniparser_getstr(configuration, "prediction:Preal_fire_map_t1");
        real_fire_map_tINI  = iniparser_getstr(configuration, "prediction:Preal_fire_map_tINI");
    } else {
        ignitionFile        = iniparser_getstr(configuration, "farsite:ignitionFile");
        ignitionFileType    = iniparser_getstr(configuration, "farsite:ignitionFileType");
        ConditMonth         = iniparser_getstr(configuration, "farsite:ConditMonth");
        ConditDay           = iniparser_getstr(configuration, "farsite:ConditDay");
        StartMonth          = iniparser_getstr(configuration, "farsite:StartMonth");
        StartDay            = iniparser_getstr(configuration, "farsite:StartDay");
        StartHour           = iniparser_getstr(configuration, "farsite:StartHour");
        StartMin            = iniparser_getstr(configuration, "farsite:StartMin");
        EndMonth            = iniparser_getstr(configuration, "farsite:EndMonth");
        EndDay              = iniparser_getstr(configuration, "farsite:EndDay");
        EndHour             = iniparser_getstr(configuration, "farsite:EndHour");
        EndMin              = iniparser_getstr(configuration, "farsite:EndMin");
        start_time          = iniparser_getint(configuration, "farsite:start_time",1);
        end_time            = iniparser_getint(configuration, "farsite:end_time",1);
        real_fire_map_t0    = iniparser_getstr(configuration, "farsite:real_fire_map_t0");
        real_fire_map_t1    = iniparser_getstr(configuration, "farsite:real_fire_map_t1");
        real_fire_map_tINI  = iniparser_getstr(configuration, "farsite:real_fire_map_tINI");
    }

    timestep             = iniparser_getstr(configuration, "farsite:timestep");
    visibleStep          = iniparser_getstr(configuration, "farsite:visibleStep");
    secondaryVisibleStep = iniparser_getstr(configuration, "farsite:secondaryVisibleStep");
    perimeterResolution  = iniparser_getstr(configuration, "farsite:perimeterResolution");
    distanceResolution   = iniparser_getstr(configuration, "farsite:distanceResolution");
    TEMP_VARIATION       = iniparser_getdouble(configuration, "farsite:TEMP_VARIATION",1.0);
    HUM_VARIATION        = iniparser_getdouble(configuration, "farsite:HUM_VARIATION",1.0);
    FireSimLimit         = iniparser_getint(configuration, "farsite:ExecutionLimit", 1);

    if (CalibrateAdjustments) {
        FILE *FuelsToCalibrateFILE;
        int i,nFuel;

        for (i=0; i<256; i++) {
            FuelsUs[i]=0;
        }
        if ((FuelsToCalibrateFILE = fopen(FuelsToCalibrateFileName,"r"))==NULL) {
            printf("ERROR: Farsite.initFarsiteVariables -> Opening fuels used file.\n");
        } else {
            while(fscanf(FuelsToCalibrateFILE,"%d",&nFuel)!=EOF) {
                FuelsUs[nFuel-1]=1;
            }
        }
    }

}

/**
 * Create input files to be used in farsite simulation (fms, adj, wnd, wtr).
 * - individual
 * - configurationFile
 */
void createInputFiles(INDVTYPE_FARSITE individual, char * configurationFile) {
    char * line = (char*)malloc(sizeof(char) * 200);
    char * newline = (char*)malloc(sizeof(char) * 200);
    char * buffer = (char*)malloc(sizeof(char) * 200);

    fmsFileNew = (char*)malloc(sizeof(char) * 200);
    adjFileNew = (char*)malloc(sizeof(char) * 200);

    if (doMeteoSim == 0) {
        wndFileNew = (char*)malloc(sizeof(char) * 200);
        wtrFileNew = (char*)malloc(sizeof(char) * 200);
    }
    FILE * fFMS, *fWND, *fWTR, *fADJ, *fFMSnew, *fWNDnew, *fWTRnew,*fADJnew;
    char * tmp = (char*)malloc(sizeof(char) * 400);
    // Create corresponding fms,wnd & wtr filename for each individual
    sprintf(tmp,"%d",generation);

    fmsFileNew = str_replace(fmsFile, "$1", tmp);

    if (CalibrateAdjustments) {
        adjFileNew = str_replace(adjFile, "$1", tmp);
    }

    if(doMeteoSim == 0) {
        wndFileNew = str_replace(wndFile,"$1", tmp);
        wtrFileNew = str_replace(wtrFile,"$1", tmp);
    }
    sprintf(tmp,"%d",individual.id);
    fmsFileNew = str_replace(fmsFileNew, "$2", tmp);
    if (CalibrateAdjustments) {
        adjFileNew = str_replace(adjFileNew, "$2", tmp);
    }


    if(doMeteoSim == 0) {
        wndFileNew = str_replace(wndFileNew,"$2", tmp);
        wtrFileNew = str_replace(wtrFileNew,"$2", tmp);
    }

    if ((fFMS = fopen(baseFmsFile, "r")) == NULL) {
        printf("ERROR: Farsite.createInputFiles -> Unable to open FMS file");
        seguir = 0;
    }
    if(doMeteoSim == 0) {
        if(((fWND = fopen(baseWndFile, "r")) == NULL) || ((fWTR = fopen(baseWtrFile, "r")) == NULL) ) {
            printf("ERROR: Farsite.createInputFiles -> Unable to open WND or WTR files\n");
            seguir = 0;
        }
    }
    if(seguir == 1) {
        if((fFMSnew = fopen(fmsFileNew, "w")) == NULL) {
            printf("ERROR: Farsite.createInputFiles -> Unable to create FMS temp file\n");
            seguir = 0;
        }
        if(doMeteoSim == 0) {
            if(((fWNDnew = fopen(wndFileNew, "w")) == NULL) || ((fWTRnew = fopen(wtrFileNew, "w")) == NULL) ) {
                printf("ERROR: Farsite.createInputFiles -> Unable to create WND or WTR temp files\n");
                seguir = 0;
            }
        }
        if(seguir == 1) {
            while(fgets( line, 100, fFMS ) != NULL) {
                sprintf(buffer,"%1.0f",individual.parameters[0]);
                newline = str_replace(line, "1h", buffer);
                sprintf(buffer,"%1.0f",individual.parameters[1]);
                newline = str_replace(newline, "10h", buffer);
                sprintf(buffer,"%1.0f",individual.parameters[2]);
                newline = str_replace(newline, "100h", buffer);
                sprintf(buffer,"%1.0f",individual.parameters[3]);
                newline = str_replace(newline, "herb", buffer);
                fprintf(fFMSnew,"%s", newline);
            }
            fclose(fFMSnew);

            if(CalibrateAdjustments) {
                if(((fADJnew = fopen(adjFileNew, "w")) == NULL) || (fADJ = fopen(baseAdjFile, "r")) == NULL) {
                    printf("ERROR: Farsite.createInputFiles -> Unable to create ADJ temp file\n");
                    seguir = 0;
                }
                int nfuel,param=9;
                float adjust=0.0f;
                while(fscanf(fADJ,"%d %f",&nfuel,&adjust)!= EOF ) {
                    if (FuelsUs[nfuel-1]) {
                        fprintf(fADJnew,"%d %1.6f\n",nfuel,individual.parameters[param]);
                        param++;
                    } else {
                        fprintf(fADJnew,"%d 1.000000\n",nfuel);
                    }
                }
                fclose(fADJnew);
                fclose(fADJ);
            }

            if(doMeteoSim == 0) {
                fgets( line, 100, fWND );
                fprintf(fWNDnew,"%s", line);
                while(fgets( line, 100, fWND ) != NULL) {
                    sprintf(buffer,"%1.0f",individual.parameters[5]);
                    newline = str_replace(line, "ws", buffer);
                    sprintf(buffer,"%1.0f",individual.parameters[6]);
                    newline = str_replace(newline, "wd", buffer);
                    sprintf(buffer,"%d",0);
                    newline = str_replace(newline, "wc", buffer);
                    fprintf(fWNDnew,"%s", newline);
                }
                if(doMeteoSim == 0) {
                    fclose(fWNDnew);
                }

                fgets( line, 100, fWTR );
                fprintf(fWTRnew,"%s", line);
                float tl = individual.parameters[7] - TEMP_VARIATION;
                float hl = individual.parameters[8] - HUM_VARIATION;

                while(fgets( line, 100, fWTR ) != NULL) {
                    sprintf(buffer,"%1.0f",tl);
                    newline = str_replace(line, "tl", buffer);

                    sprintf(buffer,"%1.0f",individual.parameters[7]);
                    newline = str_replace(newline, "th", buffer);

                    sprintf(buffer,"%1.0f", individual.parameters[8]);
                    newline = str_replace(newline, "hh", buffer);

                    sprintf(buffer,"%1.0f",hl);
                    newline = str_replace(newline, "hl", buffer);

                    fprintf(fWTRnew,"%s", newline);
                }
                if(doMeteoSim == 0) {
                    fclose(fWTRnew);
                }
            }
        }
        fclose(fFMS);
        if(doMeteoSim == 0) {
            fclose(fWND);
            fclose(fWTR);
        }
    }
}

/**
 * Create settings file to be used in farsite simulation.
 * - settingsFile
 * - individualId
 * - generation
 * - resolution
 */
void createSettingsFile(char * settingsFile, int individualId, int generation, int perimeterResolution) {
    char * shapefileNew = (char*)malloc(sizeof(char) * 400);
    RasterFileNameNew = (char*)malloc(sizeof(char) * 400);
    char * VectorFileNameNew = (char*)malloc(sizeof(char) * 400);
    char * tmp = (char*)malloc(sizeof(char) * 400);

    FILE * settings;

    if ( (settings = fopen(settingsFile, "w")) == NULL ) {
        printf("ERROR: Farsite.createSettingsFile -> Unable to open settings file");
    } else {
        sprintf(tmp,"%d",generation);
        shapefileNew = str_replace(shapefile, "$1", tmp);
        RasterFileNameNew = str_replace(RasterFileName,"$1", tmp);
        VectorFileNameNew = str_replace(VectorFileName,"$1", tmp);
        sprintf(tmp,"%d",individualId);
        shapefileNew = str_replace(shapefileNew, "$2", tmp);
        RasterFileNameNew = str_replace(RasterFileNameNew,"$2", tmp);
        VectorFileNameNew = str_replace(VectorFileNameNew,"$2", tmp);

        fprintf(settings,"version = 43\n");
        fprintf(settings,"landscapeFile = %s\n", landscapeFile);
        fprintf(settings,"FUELMOISTUREFILE = %s\n", fmsFileNew);
        if (CustomFuelFile != NULL) {
            fprintf(settings,"fuelmodelfile = %s\n",CustomFuelFile);
        }
        if(doWindFields == 0) {
            if(doMeteoSim == 0) {
                fprintf(settings,"windFile0 = %s\n", wndFileNew);
            } else {
                fprintf(settings,"windFile0 = %s\n", wndFile);
            }
        } else {
            if(doMeteoSim == 0) {
                fprintf(settings,"windFile0 = %s\n", atmPath);
            }
            if(doMeteoSim == 1) {
                fprintf(settings,"windFile0 = %s\n", wndFile);
            }
        }
        if(CalibrateAdjustments) {
            fprintf(settings,"adjustmentFile = %s\n", adjFileNew);
        } else {
            fprintf(settings,"adjustmentFile = %s\n", baseAdjFile);
        }
        if(doMeteoSim == 0) {
            fprintf(settings,"weatherFile0 = %s\n", wtrFileNew);
        } else {
            fprintf(settings,"weatherFile0 = %s\n", wtrFile);
        }

        fprintf(settings,"\n");

        // OUTPUTS CREATION
        fprintf(settings,"vectMake = %s\n", doVector);
        fprintf(settings,"rastMake = %s\n", doRaster);
        fprintf(settings,"shapeMake = %s\n", doShape);

        fprintf(settings,"\n");

        // RESOLUTION
        fprintf(settings,"timestep = %s\n", timestep);
        fprintf(settings,"visibleStep = %s\n", visibleStep);
        fprintf(settings,"secondaryVisibleStep = %s\n", secondaryVisibleStep);        
        fprintf(settings,"perimeterResolution = %dm\n", perimeterResolution);
        fprintf(settings,"distanceResolution = %s\n", distanceResolution);

        fprintf(settings,"\n");

        // IGNITION DATA & TYPE
        fprintf(settings,"ignitionFile = %s\n", ignitionFile);
        fprintf(settings,"ignitionFileType = %s\n", ignitionFileType);
        if(strcmp(doShape,"true") == 0) {
            fprintf(settings,"shapefile = %s%s.shp\n", output_path, shapefileNew);
        }
        if(strcmp(doRaster,"true") == 0) {
            fprintf(settings,"RasterFileName = %s%s\n", output_path, RasterFileNameNew);
        }
        if(strcmp(doVector,"true") == 0) {
            fprintf(settings,"VectorFileName = %s%s\n", output_path, VectorFileNameNew);
        }

        fprintf(settings,"\n");

        fprintf(settings,"enableCrownfire = false\n");
        fprintf(settings,"linkCrownDensityAndCover = false\n");
        fprintf(settings,"embersFromTorchingTrees = false\n");
        fprintf(settings,"enableSpotFireGrowth = false\n");
        fprintf(settings,"nwnsBackingROS = false\n");
        fprintf(settings,"fireacceleration = false\n");
        fprintf(settings,"accelerationtranstion = 1m\n");
        fprintf(settings,"distanceChecking = fireLevel\n");
        fprintf(settings,"simulatePostFrontalCombustion = false\n");
        fprintf(settings,"fuelInputOption = absent\n");
        fprintf(settings,"calculationPrecision = normal\n");

        fprintf(settings,"\n");

        fprintf(settings,"useConditioningPeriod = false\n");
        fprintf(settings,"ConditMonth = %s\n",ConditMonth);
        fprintf(settings,"ConditDay = %s\n", ConditDay);
        fprintf(settings,"StartMonth = %s\n", StartMonth);
        fprintf(settings,"StartDay = %s\n", StartDay);
        fprintf(settings,"StartHour = %s\n", StartHour);
        fprintf(settings,"StartMin = %s\n", StartMin);
        fprintf(settings,"EndMonth = %s\n", EndMonth);
        if (generation==10) {
            fprintf(settings,"EndDay = %d\n", atoi(StartDay)+1);
        } else {
            fprintf(settings,"EndDay = %s\n", EndDay);
        }
        fprintf(settings,"EndHour = %s\n", EndHour);
        fprintf(settings,"EndMin = %s\n", EndMin);

        fprintf(settings,"\n");

        fprintf(settings, "rast_arrivaltime = true\n");
        fprintf(settings, "rast_fireIntensity = false\n");
        fprintf(settings, "rast_spreadRate = false\n");
        fprintf(settings, "rast_flameLength = false\n");
        fprintf(settings, "rast_heatPerArea = false\n");
        fprintf(settings, "rast_crownFire = false\n");
        fprintf(settings, "rast_fireDirection = false\n");
        fprintf(settings, "rast_reactionIntensity = false\n");

        fclose(settings);
    }
}

/**
 * All the files must be provided in GRASS ASCII format (see https://grass.osgeo.org/grass75/manuals/r.in.ascii.html) with one data value per line.
 * For example, the 2x3 grid
 * 1 2 3
 * 4 5 6
 * should be formated as: 
 * 1 
 * 2 
 * 3 
 * 4 
 * 5 
 * 6
 * - simulatedFireMap (input): path to the farsite output file (.toa).
 * - realFireMap: path to the raster map file with the real fire evolution.
 * - ignitionFireMap: path to the raster map file with the ignition fire evolution (initial fire perimeter).
 * - startTime: the simulation start time in hours
 * - endTime the simulation end time in hours
 */
double getSimulationError(char * simulatedFireMap, char * realFireMap, char * ignitionFireMap, int startTime, int endTime) {
    //Args: mode mapaRealFileName mapaSimFileName t1 t2
    FILE *fd,*fd2;
    char tmp;
    char name[20];
    int i, n, j, srows, scols, rrows=0, rcols=0, aux;
    double *realMap, *simulatedMap;
    double error=ERROR_INFINITY, fitness, doubleValue, val;

    if(((fd = fopen(realFireMap, "r")) == NULL) || ((fd2 = fopen(ignitionFireMap, "r")) == NULL) ) {
        printf("ERROR: Farsite.getSimulationError -> Unable to open real map file or ignition fire map file\n");
    } else {
        fscanf(fd,"%7s%f\n%7s%f\n%7s%f\n%7s%f\n%7s%d\n%7s%d\n",name,&val,name,&val,name,&val,name,&val,name,&rrows,name,&rcols);
        printf("INFO: Farsite.getSimulationError.realFireMap.fd -> name(%7s) val(%f) rrows(%d) rcols(%d)\n",name,val,rrows,rcols);
        fscanf(fd2,"%7s%f\n%7s%f\n%7s%f\n%7s%f\n%7s%d\n%7s%d\n",name,&val,name,&val,name,&val,name,&val,name,&aux,name,&aux);
        printf("INFO: Farsite.getSimulationError.ignitionFireMap.fd2 -> name(%7s) val(%f) aux(%d) \n",name,val,aux);
        realMap=(double *)calloc(rrows*rcols,sizeof(double));

        for(i=0; i<rrows*rcols; i++) {
            fscanf(fd2,"%lf\n",&doubleValue);
            if(doubleValue==1.0f) {
                fscanf(fd,"%lf\n",&realMap[i]);
                realMap[i]=-1.0f;
            } else {
                fscanf(fd,"%lf\n",&realMap[i]);
            }
        }
        fclose(fd);
        fclose(fd2);

        if((fd=fopen(simulatedFireMap,"r")) == NULL) {
            printf("ERROR: Unable to open simulated map file");
        } else {
            fscanf(fd,"%7s%f\n%7s%f\n%7s%f\n%7s%f\n%7s%d\n%7s%d\n",name,&val,name,&val,name,&val,name,&val,name,&srows,name,&scols);
            printf("INFO: Farsite.getSimulationError.simulatedFireMap.fd -> name(%7s) val(%f) srows(%d) scols(%d)\n",name,val,srows,scols);
            simulatedMap=(double *)calloc(srows*scols,sizeof(double));

            if( (srows!=rrows) || (scols!=rcols) ) {
                printf("ERROR: Farsite.getSimulationError -> Different map dimensions: Real (%dx%d) X Simulated (%dx%d)\n",rrows,rcols,srows,scols);
            } else {
                for(i=0; i< srows*scols; i++) {
                    fscanf(fd,"%lf\n",&simulatedMap[i]);
                }
                fclose(fd);
                fitness = fitnessYError(realMap, simulatedMap, rrows, rcols, startTime, endTime, &error);
                free(realMap);
                free(simulatedMap);
            }
        }
    }
    return error;
}
