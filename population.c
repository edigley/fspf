/*********************************************************************/
/* population.c     			  			                                 */
/* rutinas para manejo de la poblacion y de los individuos           */
/*********************************************************************/

#include <stdlib.h>
#include "population.h"

void init_population(POPULATIONTYPE *p, int popuSize,int nFuels)
{

    p->currentGen = 0;
    p->maxGen = 0;
    p->totfit = 0;
    p->maxError = 0.0;
    p->nparams_farsite = FarsiteFixVariables + nFuels;
    p->maxparams = FarsiteFixVariables + nFuels + 2;

    int nparams_farsite;				  		// number of parameters of each individuali
    int maxparams;

    p->popuSize = popuSize;
    p->popu = (INDVTYPE *)malloc(sizeof(INDVTYPE)*popuSize);
    p->popu_fs = (INDVTYPE_FARSITE *)malloc(sizeof(INDVTYPE_FARSITE)*popuSize);
    int i = 0;
    for(i=0; i<popuSize; i++)
    {
        p->popu_fs[i].id = i;
        p->popu_fs[i].oldid = i;
        p->popu_fs[i].Time = 0.0f;
        p->popu_fs[i].nparams_farsite = FarsiteFixVariables + nFuels;
        p->popu_fs[i].maxparams = FarsiteFixVariables + nFuels + 2;
    }
    //p->labels = (char**)malloc(sizeof(char*)*p->nParams);
}

void print_indv_default(INDVTYPE indv)
{
    int i;

    // print number of parameters
    printf(" %d  ||  ", indv.n);

    // print each parameter
    for (i=0; i<indv.n; i++)
        printf("%1.2f\t", indv.p[i]);

    // print individual attributes
    printf(" || %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %d %d %s\n", indv.fit, indv.dist, indv.dir, indv.vel, indv.error, indv.errorc, indv.wnddir, indv.wndvel,indv.threads,indv.offset,indv.class_ind);

}

void print_indv_farsite(INDVTYPE_FARSITE indv)
{
    // printf("%d - %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %d\n", indv.id, indv.m1, indv.m10, indv.m100, indv.mherb, indv.wndvel, indv.wnddir, indv.temp, indv.hum, indv.error, indv.errorc,indv.ExecTime,indv.executed);
    int j;
    for (j=0; j<indv.nparams_farsite; j++)
    {
        printf("%f ",indv.parameters[j]);
    }
    printf("%f %f %d\n",indv.error, indv.errorc,indv.executed);
}

int get_population_farsite(POPULATIONTYPE * pobla, char * nombreInitSet)
{
    FILE * fichero;
    int i,j;


    if ((fichero = fopen(nombreInitSet, "r")) == NULL)
    {
        printf("(get_population_farsite)-> Population file can't be found or opened!: %s \n", nombreInitSet);
        return -1;
    }

    //printf("(get_population_farsite)-> Reading population (%s) \n", nombreInitSet);
    fscanf(fichero,"%d %d %d\n", &pobla->popuSize, &pobla->currentGen, &pobla->nParams);

    pobla->popu_fs = (INDVTYPE_FARSITE *)malloc(sizeof(INDVTYPE_FARSITE) * pobla->popuSize);
    if((pobla->nParams-2)>pobla->maxparams)
    {
        printf("ERROR:The number of parameters specified in population files is greater than maxparams used in compilation.\n");
    };
    // read each individual
    for (i = 0; i <pobla->popuSize; i++)
    {
        // read each parameter
//        fscanf(fichero, "%f %f %f %f %f %f %f %f %f %f %d", &pobla->popu_fs[i].m1, &pobla->popu_fs[i].m10, &pobla->popu_fs[i].m100, &pobla->popu_fs[i].mherb, &pobla->popu_fs[i].wndvel, &pobla->popu_fs[i].wnddir, &pobla->popu_fs[i].temp, &pobla->popu_fs[i].hum, &pobla->popu_fs[i].error, &pobla->popu_fs[i].errorc,&pobla->popu_fs[i].executed);
        pobla->popu_fs[i].id = i;
        pobla->popu_fs[i].class_ind = 'A';
        pobla->popu_fs[i].threads = 1;
//	pobla->popu_fs[i].parameters = (float*)malloc(sizeof(float)*pobla->nParams);
        //printf("Parametros a leer por individuo:%d\n",pobla->popu_fs[i].nparams_farsite);
        for (j=0; j<(pobla->nParams-2); j++)
        {
            fscanf(fichero,"%f ", &(pobla->popu_fs[i].parameters[j]));
            //printf("%f ",pobla->popu_fs[i].parameters[j]);
        }
        pobla->popu_fs[i].nparams_farsite=pobla->nParams-2;
        fscanf(fichero,"%f %f %d %d %d",&pobla->popu_fs[i].error, &pobla->popu_fs[i].errorc,&pobla->popu_fs[i].executed,&pobla->popu_fs[i].oldid,&pobla->popu_fs[i].generation);
        //printf("%f %f %d\n",pobla->popu_fs[i].error, pobla->popu_fs[i].errorc,pobla->popu_fs[i].executed);
    }

    fclose(fichero);

    return 1;
}

int get_population_default(POPULATIONTYPE * pobla, char * nombreInitSet)
{
    FILE * fichero;
    int i, cantParams, x;


    if ((fichero = fopen(nombreInitSet, "r")) == NULL)
    {
        printf("(get_population_default)-> Population file can't be found or opened!: %s \n", nombreInitSet);
        return -1;
    }

    printf("Leyendo poblacion (%s) \n", nombreInitSet);
    fscanf(fichero,"%d %d %d\n", &pobla->popuSize, &pobla->currentGen, &pobla->nParams);
    pobla->popu = (INDVTYPE *)malloc(sizeof(INDVTYPE) * pobla->popuSize);

    // read each individual
    for (i = 0; i <pobla->popuSize; i++)
    {
        // read each parameter
        for (x = 0; x < pobla->popu[i].n; x ++)
            fscanf(fichero, "%f", &pobla->popu[i].p[x]);

        // read each attribute
        fscanf(fichero, "%f %f %f %f %f %f %f %f ", &pobla->popu[i].fit, &pobla->popu[i].dist, &pobla->popu[i].dir, &pobla->popu[i].vel, &pobla->popu[i].error, &pobla->popu[i].errorc, &pobla->popu[i].wnddir, &pobla->popu[i].wndvel);
    }
}


// Print population in a file with filename "nombreSet"
int save_population_farsite(POPULATIONTYPE pobla, char * nombreSet)
{
    FILE * fichero;
    int i,j, x, cantParams;

    if (!(fichero = fopen(nombreSet, "w+")))
    {
        printf("(print_population)-> Population file can't be found or opened! (%s)", nombreSet);
        return -1;
    }
    cantParams = 9;
    printf("(print_population)-> Writing population in: %s \n", nombreSet);
    // first line: number of individuals in the population
    fprintf(fichero, "%d %d %d\n", pobla.popuSize, pobla.currentGen, pobla.nParams);

//    for (i = 0; i < pobla.popuSize; i++) //write each individual
    //      fprintf(fichero,"%1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.2f %1.4f %1.4f %d\n",pobla.popu_fs[i].m1, pobla.popu_fs[i].m10, pobla.popu_fs[i].m100, pobla.popu_fs[i].mherb, pobla.popu_fs[i].wndvel, pobla.popu_fs[i].wnddir, pobla.popu_fs[i].temp, pobla.popu_fs[i].hum, pobla.popu_fs[i].error, pobla.popu_fs[i].errorc,pobla.popu_fs[i].executed);
    for (i=0; i<pobla.popuSize; i++)
    {
        for(j=0; j<pobla.nParams-2; j++)
        {
            fprintf(fichero,"%f ",pobla.popu_fs[i].parameters[j]);
        }
        fprintf(fichero,"%f %f %d %d %d\n",pobla.popu_fs[i].error, pobla.popu_fs[i].errorc,pobla.popu_fs[i].executed,pobla.popu_fs[i].oldid,pobla.popu_fs[i].generation);
    }

    fclose(fichero);

    return 1;
}

int save_population_default(POPULATIONTYPE pobla, char * nombreSet)
{
    FILE * fichero;
    int i, x, cantParams;

    if (!(fichero = fopen(nombreSet, "w")))
    {
        printf("(print_population)-> Population file can't be found or opened! (%s)", nombreSet);
        return -1;
    }

    printf("(print_population)-> Writing population in: %s \n", nombreSet);
    // first line: number of individuals in the population
    fprintf(fichero, "%d %d \n", pobla.popuSize, pobla.currentGen);

    for (i = 0; i < pobla.popuSize; i++)
    {
        // write each parameter
        cantParams= pobla.popu[i].n;      // number of individual parameters
        fprintf(fichero, "%d ", cantParams);

        for (x = 0; x < cantParams; x ++)
            fprintf(fichero," %f ", pobla.popu[i].p[x]);

        fprintf(fichero,"%f %f %f %f %f %f %f %f", pobla.popu[i].fit, pobla.popu[i].dist, pobla.popu[i].dir, pobla.popu[i].vel, pobla.popu[i].error, pobla.popu[i].errorc, pobla.popu[i].wnddir, pobla.popu[i].wndvel);
        fprintf(fichero,"\n");
    }
    fclose(fichero);

    return 1;
}

void print_population_farsite(POPULATIONTYPE p)
{
    int i = 0;
    printf("POPULATION -> size = %d\n",p.popuSize);
    for(i=0; i < p.popuSize; i++)
        print_indv_farsite(p.popu_fs[i]);

}

// imprime la poblacion pasado por parametro por pantalla
int print_populationScreen(POPULATIONTYPE pobla)
{

    int i, x, cantParams;
    // first line: number of individuals in the population
    printf("%d %d \n", pobla.popuSize, pobla.currentGen);
    // cols headers
    printf("     LOAD     M1     M10     M100   MHERB    SAVR      DEPTH    MEXT   WNSPD   WNDIR   FIT     DIST    DIRE    SPD    ERROR    ERRORC    WNDIR    WNSPD \n");

    for (i = 0; i < pobla.popuSize; i++)
    {
        // write each parameter
        cantParams= pobla.popu[i].n;      // number of individual parameters
        printf("%d ", cantParams);

        for (x = 0; x < cantParams; x ++)
            printf(" %1.3f  ", pobla.popu[i].p[x]);

        printf(" %1.2f    %1.2f    %1.2f    %1.2f    %1.2f    %1.2f    %1.2f    %1.2f  ", pobla.popu[i].fit, pobla.popu[i].dist, pobla.popu[i].dir, pobla.popu[i].vel, pobla.popu[i].error, pobla.popu[i].errorc, pobla.popu[i].wnddir,  pobla.popu[i].wndvel);

        printf("\n");
    }

    return 1;
}


// obtiene el individuo con indice ind de la poblacion p
int get_indv(POPULATIONTYPE *p, int indi, INDVTYPE * indv)
{
    int x;

    indv->n = p->popu[indi].n;

    for (x = 0; x < indv->n; x ++)
        indv->p[x] = p->popu[indi].p[x];


    indv->fit = p->popu[indi].fit;
    indv->dist = p->popu[indi].dist;
    indv->dir = p->popu[indi].dir;
    indv->vel = p->popu[indi].vel;
    indv->error = p->popu[indi].error;
    indv->errorc = p->popu[indi].errorc;
    indv->wnddir = p->popu[indi].wnddir;
    indv->wndvel = p->popu[indi].wndvel;

    return 1;
}


// ocsolote: ara cal que ordini per error i no per fitness
// OBSOLETO!!!!!!!!! SI SE VUELVE A UTILIZAR AGREGAR EL SWAP DE LOS CAMPOS AGREGADOS:
// ERROR, ERRORC, WNDDIR, WNDVEL
int sortPopulationByFitness(POPULATIONTYPE * p)
{
    int i, j, max, k;
    float tmp;

    for (i = 0; i < p->popuSize; i++)
    {
        // busco el i-esimo maximo

        max = i;
        for (j = i+1; j < p->popuSize; j++)
            if (p->popu[j].fit > p->popu[max].fit)
                max = j;

        // swapeo los individuos si cal
        if (max != i)
        {
            //swapeo los parametros
            for (k = 0; k < p->popu[i].n; k++)
            {
                tmp = p->popu[i].p[k];
                p->popu[i].p[k] = p->popu[max].p[k];
                p->popu[max].p[k] = tmp;
            }

            // swapeo los atributos
            tmp = p->popu[i].fit;               // fitness
            p->popu[i].fit = p->popu[max].fit;
            p->popu[max].fit = tmp;

            tmp = p->popu[i].dist;              // distancia
            p->popu[i].dist = p->popu[max].dist;
            p->popu[max].dist = tmp;

            tmp = p->popu[i].dir;               // direccion
            p->popu[i].dir = p->popu[max].dir;
            p->popu[max].dir = tmp;

            tmp = p->popu[i].vel;               // velocidad
            p->popu[i].vel = p->popu[max].vel;
            p->popu[max].vel = tmp;

        } // if max != i
    }

    return 1;
}

int sortPopulationByErrorFarsite(POPULATIONTYPE * p)
{
    int i, j, t, min, idtmp;
    float tmp;

    for (i = 0; i < p->popuSize; i++)
    {
        // busco el i-esimo minimo

        min = i;
        for (j = i+1; j < p->popuSize; j++)
            if (p->popu_fs[j].error < p->popu_fs[min].error)
                min = j;

        if (min != i)
        {
            // swapeo los atributos
            idtmp = p->popu_fs[i].id;
            p->popu_fs[i].id = p->popu_fs[min].id;
            p->popu_fs[min].id = idtmp;

            idtmp = p->popu_fs[i].oldid;
            p->popu_fs[i].oldid = p->popu_fs[min].oldid;
            p->popu_fs[min].oldid = idtmp;

            idtmp = p->popu_fs[i].executed;
            p->popu_fs[i].executed = p->popu_fs[min].executed;
            p->popu_fs[min].executed = idtmp;

            idtmp = p->popu_fs[i].generation;
            p->popu_fs[i].generation = p->popu_fs[min].generation;
            p->popu_fs[min].generation = idtmp;

            tmp = p->popu_fs[i].m1;
            p->popu_fs[i].m1 = p->popu_fs[min].m1;
            p->popu_fs[min].m1 = tmp;

            tmp = p->popu_fs[i].m10;
            p->popu_fs[i].m10 = p->popu_fs[min].m10;
            p->popu_fs[min].m10 = tmp;

            tmp = p->popu_fs[i].m100;
            p->popu_fs[i].m100 = p->popu_fs[min].m100;
            p->popu_fs[min].m100 = tmp;

            tmp = p->popu_fs[i].mherb;
            p->popu_fs[i].mherb = p->popu_fs[min].mherb;
            p->popu_fs[min].mherb = tmp;

            tmp = p->popu_fs[i].wndvel;
            p->popu_fs[i].wndvel = p->popu_fs[min].wndvel;
            p->popu_fs[min].wndvel = tmp;

            tmp = p->popu_fs[i].wnddir;
            p->popu_fs[i].wnddir = p->popu_fs[min].wnddir;
            p->popu_fs[min].wnddir = tmp;

            tmp = p->popu_fs[i].temp;
            p->popu_fs[i].temp = p->popu_fs[min].temp;
            p->popu_fs[min].temp = tmp;

            tmp = p->popu_fs[i].hum;
            p->popu_fs[i].hum = p->popu_fs[min].hum;
            p->popu_fs[min].hum = tmp;

            tmp = p->popu_fs[i].error;
            p->popu_fs[i].error = p->popu_fs[min].error;
            p->popu_fs[min].error = tmp;

            tmp = p->popu_fs[i].errorc;
            p->popu_fs[i].errorc = p->popu_fs[min].errorc;
            p->popu_fs[min].errorc = tmp;

            for(t=0; t<p->popu_fs[i].nparams_farsite; t++)
            {
                tmp = p->popu_fs[i].parameters[t];
                p->popu_fs[i].parameters[t]=p->popu_fs[min].parameters[t];
                p->popu_fs[min].parameters[t]=tmp;
            }
        } // if min != i
    }

    return 1;
}

// ordeno por error (busco un minimo ahora!)
int sortPopulationByErrorC(POPULATIONTYPE * p)
{
    int i, j, max, k;
    float tmp;

    for (i = 0; i < p->popuSize; i++)
    {
        // busco el i-esimo minimo

        max = i;
        for (j = i+1; j < p->popuSize; j++)
            if (p->popu[j].errorc > p->popu[max].errorc)
                max = j;

        // swapeo los individuos si cal
        if (max != i)
        {
            //swapeo los parametros
            for (k = 0; k < p->popu[i].n; k++)
            {
                tmp = p->popu[i].p[k];
                p->popu[i].p[k] = p->popu[max].p[k];
                p->popu[max].p[k] = tmp;
            }

            // swapeo los atributos
            tmp = p->popu[i].fit;               // fitness
            p->popu[i].fit = p->popu[max].fit;
            p->popu[max].fit = tmp;

            tmp = p->popu[i].dist;              // distancia
            p->popu[i].dist = p->popu[max].dist;
            p->popu[max].dist = tmp;

            tmp = p->popu[i].dir;               // direccion
            p->popu[i].dir = p->popu[max].dir;
            p->popu[max].dir = tmp;

            tmp = p->popu[i].vel;               // velocidad
            p->popu[i].vel = p->popu[max].vel;
            p->popu[max].vel = tmp;

            tmp = p->popu[i].error;             // error
            p->popu[i].error = p->popu[max].error;
            p->popu[max].error = tmp;

            tmp = p->popu[i].errorc;                // errorc
            p->popu[i].errorc = p->popu[max].errorc;
            p->popu[max].errorc = tmp;

            tmp = p->popu[i].wnddir;                // winddir
            p->popu[i].wnddir = p->popu[max].wnddir;
            p->popu[max].wnddir = tmp;

            tmp = p->popu[i].wndvel;                // windvel
            p->popu[i].wndvel = p->popu[max].wndvel;
            p->popu[max].wndvel = tmp;
        } // if max != i
    }

    return 1;
}



/**
 * Genera numeros Randon entre l y u
 *
 * @param l      Limite inferior
 * @param u      Limite Superior
 *
 * @return Valor Randon Generado
 */
double
randLim(double l, double u)
{
    return(drand48() * (u-l) + l);
}




/**
 * busca el indice del individuo de menor error de toda la poblacion
 *
 * @param p      poblacion
 *
 * @return indice de individuo de menor error
 */
int buscarIndividuoMinError(POPULATIONTYPE * p)
{
    int i, indMin;
    double min = 999999.;

    for (i=0; i<p->popuSize; i++)
        if (p->popu[i].error < min)
        {
            min = p->popu[i].error;
            indMin = i;
        }

    return indMin;

}


/**
 * almacena en un fichero el 1er individuo de una poblacion que
 * es el de menor error. Como la poblacion no esta ordenada,
 * primero busco el mejor individio (min error) y luego lo
 * escribo en el fichero fBest
 *
 * @param p      poblacion de la cual se quiere almacenar el mejor individuo
 * @param fBest  nombre del fichero donde se almacenara el mejor (se lee en datos.ini)
 *
 * @return todo ok o no
 */
int save_bestIndv(POPULATIONTYPE * p, char * fBest)
{

    FILE *mejores;
    int numIndv, numGene;
    INDVTYPE indv;
    int cantParams, x, numindv;


    numindv = buscarIndividuoMinError(p);
    get_indv(p, numindv, &indv);
    numIndv = p->popuSize;
    numGene = p->currentGen;
    printf("numGene:%d best:%d fbest:%s\n", numGene, numindv, fBest);


    // si es la primer generacion, abro el fichero como w para que borre lo que haya
    // si no es la primera agrego las lineas al contenido existente
    if (numGene == 0)
    {
        if (!(mejores = fopen(fBest, "w")))
        {
            printf("SAVE_BESTINDV:: no se pudo abrir el fichero %s \n", fBest);
            return -1;
        }
    }
    else
    {
        if (!(mejores = fopen(fBest, "a")))
        {
            printf("SAVE_BESTINDV:: no se pudo abrir el fichero %s \n", fBest);
            return -1;
        }
    }

    fprintf(mejores, "%d %d \n", numIndv, numGene);
    // imprimo el individuo completo
    cantParams = p->popu[numindv].n;
    fprintf(mejores, "%d ", cantParams);
    for (x = 0; x < cantParams; x ++)
        fprintf(mejores," %1.3f  ", p->popu[numindv].p[x]);
    fprintf(mejores, " %1.2f    %1.2f    %1.2f    %1.2f    %1.2f    %1.2f    %1.2f    %1.2f \n", p->popu[numindv].fit, p->popu[numindv].dist, p->popu[numindv].dir, p->popu[numindv].vel, p->popu[numindv].error, p->popu[numindv].errorc, p->popu[numindv].wnddir,  p->popu[numindv].wndvel);

    fclose(mejores);
    return 1;

}

int evolve_population_farsite (POPULATIONTYPE * p, int eli, double cross, double mut, char * range, char * bestind, char * newpob)
{
    // EVOLVE POPULATION
    printf("GENETIC_Init_Farsite: BEGIN\n");

    if(GENETIC_Init_Farsite(eli,cross,mut,range,bestind,1,0,0)<1)
    {
        printf("\nERROR Initializing Genetic Algorithm! Exiting...\n");
        return -1;
    }
    printf("GENETIC_Init_Farsite: END\n");
    print_population_farsite(*p);
    printf("GENETIC_Algorithm_Farsite: BEGIN\n");

    if(GENETIC_Algorithm_Farsite(&p, newpob)<1)
    {
        printf("\nERROR Running Genetic Algorithm! Exiting...\n");
        return -1;
    }

    printf("GENETIC_Algorithm_Farsite: END\n");

    return 0;
}

void indFarsiteToArray (INDVTYPE_FARSITE * pin1,float * p1)
{
    int i;
    /*
     p1[0] = pin1->m1;
      p1[1] = pin1->m10;
      p1[2] = pin1->m100;
      p1[3] = pin1->mherb;
      p1[4] = pin1->wndvel;
      p1[5] = pin1->wnddir;
      p1[6] = pin1->temp;
      p1[7] = pin1->hum;
     */
    //p1[8] = pin1.error;
    //p1[9] = pin1.errorc;
//printf("Converdison: to a :%d\n",pin1->nparams_farsite);
    for(i=0; i<pin1->nparams_farsite; i++)
    {
        p1[i] = pin1->parameters[i];
    }
}

void arrayToIndFarsite (float * p1, INDVTYPE_FARSITE * pin1)
{
    int i;
    /*
    pin1->m1 = p1[0];
    pin1->m10 = p1[1];
    pin1->m100 = p1[2];
    pin1->mherb = p1[3];
    pin1->wndvel = p1[4];
    pin1->wnddir = p1[5];
    pin1->temp = p1[6];
    pin1->hum = p1[7];
    */
    pin1->error = 0;
    pin1->errorc = 0;
//printf("Converdison: to i :%d\n",pin1->nparams_farsite);
    for(i=0; i<pin1->nparams_farsite; i++)
    {
        pin1->parameters[i] = p1[i];
    }


    //p1[8] = pin1.error;
    //p1[9] = pin1.errorc;
}
