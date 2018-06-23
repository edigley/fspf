#include <stdio.h>
#include <stdlib.h>
#include "population.h"

double
randim(double l, double u)
{
    return (drand48()* (u-l) + l);
}


main(int argv, char * argc[])
{
    int cant, i, p, cantP;
    double d;
    FILE * f, * fRange;
    char * name, * nameRange;
    INDVTYPE min, max;

    if (argv != 4)
    {
        printf("Use generar populationSize rangeIn fileOut\n");
        return -1;
    }

    srand48((unsigned)time(NULL));
    name = argc[3];
    nameRange = argc[2];
    cant = atoi(argc[1]);

    if ((f = fopen(name, "w")) == NULL)
    {
        printf("No se pudo abrir el fichero %s \n", name);
        return -1;
    }

// leo los limites inferior y superior en min y max
    if ((fRange = fopen(nameRange, "r")) == NULL)
    {
        printf("No se pudo abrir el fichero de rangos  %s \n", nameRange);
        return -1;
    }

    fscanf(fRange, "%d ", &min.n);

    for (i = 0; i < min.n; i++)
        fscanf(fRange, "%f ", &min.p[i]);

    fscanf(fRange, "%d ", &max.n);
    for (i=0; i<max.n; i++)
        fscanf(fRange, "%f ", &max.p[i]);

    printf("Rangos leidos correctamente desde %s %d \n", nameRange, max.n);
    fclose(fRange);

    // genero cada uno de los individuos de la poblacion
    fprintf(f, "%d 0 %d\n", cant, max.n);

    for (i = 0; i < cant; i++)
    {
        for(p = 0; p < min.n; p++)
            fprintf(f, "%f ", randim(min.p[p], max.p[p]));
        fprintf(f,"0 %d 0",i);
        fprintf(f,"\n");
    }

    printf("Poblacion generada correctamente en \"%s\" \n", name);
    fclose(f);
    return 0;

}
