#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "iniparser.h"
#include <sys/types.h>
#include "myutils.h"

char    *baseAtmFile;
char    *atmFileNew;
char    *atmFile;
char    *resolution;
char    *VGeneral;
char    *VGrid;
char    *windinit;
char    *vegetation;
char    *elevfilename2;
int     wn_num_theads;
char    *wn_path;
char *landscapeName;
char *landscapePath;

void initWindninjaVariables(char *datafile)
{
    dictionary * datos;
    datos 	= iniparser_load(datafile);

    landscapeName = iniparser_getstr(datos,"farsite:landscapeName");
    landscapePath = iniparser_getstr(datos,"farsite:landscapePath");
    elevfilename2   = iniparser_getstr(datos, "windninja:elevfilename");
    baseAtmFile    = iniparser_getstr(datos, "windninja:baseAtmFile");
    atmFile        = iniparser_getstr(datos, "windninja:atmFile");
    resolution     = iniparser_getstr(datos, "windninja:resolution");
    VGeneral       = iniparser_getstr(datos, "windninja:VGeneral");
    VGrid          = iniparser_getstr(datos, "windninja:VGrid");
    windinit       = iniparser_getstr(datos, "windninja:windinit");
    vegetation     = iniparser_getstr(datos, "windninja:vegetation");
    wn_num_theads  = iniparser_getint(datos, "windninja:wn_num_theads",1);
    wn_path        = iniparser_getstr(datos, "windninja:wn_path");
}

char * createATMFile(char *path_output, float vel, float dir, int id)
{
    char * line = (char*)malloc(sizeof(char) * 1000);
    char * newline;
    char * buffer= (char*)malloc(sizeof(char) * 1000);
    atmFileNew = (char*)malloc(sizeof(char) * 1000);
    char * atmFileFullPath = (char*)malloc(sizeof(char) * 1000);
    FILE * fATM, *fATMnew;
    char * tmp = (char*)malloc(sizeof(char) * 1000);
    sprintf(tmp,"%d",id);
    //printf("id=%s\n",tmp);
    //printf("atmFile=%s\n",atmFile);
    atmFileNew = str_replace(atmFile, "$1", tmp);
    //printf("atmFileNew=%s\n",atmFileNew);
    sprintf(atmFileFullPath, "%s%s", path_output, atmFileNew);

    printf("atmFileFullPath:%s\n",atmFileFullPath);
    char * elevOnlyName;
    elevOnlyName = strtok (landscapeName,".");
    if ( (fATM = fopen(baseAtmFile, "r")) == NULL)
    {
        printf("Unable to open ATM file");
        return "";
    }
    else
    {
        if((fATMnew = fopen(atmFileFullPath, "w")) == NULL)
        {
            printf("Unable to create ATM temp file");
            return "";
        }
        else
        {
            //printf("VALORES DE LAS HUMEDADES: m1:%1.3f m10:%1.3f m100:%1.3f mherb:%1.3f\n", individuo.m1, individuo.m10, individuo.m100, individuo.mherb);
            fgets( line, 100, fATM );
            fprintf(fATMnew,"%s", line);

            while(fgets( line, 100, fATM ) != NULL)
            {
                sprintf(buffer,"%s%s_%1.0f_%1.0f_%sm_vel.asc", path_output, elevOnlyName, dir, vel, resolution);
                newline = str_replace(line, "ws", buffer);
                sprintf(buffer,"%s%s_%1.0f_%1.0f_%sm_ang.asc", path_output, elevOnlyName, dir, vel, resolution);
                newline = str_replace(newline, "wd", buffer);
                sprintf(buffer,"%s%s_%1.0f_%1.0f_%sm_cld.asc", path_output, elevOnlyName, dir, vel, resolution);
                newline = str_replace(newline, "wc", buffer);
                fprintf(fATMnew,"%s", newline);
            }
            fclose(fATMnew);
        }
        fclose(fATM);
    }
    free(line);
    free(newline);
    free(buffer);

    return atmFileFullPath;
}

char * runWindNinja(char *path_output, float vel, float dir, int id, char * datos)
{
    initWindninjaVariables(datos);
    char sys_call[5000];
    char *atmPath;
    sprintf(sys_call,"%s --initialization_method %s --elevation_file %s%s --input_speed %1.0f --input_speed_units mph --input_direction %1.0f --uni_cloud_cover 0 --cloud_cover_units percent --mesh_resolution 100 --units_mesh_resolution m --write_ascii_output 1 --output_wind_height %s --units_output_wind_height m --vegetation %s --input_wind_height %s --units_input_wind_height m --ascii_out_resolution %s --units_ascii_out_resolution m --num_threads %d", wn_path, windinit, path_output,landscapeName, vel, dir, VGrid, vegetation, VGeneral, resolution, wn_num_theads);

    printf("LLAMADA WN: %s\n", sys_call);
    int err_syscall = system(sys_call);
    atmPath = createATMFile(path_output, vel, dir, id);
    return atmPath;
}
