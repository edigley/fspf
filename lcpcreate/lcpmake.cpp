//---------------------------------------------------------------------------

#pragma hdrstop

#include "lcpgen4.h"
//#include <windows.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

char TLndFile[256]="";
char LndFile[256]="";
char ElFile[256]="";
char SlFile[256]="";
char AsFile[256]="";
char FlFile[256]="";
char CvFile[256]="";
char HtFile[256]="";
char HlcbFile[256]="";
char CbdFile[256]="";
char DuffFile[256]="";
char WoodyFile[256]="";
int32_t latitude=100;

int main(int argc, char *argv[])
{
    int32_t i;

    /*printf("numargs %d\n", argc);
    for(i=1; i<argc; i++)
    {    printf("%s %s\n", argv[i], argv[i+1]);
         i++;
    }
    */

    if(argc>14)
    {
        for(i=1; i<argc; i++)
        {
            if(!strncmp("-lan", argv[i], 4))
                sprintf(LndFile, "%s%s", argv[i+1], ".lcp");
            else if(!strncmp("-ele", argv[i], 4))
                sprintf(ElFile, "%s", argv[i+1]);
            else if(!strncmp("-slo", argv[i], 4))
                sprintf(SlFile, "%s", argv[i+1]);
            else if(!strncmp("-asp", argv[i], 4))
                sprintf(AsFile, "%s", argv[i+1]);
            else if(!strncmp("-fue", argv[i], 4))
                sprintf(FlFile, "%s", argv[i+1]);
            else if(!strncmp("-cov", argv[i], 4))
                sprintf(CvFile, "%s", argv[i+1]);
            else if(!strncmp("-hei", argv[i], 4))
                sprintf(HtFile, "%s", argv[i+1]);
            else if(!strncmp("-bas", argv[i], 4))
                sprintf(HlcbFile, "%s", argv[i+1]);
            else if(!strncmp("-den", argv[i], 4))
                sprintf(CbdFile, "%s", argv[i+1]);
            else if(!strncmp("-duf", argv[i], 4))
                sprintf(DuffFile, "%s", argv[i+1]);
            else if(!strncmp("-woo", argv[i], 4))
                sprintf(WoodyFile, "%s", argv[i+1]);
            else if(!strncmp("-lat", argv[i], 4))
                latitude=atol(argv[i+1]);
            i++;
        }

        if(strlen(LndFile)==0)
        {
            printf("\n\nError, NO Landscape File Specified:\n\n");
            exit(1);
        }
        //if(GetFileAttributes(ElFile)==0xFFFFFFFF)
        if(access(ElFile,F_OK)==-1)
        {
            printf("\n\nFILE READ ERROR:\n\n");
            printf("File Not Found %s\n", ElFile);
            exit(1);
        }
        else if(access(SlFile,F_OK)== -1)
        {
            printf("\n\nFILE READ ERROR:\n\n");
            printf("File Not Found %s\n", SlFile);
            exit(1);
        }
        else if(access(AsFile,F_OK)== -1)
        {
            printf("\n\nFILE READ ERROR:\n\n");
            printf("File Not Found %s\n", AsFile);
            exit(1);
        }
        else if(access(FlFile,F_OK)== -1)
        {
            printf("\n\nFILE READ ERROR:\n\n");
            printf("File Not Found %s\n", FlFile);
            exit(1);
        }
        else if(access(CvFile,F_OK)== -1)
        {
            printf("\n\nFILE READ ERROR:\n\n");
            printf("File Not Found %s\n", CvFile);
            exit(1);
        }
        else if(strlen(HtFile)>0 && access(HtFile,F_OK)== -1)
        {
            printf("\n\nFILE READ ERROR:\n\n");
            printf("File Not Found %s\n", HtFile);
            exit(1);
        }
        else if(strlen(HlcbFile)>0 && access(HlcbFile,F_OK)== -1)
        {
            printf("\n\nFILE READ ERROR:\n\n");
            printf("File Not Found %s\n", HlcbFile);
            exit(1);
        }
        else if(strlen(CbdFile)>0 && access(CbdFile,F_OK)== -1)
        {
            printf("\n\nFILE READ ERROR:\n\n");
            printf("File Not Found %s\n", CbdFile);
            exit(1);
        }
        else if(strlen(DuffFile)>0 && access(DuffFile,F_OK)== -1)
        {
            printf("\n\nFILE READ ERROR:\n\n");
            printf("File Not Found %s\n", DuffFile);
            exit(1);
        }
        else if(strlen(WoodyFile)>0 && access(WoodyFile,F_OK)== -1)
        {
            printf("\n\nFILE READ ERROR:\n\n");
            printf("File Not Found %s\n", WoodyFile);
            exit(1);
        }

        printf("-----------------------------------\n");
        printf("-----------------------------------\n\n");
        printf("LCPMake processing grid themes.....\n\n");
        printf("-----------------------------------\n");
        printf("-----------------------------------\n\n");

        if(!GenerateLCPFromArc())
        {
            printf("\n\n");
            printf("LCPMAKE.EXE Failed\n");
            printf("probable file read error\n");
            printf("check paths\n\n");
            getc(stdin);
        }
        else
        {
            printf("\n\n");
            printf("LCPMAKE.EXE Completed......\n\n");
            printf("Landscape File Generated:\n\n");
            printf(">>>>  %s\n\n\n", LndFile);
        }
        /*
        		if(GenerateLCPFromArc())
                  {	// example read from LCP file
                       // 3.5 cells from lower left corner
                       //
                       int32_t posit;
                       float xpt, ypt;
                       celldata Cell;

                  	SetLandFileName(LndFile);
                  	OpenLandFile();
                       ReadHeader();
                       xpt=GetLoEast()+GetCellResolutionX()*3.5;
                       ypt=GetLoNorth()+GetCellResolutionY()*3.5;
                       Cell=CellData(xpt, ypt, &posit);
                       printf("%s %d %s %lf %lf\n\n\n", "fuel model = ", (int) Cell.f,
                       	"at location ",
                            ConvertEastingOffsetToUtm(xpt),       		// convert local coords to utm
                            ConvertNorthingOffsetToUtm(ypt));			// convert local coords to utm
                  }
        */

    }
    else
    {
        printf("\n\n");
        printf(">> LCPMAKE.EXE makes a binary FARSITE .LCP file.\n\n");
        printf(">> It requires the names of at least 5 ARC GRID ASCII\n");
        printf(">> GIS input files, 1 LCP output file, and the latitude\n");
        printf(">> for the landscape (negative for Southern Hemisphere).\n");
        printf(">> All files must be coregistered and with identical \n");
        printf(">> resolution with the correct units.  Preceed each file\n");
        printf(">> with the identifying switch indicated below:\n");

        printf(">> Required Inputs include...\n\n");
        printf("  SWITCH         NAME (UNITS)\n");
        printf("  -latitude      latitude (-90 to 90) \n\n");
        printf("  -landscape     Landscape      output file)\n");
        printf("  -elevation     Elevation (m)  input file \n");
        printf("  -slope         Slope     (deg)  input file \n");
        printf("  -aspect        Aspect    (deg)  input file \n");
        printf("  -fuel          Fuel           input file \n");
        printf("  -cover         Cover     (%)  input file \n\n");

        printf(">> Optional GIS Themes include...\n\n");
        printf("  SWITCH         NAME (UNITS)\n");
        printf("  -height        Tree Height         (m)       input File\n");
        printf("  -base          Crown Base Height   (m)       input File\n");
        printf("  -density       Crown Bulk Density  (kg/m3)    input File\n\n");
        printf("  -duff          Duff Loading        (Mg/Ha)    input File\n");
        printf("  -woody         Woody Profile                  input File\n\n");
        printf("MAF\n");
        exit(0);
    }
    return 0;
}

