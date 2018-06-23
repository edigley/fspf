#include <stdlib.h>
#include <stdio.h>
//#include <mem.h>
//#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <math.h>


typedef int BOOL;// int;


typedef struct
{// header for landscape file
	int32_t CrownFuels;//4         // 20 if no crown fuels, 21 if crown fuels exist
     int32_t GroundFuels;//4		// 20 if no ground fuels, 21 if ground fuels exist
	int32_t latitude;//4
	double loeast;//8
	double hieast;//8
	double lonorth;//8
	double hinorth;//8
	int32_t loelev;//4
	int32_t hielev;//4
	int32_t numelev; //4			//-1 if more than 100 categories
     int32_t elevs[100];//400
     int32_t loslope;//4
     int32_t hislope;//4
     int32_t numslope;//4			//-1 if more than 100 categories
     int32_t slopes[100];//400
     int32_t loaspect;//4
     int32_t hiaspect;//4
     int32_t numaspect;	//4	//-1 if more than 100 categories
     int32_t aspects[100];//400
     int32_t lofuel;//4
     int32_t hifuel;//4
     int32_t numfuel;	//4		//-1 if more than 100 categories
     int32_t fuels[100];//400
     int32_t locover;//4
     int32_t hicover;//4
     int32_t numcover;//4			//-1 if more than 100 categories
     int32_t covers[100];//400
     int32_t loheight;//4
     int32_t hiheight;//4
     int32_t numheight;	//4	//-1 if more than 100 categories
     int32_t heights[100];//400
     int32_t lobase;//4
     int32_t hibase;//4
     int32_t numbase;	//4		//-1 if more than 100 categories
     int32_t bases[100];//400
     int32_t lodensity;//4
     int32_t hidensity;//4
     int32_t numdensity;//4		//-1 if more than 100 categories
     int32_t densities[100];//400
     int32_t loduff;//4
     int32_t hiduff;//4
     int32_t numduff;	//4		//-1 if more than 100 categories
     int32_t duffs[100];//400
     int32_t lowoody;//4
     int32_t hiwoody;//4
     int32_t numwoody;	//4		//-1 if more than 100 categories
     int32_t woodies[100];//400
	int32_t numeast;//4
	int32_t numnorth;//4
	double EastUtm;//8
	double WestUtm;//8
	double NorthUtm;//8
	double SouthUtm;//8
	int32_t GridUnits; //4       // 0 for metric, 1 for English
	double XResol;//8
	double YResol;//8
	int16_t EUnits;//2
	int16_t SUnits;//2
	int16_t AUnits;//2
	int16_t FOptions;//2
	int16_t CUnits;//2
	int16_t HUnits;//2
	int16_t BUnits;//2
	int16_t PUnits;//2
     int16_t DUnits;//2
     int16_t WOptions;//2
     char ElevFile[256];//256
     char SlopeFile[256];//256
     char AspectFile[256];//256
     char FuelFile[256];//256
     char CoverFile[256];//256
     char HeightFile[256];//256
     char BaseFile[256];//256
     char DensityFile[256];//256
     char DuffFile[256];//256
     char WoodyFile[256];//256
     char Description[512];//512
}__attribute__((aligned(4),packed)) headdata;


typedef struct
{// structure for holding basic cell information
	int16_t e;                 // elevation
	int16_t s;                 // slope
	int16_t a;                 // aspect
	int16_t f;                 // fuel models
	int16_t c;                 // canopy cover
}celldata;


typedef struct
{// structure for holding optional crown fuel information
	int16_t h;				// canopy height
	int16_t b;				// crown base
	int16_t p;				// bulk density
}crowndata;


typedef struct
{
     int16_t d;				// duff model
      int16_t w;				// coarse woody model
}grounddata;


FILE * 	GetLandFile();
BOOL 	OpenLandFile();
void 	CloseLandFile();
void 	SetLandFileName(char* FileName);
char *	GetLandFileName();

double 	GetWestUtm();
double 	GetEastUtm();
double 	GetSouthUtm();
double 	GetNorthUtm();
double  	GetLoEast();
double 	GetHiEast();
double 	GetLoNorth();
double	GetHiNorth();
int32_t 	GetLoElev();
int32_t 	GetHiElev();
int32_t 	GetNumEast();
int32_t 	GetNumNorth();
double 	GetCellResolutionX();                      	// return landscape X cell dimension
double 	GetCellResolutionY();                      	// return landscape Y cell dimension
double 	ConvertEastingOffsetToUtm(double input);
double 	ConvertNorthingOffsetToUtm(double input);
double 	ConvertUtmToEastingOffset(double input);
double 	ConvertUtmToNorthingOffset(double input);

void 	ReadHeader();
int32_t 	HaveCrownFuels();
int32_t      HaveGroundFuels();
size_t 	GetHeadSize();
//celldata 	CellData(double east, double north, int32_t *posit);
int32_t		GetCellPosition(double east, double north);
void 	SetFilePosition(int32_t position);

bool 	GenerateLCPFromArc();
void      FinalizeHeader(headdata *NewHeader);
void      FillCats(celldata *cell, crowndata *cfuel, grounddata *gfuel);
void      SortCats();
