#include <stdio.h>
#include <stdlib.h>
#include "population.h"

/*
 * - l: lower value
 * - u: upper value
 */
double randim(double l, double u) {
    return (drand48()* (u-l) + l);
}

/**
 * - argv[1] int: population size
 * - argv[2] file path: range file
 * - argv[3] file path: output file
 */
main(int argv, char * argc[]) {
    int populationSize, i, p;
    double d;
    FILE * f, * fRange;
    char * name, * nameRange;
    INDVTYPE min, max;

    if (argv != 4) {
        printf("ERROR: GenPopulation.main -> Use generate populationSize rangeIn fileOut\n");
        return -1;
    }

    populationSize = atoi(argc[1]);
    nameRange = argc[2];
    name = argc[3];

    srand48((unsigned)time(NULL));

    if ((f = fopen(name, "w")) == NULL) {
        printf("ERROR: GenPopulation.main -> Couldn't open the output file %s \n", name);
        return -2;
    }

    // leo los limites inferior y superior en min y max
    if ((fRange = fopen(nameRange, "r")) == NULL) {
        printf("ERROR: GenPopulation.main -> Couldn't open the range file %s \n", nameRange);
        return -3;
    }

    fscanf(fRange, "%d ", &min.n);

    for (i = 0; i < min.n; i++) {
        fscanf(fRange, "%f ", &min.p[i]);
    }

    fscanf(fRange, "%d ", &max.n);
    for (i=0; i<max.n; i++) {
        fscanf(fRange, "%f ", &max.p[i]);
    }

    printf("INFO: GenPopulation.main -> Successfully read range from %s %d \n", nameRange, max.n);
    fclose(fRange);

    // genero cada uno de los individuos de la poblacion
    fprintf(f, "%d 0 %d\n", populationSize, max.n);

    for (i = 0; i < populationSize; i++) {
        for(p = 0; p < min.n; p++) {
            fprintf(f, "%f ", randim(min.p[p], max.p[p]));
        }
        fprintf(f,"0 %d 0",i);
        fprintf(f,"\n");
    }

    printf("INFO: GenPopulation.main -> Population successfully Poblacion generada correctamente en \"%s\" \n", name);
    fclose(f);
    return 0;

}
