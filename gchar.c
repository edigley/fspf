#include "genetic.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


int submitPopulation(POPULATIONTYPE p, int pob, int gen);

int main (int argc, char** argv)
{
    int num_individuals = 10;
    int i,n,srows,scols,rrows=0,rcols=0,mode=-1;
    double *mapaReal, *mapaSim, fitness, error;
    float t1,t2;
    POPULATIONTYPE p;
    char cmd[300],tmp[32];
    FILE *fd;


    mode=atoi(argv[1]);

// EVOLVE POPULATION

    //GENETIC_Init(int eli, double crossP, double mutaP, char * fRange, char * fBests, int guidedMutation, int guidedEllitism, int doCompu)
    if(GENETIC_Init(5,0.2,0.1,"range.txt","bests.pob",1,0,0)<1)
    {
        printf("\nERROR Initializing Genetic Algorithm! Exiting...\n");
        return -1;
    }

    init_population(&p, num_individuals);
    get_population_farsite(&p, argv[2]);

    if(GENETIC_Algorithm(&p, argv[3])<1)
    {
        printf("\nERROR Running Genetic Algorithm! Exiting...\n");
        return -1;
    }

    return 0;

}



int submitPopulation(POPULATIONTYPE p, int pob, int gen)
{

    int i;
    char command[300];

    for(i=0; i<p.popuSize; i++)
    {

        //if(submitPopulation(p.popuSize, p.popu[i].p[0], p.popu[i].p[1], p.popu[i].p[2], p.popu[i].p[3], p.popu[i].p[4], p.popu[i].p[5], atoi(argv[3]), atoi(argv[4]))<0){
        //sprintf(command,"qsub -q cluster.q@@newnodes /home/andres/genetic_pruebas/genetic_characterization/test_synthetic/heterogeneus/hetsyntest.sub %f %f %f %f %f %f %d %d %d", p.popu[i].p[0], p.popu[i].p[1], p.popu[i].p[2], p.popu[i].p[3], p.popu[i].p[4], p.popu[i].p[5], pob, gen, i);

        sprintf(command,"qsub /home/cbrun/GENETICO/exp2/hetsyntest.sub %f %f %f %f %f %f %d %d %d", p.popu[i].p[0], p.popu[i].p[1], p.popu[i].p[2], p.popu[i].p[3],
                p.popu[i].p[4], p.popu[i].p[5], pob, gen, i);

        if (system(command)<0)
        {
            printf("\nERROR al ejecutar comando: %s\n",command);
            return -1;
        }
    }

    return 0;

}
