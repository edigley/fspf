#include "lcpgen4.h"

//------------------------------------------------------------------------------
// global data
//------------------------------------------------------------------------------

#define FALSE 0
#define TRUE 1

static int32_t OldFilePosition=0;
static BOOL NEED_CUST_MODELS=FALSE;	// custom fuel models
static BOOL HAVE_CUST_MODELS=FALSE;
static BOOL NEED_CONV_MODELS=FALSE;     // fuel model conversions
static BOOL HAVE_CONV_MODELS=FALSE;
static headdata Header;
static celldata cell;
static crowndata cfuel;
static FILE *landfile=0;
static char LandFName[256];              // landscape file name
static double RasterCellResolutionX;
static double RasterCellResolutionY;


extern char ElFile[256];
extern char SlFile[256];
extern char AsFile[256];
extern char FlFile[256];
extern char CvFile[256];
extern char HtFile[256];
extern char HlcbFile[256];
extern char CbdFile[256];
extern char LndFile[256];
extern char DuffFile[256];
extern char WoodyFile[256];
extern int32_t latitude;

static int32_t 	NumAllCats[10];
static int32_t 	AllCats[10][100];
static int32_t    NumVals=5;
static double maxval[10], minval[10];

#define NODATA -9999


//------------------------------------------------------------------------------
// global landscape data access functions
//------------------------------------------------------------------------------


int32_t GetNumEast()
{
    return Header.numeast;
}


int32_t GetNumNorth()
{
    return Header.numnorth;
}


double GetWestUtm()
{
    return Header.WestUtm;
}

double GetEastUtm()
{
    return Header.EastUtm;
}

double GetSouthUtm()
{
    return Header.SouthUtm;
}

double GetNorthUtm()
{
    return Header.NorthUtm;
}

double GetLoEast()
{
    return Header.loeast;
}
double GetHiEast()
{
    return Header.hieast;
}

double GetLoNorth()
{
    return Header.lonorth;
}

double GetHiNorth()
{
    return Header.hinorth;
}


double GetCellResolutionX()
{
    return Header.XResol;
}


double GetCellResolutionY()
{
    return Header.YResol;
}


double ConvertEastingOffsetToUtm(double input)
{
    return input+((int32_t)(Header.WestUtm/1000))*1000;
}

double ConvertNorthingOffsetToUtm(double input)
{
    return input+((int32_t)(Header.SouthUtm/1000))*1000;
}

double ConvertUtmToEastingOffset(double input)
{
    return input-((int32_t)(Header.WestUtm/1000))*1000;
}

double ConvertUtmToNorthingOffset(double input)
{
    return input-((int32_t)(Header.SouthUtm/1000))*1000;
}


char *GetLandFileName()
{
    return LandFName;
}

void SetLandFileName(char* FileName)
{
    memset(LandFName, 0x0, sizeof LandFName);
    sprintf(LandFName, "%s", FileName);
}


BOOL OpenLandFile()
{
    if(landfile)
        fclose(landfile);
    if((landfile=fopen(LandFName, "rb"))!=NULL)
        return TRUE;
    landfile=0;

    return FALSE;
}


void CloseLandFile()
{
    if(landfile)
        fclose(landfile);
    memset(LandFName, 0x0, sizeof LandFName);
    landfile=0;

    //canopy characteristics stuff
    /*
    CanopyChx.Height=CanopyChx.DefaultHeight;
    CanopyChx.CrownBase=CanopyChx.DefaultBase;
    CanopyChx.BulkDensity=CanopyChx.DefaultDensity;
        */
}


int32_t HaveCrownFuels()
{
    // 1 if have them, 0 if not
    return Header.CrownFuels-20;		// subtract 20 to ID file as version 4.x
}


int32_t HaveGroundFuels()
{
    return Header.GroundFuels-20;      // subtract 20 to ID file as version 4.x
}


void ReadHeader()
{
    double ViewPortNorth, ViewPortEast, ViewPortSouth, ViewPortWest;
    int32_t NumViewNorth, NumViewEast;

    //fseek(landfile, 0, SEEK_SET);

    uint32_t sizeh=sizeof(headdata);
    fread(&Header, sizeh, 1, landfile);
    fseek(landfile, 0, SEEK_SET);

    fread(&Header.CrownFuels, sizeof(int32_t), 1, landfile);
    fread(&Header.GroundFuels, sizeof(int32_t), 1, landfile);
    fread(&Header.latitude, sizeof(int32_t), 1, landfile);
    fread(&Header.loeast, sizeof(double), 1, landfile);
    fread(&Header.hieast, sizeof(double), 1, landfile);
    fread(&Header.lonorth, sizeof(double), 1, landfile);
    fread(&Header.hinorth, sizeof(double), 1, landfile);
    fread(&Header.loelev, sizeof(int32_t), 1, landfile);
    fread(&Header.hielev, sizeof(int32_t), 1, landfile);
    fread(&Header.numelev, sizeof(int32_t), 1, landfile);
    fread(Header.elevs, sizeof(int32_t), 100, landfile);
    fread(&Header.loslope, sizeof(int32_t), 1, landfile);
    fread(&Header.hislope, sizeof(int32_t), 1, landfile);
    fread(&Header.numslope, sizeof(int32_t), 1, landfile);
    fread(Header.slopes, sizeof(int32_t), 100, landfile);
    fread(&Header.loaspect, sizeof(int32_t), 1, landfile);
    fread(&Header.hiaspect, sizeof(int32_t), 1, landfile);
    fread(&Header.numaspect, sizeof(int32_t), 1, landfile);
    fread(Header.aspects, sizeof(int32_t), 100, landfile);
    fread(&Header.lofuel, sizeof(int32_t), 1, landfile);
    fread(&Header.hifuel, sizeof(int32_t), 1, landfile);
    fread(&Header.numfuel, sizeof(int32_t), 1, landfile);
    fread(Header.fuels, sizeof(int32_t), 100, landfile);
    fread(&Header.locover, sizeof(int32_t), 1, landfile);
    fread(&Header.hicover, sizeof(int32_t), 1, landfile);
    fread(&Header.numcover, sizeof(int32_t), 1, landfile);
    fread(Header.covers, sizeof(int32_t), 100, landfile);
    fread(&Header.loheight, sizeof(int32_t), 1, landfile);
    fread(&Header.hiheight, sizeof(int32_t), 1, landfile);
    fread(&Header.numheight, sizeof(int32_t), 1, landfile);
    fread(Header.heights, sizeof(int32_t), 100, landfile);
    fread(&Header.lobase, sizeof(int32_t), 1, landfile);
    fread(&Header.hibase, sizeof(int32_t), 1, landfile);
    fread(&Header.numbase, sizeof(int32_t), 1, landfile);
    fread(Header.bases, sizeof(int32_t), 100, landfile);
    fread(&Header.lodensity, sizeof(int32_t), 1, landfile);
    fread(&Header.hidensity, sizeof(int32_t), 1, landfile);
    fread(&Header.numdensity, sizeof(int32_t), 1, landfile);
    fread(Header.densities, sizeof(int32_t), 100, landfile);
    fread(&Header.loduff, sizeof(int32_t), 1, landfile);
    fread(&Header.hiduff, sizeof(int32_t), 1, landfile);
    fread(&Header.numduff, sizeof(int32_t), 1, landfile);
    fread(Header.duffs, sizeof(int32_t), 100, landfile);
    fread(&Header.lowoody, sizeof(int32_t), 1, landfile);
    fread(&Header.hiwoody, sizeof(int32_t), 1, landfile);
    fread(&Header.numwoody, sizeof(int32_t), 1, landfile);
    fread(Header.woodies, sizeof(int32_t), 100, landfile);
    fread(&Header.numeast, sizeof(int32_t), 1, landfile);
    fread(&Header.numnorth, sizeof(int32_t), 1, landfile);
    fread(&Header.EastUtm, sizeof(double), 1, landfile);
    fread(&Header.WestUtm, sizeof(double), 1, landfile);
    fread(&Header.NorthUtm, sizeof(double), 1, landfile);
    fread(&Header.SouthUtm, sizeof(double), 1, landfile);
    fread(&Header.GridUnits, sizeof(int32_t), 1, landfile);
    fread(&Header.XResol, sizeof(double), 1, landfile);
    fread(&Header.YResol, sizeof(double), 1, landfile);
    fread(&Header.EUnits, sizeof(int16_t), 1, landfile);
    fread(&Header.SUnits, sizeof(int16_t), 1, landfile);
    fread(&Header.AUnits, sizeof(int16_t), 1, landfile);
    fread(&Header.FOptions, sizeof(int16_t), 1, landfile);
    fread(&Header.CUnits, sizeof(int16_t), 1, landfile);
    fread(&Header.HUnits, sizeof(int16_t), 1, landfile);
    fread(&Header.BUnits, sizeof(int16_t), 1, landfile);
    fread(&Header.PUnits, sizeof(int16_t), 1, landfile);
    fread(&Header.DUnits, sizeof(int16_t), 1, landfile);
    fread(&Header.WOptions, sizeof(int16_t), 1, landfile);
    fread(Header.ElevFile, sizeof(char), 256, landfile);
    fread(Header.SlopeFile, sizeof(char), 256, landfile);
    fread(Header.AspectFile, sizeof(char), 256, landfile);
    fread(Header.FuelFile, sizeof(char), 256, landfile);
    fread(Header.CoverFile, sizeof(char), 256, landfile);
    fread(Header.HeightFile, sizeof(char), 256, landfile);
    fread(Header.BaseFile, sizeof(char), 256, landfile);
    fread(Header.DensityFile, sizeof(char), 256, landfile);
    fread(Header.DuffFile, sizeof(char), 256, landfile);
    fread(Header.WoodyFile, sizeof(char), 256, landfile);
    fread(Header.Description, sizeof(char), 512, landfile);
    // do this in case a version 1.0 file has gotten through
    Header.loeast=ConvertUtmToEastingOffset(Header.WestUtm);
    Header.hieast=ConvertUtmToEastingOffset(Header.EastUtm);
    Header.lonorth=ConvertUtmToNorthingOffset(Header.SouthUtm);
    Header.hinorth=ConvertUtmToNorthingOffset(Header.NorthUtm);

    if(Header.FOptions==1 || Header.FOptions==3)
        NEED_CUST_MODELS=true;
    else
        NEED_CUST_MODELS=false;
    if(Header.FOptions==2 || Header.FOptions==3)
        NEED_CONV_MODELS=true;
    else
        NEED_CONV_MODELS=false;
    //HAVE_CUST_MODELS=false;
    //HAVE_CONV_MODELS=false;
    // set raster resolution
    RasterCellResolutionX=(Header.EastUtm-Header.WestUtm)/(double) Header.numeast;
    RasterCellResolutionY=(Header.NorthUtm-Header.SouthUtm)/(double) Header.numnorth;
    ViewPortNorth=RasterCellResolutionY*(double) Header.numnorth+Header.lonorth;
    ViewPortSouth=Header.lonorth;
    ViewPortEast=RasterCellResolutionX*(double) Header.numeast+Header.loeast;
    ViewPortWest=Header.loeast;
//	NumViewNorth=(ViewPortNorth-ViewPortSouth)/Header.YResol;
//	NumViewEast=(ViewPortEast-ViewPortWest)/Header.XResol;
    double rows, cols;
    rows=(ViewPortNorth-ViewPortSouth)/Header.YResol;
    NumViewNorth=rows;
    if(modf(rows, &rows)>0.5)
        NumViewNorth++;
    cols=(ViewPortEast-ViewPortWest)/Header.XResol;
    NumViewEast=cols;
    if(modf(cols, &cols)>0.5)
        NumViewEast++;

    if(HaveCrownFuels())
    {
        if(HaveGroundFuels())
            NumVals=10;
        else
            NumVals=8;
    }
    else
    {
        if(HaveGroundFuels())
            NumVals=7;
        else
            NumVals=5;
    }

    /*
    CantAllocLCP=false;

    if(lcptheme)
    {	delete lcptheme;
         lcptheme=0;
    }
    lcptheme= new LandscapeTheme(false);

        if(landscape==0)
        {    if(CantAllocLCP==false)
        	{	int32_t i;

        		fseek(landfile, headsize, SEEK_SET);
    //     		if((landscape=(int16_t *) calloc(Header.numnorth*Header.numeast, NumVals*sizeof(int16_t)))!=NULL)
    		if((landscape=new int16_t[Header.numnorth*Header.numeast*NumVals])!=NULL)
             	{    ZeroMemory(landscape, Header.numnorth*Header.numeast*NumVals*sizeof(int16_t));
             		for(i=0; i<Header.numnorth; i++)
             			fread(&landscape[i*NumVals*Header.numeast], sizeof(int16_t),
                       		NumVals*Header.numeast, landfile);
             		fseek(landfile, headsize, SEEK_SET);
    //     	     	OldFilePosition=0;     // thread local
                  	CantAllocLCP=false;
             	}
             	else
             		CantAllocLCP=true;
             }
        }
        */
//	int32_t p;
//   CellData(Header.loeast, Header.hinorth, &p);
}


//------------------------------------------------------------------------------
// retrieve data from a grid cell given (east, north) coordinates
// 	uses relative offsets to locate file position
//------------------------------------------------------------------------------

/*
celldata CellData(double east, double north, celldata &cell, crowndata &cfuel, grounddata &gfuel, int32_t *posit)
{
     int32_t Position;

     if(landscape==0)
     {    if(CantAllocLCP==false)
     	{	int32_t i;

     		fseek(landfile, headsize, SEEK_SET);
//     		if((landscape=(int16_t *) calloc(Header.numnorth*Header.numeast, NumVals*sizeof(int16_t)))!=NULL)
			if((landscape=new int16_t[Header.numnorth*Header.numeast*NumVals])!=NULL)
          	{    ZeroMemory(landscape, Header.numnorth*Header.numeast*NumVals*sizeof(int16_t));
			    	for(i=0; i<Header.numnorth; i++)
          			fread(&landscape[i*NumVals*Header.numeast], sizeof(int16_t),
                    		NumVals*Header.numeast, landfile);
          		fseek(landfile, headsize, SEEK_SET);
//     	     	OldFilePosition=0;     // thread local
               	CantAllocLCP=false;
          	}
          	else
          		CantAllocLCP=true;
          }
     }

     Position=GetCellPosition(east, north);
	if(!CantAllocLCP)
     {    GetCellDataFromMemory(Position, cell, cfuel, gfuel);

          return cell;
     }

	if(Header.CrownFuels==20)
	{    if(Header.GroundFuels==20)
     	{   	fseek(landfile, (Position-OldFilePosition)*sizeof(celldata), SEEK_CUR);
			fread(&cell, sizeof(celldata), 1, landfile);
          }
          else
     	{   	fseek(landfile, (Position-OldFilePosition)*(sizeof(celldata)+sizeof(grounddata)), SEEK_CUR);
			fread(&cell, sizeof(celldata), 1, landfile);
               fread(&gfuel, sizeof(grounddata), 1, landfile);
          }
	}
	else
	{    if(Header.GroundFuels==20)		// none
	     {	fseek(landfile, (Position-OldFilePosition)*(sizeof(celldata)+sizeof(crowndata)), SEEK_CUR);
			fread(&cell, sizeof(celldata), 1, landfile);
			fread(&cfuel, sizeof(crowndata), 1, landfile);
     	}
     	else
	     {	fseek(landfile, (Position-OldFilePosition)*(sizeof(celldata)+sizeof(crowndata)+
               	sizeof(grounddata)), SEEK_CUR);
			fread(&cell, sizeof(celldata), 1, landfile);
			fread(&cfuel, sizeof(crowndata), 1, landfile);
               fread(&gfuel, sizeof(grounddata), 1, landfile);
     	}
		if(cfuel.h>0)
		{	CanopyChx.Height=(double) cfuel.h/10.0;
			if(Header.HUnits==2)
				CanopyChx.Height/=3.280839;
		}
		else
			CanopyChx.Height=CanopyChx.DefaultHeight;
		if(cfuel.b>0)
		{	CanopyChx.CrownBase=(double) cfuel.b/10.0;
			if(Header.BUnits==2)
				CanopyChx.CrownBase/=3.280839;
		}
		else
			CanopyChx.CrownBase=CanopyChx.DefaultBase;
		if(cfuel.p>0)
		{    if(Header.PUnits==1)
				CanopyChx.BulkDensity=((double) cfuel.p)/100.0;
			else	if(Header.PUnits==2)
				CanopyChx.BulkDensity=((double) cfuel.p/1000.0)*16.01845;
		}
		else
			CanopyChx.BulkDensity=CanopyChx.DefaultDensity;
	}

	OldFilePosition=Position+1;
     if(posit!=NULL)
     	*posit=Position;

	return cell;
}
*/


//------------------------------------------------------------------------------
// do LCP file positioning
//------------------------------------------------------------------------------


int32_t GetCellPosition(double east, double north)
{
    double xpt=(east-Header.loeast)/GetCellResolutionX();
    double ypt=(north-Header.lonorth)/GetCellResolutionY();
    int32_t easti=((int32_t) xpt);
    int32_t northi=((int32_t) ypt);
    northi=Header.numnorth-northi-1;
    if(northi<0)
        northi=0;
    int32_t posit=(northi*Header.numeast+easti);

    return posit;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
//	Generate the LCP file
//
//		Note: that the units in this file are assumed, these must be determined
//		elsewhere from user input
//
//------------------------------------------------------------------------------


bool GenerateLCPFromArc()
{
    celldata NewCell;
    headdata NewHeader;
    memset(&NewHeader, 0x0, sizeof(headdata));
    crowndata NewCrown;
    grounddata NewGround;
    NewHeader.CrownFuels=0;
    NewHeader.GroundFuels=0;
    NewHeader.HUnits=0;
    NewHeader.BUnits=0;
    NewHeader.PUnits=0;
    NewHeader.DUnits=0;
    NewHeader.WOptions=0;
    int32_t i, j;
    char DataType[256];
    int32_t CNRows, CNCols;
    double CXLCorner, CYLCorner, CCellSize;
    double KmToMeters=1.0;
    double ipart;
    double HeightMult, BaseMult, DensityMult, DuffMult;
    FILE *EFile, *SFile, *AFile, *FFile, *CFile, *LFile;
    FILE *HFile=0, *BFile=0, *PFile=0, *DFile=0, *WFile=0;

    NewHeader.latitude=latitude;
    NewHeader.CrownFuels=0;	// 1 have crown fuels, 0 if not
    NewHeader.GroundFuels=0;	// 1 have ground fuels, 0 if not

    NewHeader.latitude=latitude;	// set latitude degrees (negative for souther hemisphere)

    // DISTANCE UNITS
    NewHeader.GridUnits=0;	// 0 meters, 1 feet

    //FUEL OPTIONS
    NewHeader.FOptions=0;    // 0 no custom models AND no conversion file
    // 1 custom models BUT no conversion file
    // 2 no cust models BUT conversion file
    // 3 cust models AND conversion file needed

    //ELEVATION UNITS
    NewHeader.EUnits=0;		// 0 metric, 1 english elevation

    //SLOPE UNITS
    NewHeader.SUnits=0;      // 0 degrees, 1 percent

    //ASPECT UNITS
    NewHeader.AUnits=2;      // 0 grass cats, 1 grass deg, 2 azimuth deg

    //COVER UNITS
    NewHeader.CUnits=1;		// 0 cats(0-4), 1 percentages

    // Crown Fuel Units get set in the section after each file is validated
    /*					//TREE HEIGHT UNITS
    NewHeader.HUnits=0;      // 1 if metric (m), 2 if english (ft)

        					//CROWN BASE UNITS
    NewHeader.BUnits=0;      // 1 if metric (m), 2 if english (ft)

        					//CROWN BULKDENSITY UNITS
    NewHeader.PUnits=0;      // 1 if metric (kg/m3), 2 if english (lb/ft3)
        */

    //-------------------------------------------------------------------------
    // take resolution and coordinate info from
    // elevation file only
    //-------------------------------------------------------------------------
    EFile=fopen(ElFile, "r");
    rewind(EFile);
    fscanf(EFile, "%s %ld", DataType, &CNCols);
    fscanf(EFile, "%s %ld", DataType, &CNRows);
    fscanf(EFile, "%s %lf", DataType, &CXLCorner);
    fscanf(EFile, "%s %lf", DataType, &CYLCorner);
    fscanf(EFile, "%s %lf", DataType, &CCellSize);
    fscanf(EFile, "%s %s", DataType, DataType);

    SFile=fopen(SlFile, "r");
    rewind(SFile);
    AFile=fopen(AsFile, "r");
    rewind(AFile);
    for(i=0; i<6; i++)    // get to proper file position
    {
        fscanf(SFile, "%s %s", DataType, DataType);
        fscanf(AFile, "%s %s", DataType, DataType);
    }
    FFile=fopen(FlFile, "r");
    rewind(FFile);
    for(i=0; i<6; i++)    // get to proper file position
        fscanf(FFile, "%s %s", DataType, DataType);
    CFile=fopen(CvFile, "r");
    rewind(CFile);
    for(i=0; i<6; i++)    // get to proper file position
        fscanf(CFile, "%s %s", DataType, DataType);
    LFile=fopen(LndFile, "wb");
    if(strlen(HtFile)>0)
    {
        NewHeader.HUnits=1;// // 1 if metric (m), 2 if english (ft), 3 if metric*10, 4 if english*10
        HeightMult=10.0;      // 10.0 if native units
        HFile=fopen(HtFile, "r");
        rewind(HFile);
        for(i=0; i<6; i++)
            fscanf(HFile, "%s %s", DataType, DataType);
        NewHeader.CrownFuels=1;
    }
    else
        NewHeader.HUnits=0;// // 1 if metric (m), 2 if english (ft), 3 if metric*10, 4 if english*10
    if(strlen(HlcbFile)>0)
    {
        NewHeader.BUnits=1;// // 1 if metric (m), 2 if english (ft)
        BaseMult=10.0;          // 10.0 if native units
        BFile=fopen(HlcbFile, "r");
        rewind(BFile);
        for(i=0; i<6; i++)
            fscanf(BFile, "%s %s", DataType, DataType);
        NewHeader.CrownFuels=1;
    }
    else
        NewHeader.BUnits=0;//
    if(strlen(CbdFile)>0)
    {
        NewHeader.PUnits=1;//    // 1 if metric (m), 2 if english (ft), 3 if m*100, 4 if eng*1000
        DensityMult=100.0;       // 100.0 if in native units metric, 1000.0 if native units english
        PFile=fopen(CbdFile, "r");
        rewind(PFile);
        for(i=0; i<6; i++)
            fscanf(PFile, "%s %s", DataType, DataType);
        NewHeader.CrownFuels=1;
    }
    else
        NewHeader.PUnits=0;// // 1 if metric (m), 2 if english (ft)

    if(strlen(DuffFile)>0)
    {
        NewHeader.DUnits=2;
        DuffMult=10.0;	          // 10xMg/ha
        //NewHeader.DUnits=1;
        //DuffMult=10.0;	          // 10x t/ac

        DFile=fopen(DuffFile, "r");
        for(i=0; i<6; i++)
            fscanf(DFile, "%s %s", DataType, DataType);
        NewHeader.GroundFuels=1;
    }
    else
        NewHeader.DUnits=0;

    if(strlen(WoodyFile)>0)
    {
        NewHeader.WOptions=1;
        WFile=fopen(WoodyFile, "r");
        for(i=0; i<6; i++)
            fscanf(WFile, "%s %s", DataType, DataType);
        NewHeader.GroundFuels=1;
    }
    else
        NewHeader.WOptions=0;


    /*
    LFile=fopen(LndFile, "wb");
        if(LFile==NULL)
        {    SetFileAttributes(LndFile, FILE_ATTRIBUTE_NORMAL);
            	LFile=fopen(LndFile, "wb");
        }
    if(HT->GetCheck())
    {    BarValue=HeightUnits->GetSelIndex();
    	switch(BarValue)
    	{	case 0: 	NewHeader.HUnits=1;
    				HeightMult=10.0; break;
    		case 1:   NewHeader.HUnits=3;
    				HeightMult=1.0; break;
    		case 2:   NewHeader.HUnits=2;
    				HeightMult=10.0; break;
    		case 3:   NewHeader.HUnits=4;
    				HeightMult=1.0; break;
    	}
    	HTFile=fopen(HtFile, "r"); rewind(HTFile);
    	for(i=0; i<6; i++)
    		fscanf(HTFile, "%s %s", DataType, DataType);
    	NewHeader.CrownFuels=1;
    }
    else if(HConst->GetCheck())
    {	NewHeader.HUnits=5;
    	NewHeader.CrownFuels=1;
             HeightUnits->SetSelIndex(0);
    }
    if(HLCB->GetCheck())
    {	BarValue=BaseUnits->GetSelIndex();
    	switch(BarValue)
    	{	case 0: 	NewHeader.BUnits=1;
    				BaseMult=10.0; break;
    		case 1:   NewHeader.BUnits=3;
    				BaseMult=1.0; break;
    		case 2:   NewHeader.BUnits=2;
    				BaseMult=10.0; break;
    		case 3:   NewHeader.BUnits=4;
    				BaseMult=1.0; break;
    	}
    	HLCBFile=fopen(HlcbFile, "r"); rewind(HLCBFile);
    	for(i=0; i<6; i++)
    		fscanf(HLCBFile, "%s %s", DataType, DataType);
    	NewHeader.CrownFuels=1;
    }
    else if(BConst->GetCheck())
    {	NewHeader.BUnits=5;
    	NewHeader.CrownFuels=1;
             BaseUnits->SetSelIndex(0);
    }
    if(CBD->GetCheck())
    {	BarValue=DensityUnits->GetSelIndex();
    	switch(BarValue)
    	{	case 0: 	NewHeader.PUnits=1;
    				DensityMult=100.0; break;
    		case 1:   NewHeader.PUnits=3;
    				DensityMult=1.0; break;
    		case 2:   NewHeader.PUnits=2;
    				DensityMult=1000.0; break;
    		case 3:   NewHeader.PUnits=4;
    				DensityMult=1.0; break;
    	}
    	PFile=fopen(CbdFile, "r"); rewind(CBDFile);
    	for(i=0; i<6; i++)
    		fscanf(CBDFile, "%s %s", DataType, DataType);
    	NewHeader.CrownFuels=1;

    }
    else if(PConst->GetCheck())
    {	NewHeader.PUnits=5;
        	DensityMult=1.0;
    	NewHeader.CrownFuels=1;
             DensityUnits->SetSelIndex(0);
    }
        if(DF->GetCheck())
        {	if(DuffTA->GetCheck())
        	{	NewHeader.DUnits=2;
              	DuffMult=10.0;	// 10xMg/ha
             }
             else
        	{	NewHeader.DUnits=1;
    		DuffMult=10.0;	   // 10x t/ac
             }
        	DFile=fopen(DuffFile, "r");
    	for(i=0; i<6; i++)
    		fscanf(DFile, "%s %s", DataType, DataType);
        	NewHeader.GroundFuels=1;
        }
        else if(DConst->GetCheck())
        {	NewHeader.DUnits=5;
        	DuffMult=1.0;
        	NewHeader.GroundFuels=1;
             DuffTA->SetCheck(BF_UNCHECKED);
             DuffMgHa->SetCheck(BF_CHECKED);
        }
        if(WDY->GetCheck())
        {	NewHeader.WOptions=1;
        	WFile=fopen(WoodyFile, "r");
         	for(i=0; i<6; i++)
    		fscanf(WFile, "%s %s", DataType, DataType);
        	NewHeader.GroundFuels=1;
        }
        else if(WConst->GetCheck())
        {	NewHeader.WOptions=5;
        	NewHeader.GroundFuels=1;
        }
        */


    memset(NewHeader.ElevFile, 0x0, sizeof(NewHeader.ElevFile));
    memset(NewHeader.SlopeFile, 0x0, sizeof(NewHeader.SlopeFile));
    memset(NewHeader.AspectFile, 0x0, sizeof(NewHeader.AspectFile));
    memset(NewHeader.FuelFile, 0x0, sizeof(NewHeader.FuelFile));
    memset(NewHeader.CoverFile, 0x0, sizeof(NewHeader.CoverFile));
    memset(NewHeader.HeightFile, 0x0, sizeof(NewHeader.HeightFile));
    memset(NewHeader.BaseFile, 0x0, sizeof(NewHeader.BaseFile));
    memset(NewHeader.DensityFile, 0x0, sizeof(NewHeader.DensityFile));
    memset(NewHeader.DuffFile, 0x0, sizeof(NewHeader.DuffFile));
    memset(NewHeader.WoodyFile, 0x0, sizeof(NewHeader.WoodyFile));

    //----------- FOR KM RESOLUTION and GRID UNITS ---------
    CCellSize*=KmToMeters;
    CXLCorner*=KmToMeters;
    CYLCorner*=KmToMeters;
    //------------------------------------------------------

    NewHeader.XResol=CCellSize;
    NewHeader.YResol=CCellSize;
    NewHeader.numnorth=CNRows;
    NewHeader.numeast=CNCols;
    NewHeader.WestUtm=CXLCorner;
    NewHeader.SouthUtm=CYLCorner;
    NewHeader.EastUtm=CXLCorner+((double) CNCols*CCellSize);
    NewHeader.NorthUtm=CYLCorner+((double) CNRows*CCellSize);
    modf(NewHeader.SouthUtm/1000.0, &ipart);
    NewHeader.lonorth=ipart*1000.0;
    NewHeader.lonorth=NewHeader.SouthUtm-NewHeader.lonorth;
    modf(NewHeader.WestUtm/1000.0, &ipart);
    NewHeader.loeast=ipart*1000.0;
    NewHeader.loeast=NewHeader.WestUtm-NewHeader.loeast;
    NewHeader.hieast=NewHeader.numeast*NewHeader.XResol+NewHeader.loeast;
    NewHeader.hinorth=NewHeader.numnorth*NewHeader.YResol+NewHeader.lonorth;
    fwrite(&NewHeader, sizeof(headdata), 1, LFile);		// write the header info
    printf("Size of header data:%d\n",sizeof(headdata));

//	printf("Size of header char:%d",sizeof(int32_t));


    for(i=0; i<10; i++)
    {
        maxval[i]=-1e100;
        minval[i]=1e100;
        NumAllCats[i]=0;
        memset(&AllCats[i], 0x0, 100*sizeof(int32_t));
    }
    double elev, slope, aspect, fuel, cover, height, base, dense, duff, woody;
    for(j=1; j<NewHeader.numnorth+1; j++)
    {
        for(i=0; i<NewHeader.numeast; i++)
        {
            fscanf(EFile, "%lf", &elev);
            if(elev<0.0)
                elev=-9999;
            NewCell.e=((int16_t) elev);

            fscanf(SFile, "%lf", &slope);
            if(slope<0.0)
                slope=-9999;
            NewCell.s=((int16_t) slope);

            fscanf(AFile, "%lf", &aspect);
            if(aspect<0.0)
                aspect=-9999;
            NewCell.a=((int16_t) aspect);

            fscanf(FFile, "%lf", &fuel);
            if(fuel<0) fuel=99;
            NewCell.f=((int16_t) fuel);

            fscanf(CFile, "%lf", &cover);
            if(cover<0.0) cover=0.0;
            NewCell.c=((int16_t) cover);

            fwrite(&NewCell, sizeof(celldata), 1, LFile);
            if(NewHeader.CrownFuels)
            {
                if(NewHeader.HUnits>0)
                {
                    fscanf(HFile, "%lf", &height);
                    if(height<0)
                        height=0;
                    height*=HeightMult;
                }
                else
                    height=-1;
                NewCrown.h=((int16_t) height);
                if(NewHeader.BUnits>0)
                {
                    fscanf(BFile, "%lf", &base);
                    if(base<0)
                        base=0;
                    base*=BaseMult;
                }
                else
                    base=-1;
                NewCrown.b=((int16_t) base);
                if(NewHeader.PUnits>0)
                {
                    fscanf(PFile, "%lf", &dense);
                    if(dense<0)
                        dense=0;
                    dense*=DensityMult;
                    if(dense>32767.0)
                        dense=32767.0;
                }
                else
                    dense=-1;
                NewCrown.p=((int16_t) dense);
                fwrite(&NewCrown, sizeof(crowndata), 1, LFile);
            }
            else
            {
                NewCrown.h=0;
                NewCrown.b=0;
                NewCrown.p=0;
            }
            if(NewHeader.GroundFuels)
            {
                if(NewHeader.DUnits>0)
                {
                    fscanf(DFile, "%lf", &duff);
                    if(duff<0.0) duff=0.0;
                    duff*=DuffMult;
                    if(duff>32767.0)
                        duff=32767.0;
                    NewGround.d=((int16_t) duff);
                }
                else
                    NewGround.d=-1;

                if(NewHeader.WOptions>0)
                {
                    fscanf(WFile, "%lf", &woody);
                    if(woody<0.0) woody=0.0;
                    NewGround.w=((int16_t) woody);
                }
                else
                    NewGround.w=-1;
                fwrite(&NewGround, sizeof(grounddata), 1, LFile);
            }
            else
            {
                NewGround.d=0;
                NewGround.w=0;
            }
            FillCats(&NewCell, &NewCrown, &NewGround);
        }
    }
    SortCats();
    FinalizeHeader(&NewHeader);

    if(NewHeader.CrownFuels)
    {
        if(NewHeader.HUnits==5)
            NewHeader.HUnits=1;
        if(NewHeader.BUnits==5)
            NewHeader.BUnits=1;
        if(NewHeader.PUnits==5)
            NewHeader.PUnits=1;
    }
    if(NewHeader.GroundFuels)
    {
        if(NewHeader.DUnits==5)
            NewHeader.DUnits==1;
        if(NewHeader.WOptions==5)
            NewHeader.WOptions==1;
    }
    rewind(LFile);
    fwrite(&NewHeader, sizeof(NewHeader), 1, LFile);		// write the header info
    fclose(LFile);
    fclose(EFile);
    fclose(SFile);
    fclose(AFile);
    fclose(FFile);
    fclose(CFile);
    if(NewHeader.CrownFuels>20)
    {
        if(NewHeader.HUnits) fclose(HFile);
        if(NewHeader.BUnits) fclose(BFile);
        if(NewHeader.PUnits) fclose(PFile);
    }
    if(NewHeader.GroundFuels>20)
    {
        if(NewHeader.DUnits) fclose(DFile);
        if(NewHeader.WOptions) fclose(WFile);
    }
    SetLandFileName(LndFile);
    OpenLandFile();
    ReadHeader();

    return true;
}


void FinalizeHeader(headdata *NewHeader)
{
    char Description[512]="";

    NewHeader->CrownFuels+=20;
    NewHeader->GroundFuels+=20;
    NewHeader->numelev=NumAllCats[0];
    NewHeader->numslope=NumAllCats[1];
    NewHeader->numaspect=NumAllCats[2];
    NewHeader->numfuel=NumAllCats[3];
    NewHeader->numcover=NumAllCats[4];
    NewHeader->numheight=NumAllCats[5];
    NewHeader->numbase=NumAllCats[6];
    NewHeader->numdensity=NumAllCats[7];
    NewHeader->numduff=NumAllCats[8];
    NewHeader->numwoody=NumAllCats[9];
    memcpy(NewHeader->elevs, AllCats[0], 100*sizeof(int32_t));
    memcpy(NewHeader->slopes, AllCats[1], 100*sizeof(int32_t));
    memcpy(NewHeader->aspects, AllCats[2], 100*sizeof(int32_t));
    memcpy(NewHeader->fuels, AllCats[3], 100*sizeof(int32_t));
    memcpy(NewHeader->covers, AllCats[4], 100*sizeof(int32_t));
    memcpy(NewHeader->heights, AllCats[5], 100*sizeof(int32_t));
    memcpy(NewHeader->bases, AllCats[6], 100*sizeof(int32_t));
    memcpy(NewHeader->densities, AllCats[7], 100*sizeof(int32_t));
    memcpy(NewHeader->duffs, AllCats[8], 100*sizeof(int32_t));
    memcpy(NewHeader->woodies, AllCats[9], 100*sizeof(int32_t));
    NewHeader->loelev=minval[0];
    NewHeader->hielev=maxval[0];
    NewHeader->loslope=minval[1];
    NewHeader->hislope=maxval[1];
    NewHeader->loaspect=minval[2];
    NewHeader->hiaspect=maxval[2];
    NewHeader->lofuel=minval[3];
    NewHeader->hifuel=maxval[3];
    NewHeader->locover=minval[4];
    NewHeader->hicover=maxval[4];
    NewHeader->loheight=minval[5];
    NewHeader->hiheight=maxval[5];
    NewHeader->lobase=minval[6];
    NewHeader->hibase=maxval[6];
    NewHeader->lodensity=minval[7];
    NewHeader->hidensity=maxval[7];
    NewHeader->loduff=minval[8];
    NewHeader->hiduff=maxval[8];
    NewHeader->lowoody=minval[9];
    NewHeader->hiwoody=maxval[9];

// erase existing garbage from header file names
    /*
         memset(NewHeader->ElevFile, sizeof(NewHeader->ElevFile));
         memset(NewHeader->SlopeFile, sizeof(NewHeader->SlopeFile));
         memset(NewHeader->AspectFile, sizeof(NewHeader->AspectFile));
         memset(NewHeader->FuelFile, sizeof(NewHeader->FuelFile));
         memset(NewHeader->CoverFile, sizeof(NewHeader->CoverFile));
         memset(NewHeader->HeightFile, sizeof(NewHeader->HeightFile));
         memset(NewHeader->BaseFile, sizeof(NewHeader->BaseFile));
         memset(NewHeader->DensityFile, sizeof(NewHeader->DensityFile));
         memset(NewHeader->DuffFile, sizeof(NewHeader->DuffFile));
         memset(NewHeader->WoodyFile, sizeof(NewHeader->WoodyFile));
    */

// copy all filenames into the header
    sprintf(NewHeader->ElevFile, "%s", ElFile);
    sprintf(NewHeader->SlopeFile, "%s", SlFile);
    sprintf(NewHeader->AspectFile, "%s", AsFile);
    sprintf(NewHeader->FuelFile, "%s", FlFile);
    //sprintf(NewHeader->FuelFile, "Constant %ld", ConstFuel);
    sprintf(NewHeader->CoverFile, "%s", CvFile);
    //sprintf(NewHeader->CoverFile, "Constant %ld", ConstCover);
    if(NewHeader->CrownFuels>20)
    {
        if(NewHeader->HUnits>0)
            sprintf(NewHeader->HeightFile, "%s", HtFile);
        if(NewHeader->BUnits>0)
            sprintf(NewHeader->BaseFile, "%s", HlcbFile);
        if(NewHeader->PUnits>0)
            sprintf(NewHeader->DensityFile, "%s", CbdFile);
    }
    if(NewHeader->GroundFuels>20)
    {
        if(NewHeader->DUnits>0)
            sprintf(NewHeader->DuffFile, "%s", DuffFile);
        if(NewHeader->WOptions>0)
            sprintf(NewHeader->WoodyFile, "%s", WoodyFile);
    }

    //int32_t i, len=0;
    //char temp[512]="";
    //memset(Description, 0x0, 512*sizeof(char));
    //strcpy(NewHeader->Description, Description);
}



void FillCats(celldata *cell, crowndata *cfuel, grounddata *gfuel)
{
    int32_t k, m;//, pos;

    AllCats[0][NumAllCats[0]]=cell->e;
    AllCats[1][NumAllCats[1]]=cell->s;
    AllCats[2][NumAllCats[2]]=cell->a;
    AllCats[3][NumAllCats[3]]=cell->f;
    AllCats[4][NumAllCats[4]]=cell->c;
    if(HaveCrownFuels())
    {
        if(cfuel->h>=0)
            AllCats[5][NumAllCats[5]]=cfuel->h;
        if(cfuel->b>=0)
            AllCats[6][NumAllCats[6]]=cfuel->b;
        if(cfuel->p>=0)
            AllCats[7][NumAllCats[7]]=cfuel->p;
    }
    if(HaveGroundFuels())
    {
        if(gfuel->d>=0)
            AllCats[8][NumAllCats[8]]=gfuel->d;
        if(gfuel->w>=0)
            AllCats[9][NumAllCats[9]]=gfuel->w;
    }
    for(m=0; m<10; m++)
    {
        if(maxval[m]<AllCats[m][NumAllCats[m]])
            maxval[m]=AllCats[m][NumAllCats[m]];
        if(AllCats[m][NumAllCats[m]]!=NODATA && AllCats[m][NumAllCats[m]]>=0)
        {
            if(minval[m]>AllCats[m][NumAllCats[m]])
                minval[m]=AllCats[m][NumAllCats[m]];
        }
    }
    for(m=0; m<10; m++)
    {
        if(NumAllCats[m]>98)
            continue;
        for(k=0; k<NumAllCats[m]; k++)
        {
            if(AllCats[m][NumAllCats[m]]==AllCats[m][k])
                break;
        }
        if(k==NumAllCats[m])
            NumAllCats[m]++;
    }
}


void SortCats()
{
    int32_t i, j, m;
    int32_t SwapCats[101];

    for(m=0; m<10; m++)
    {
        if(NumAllCats[m]>98)
            NumAllCats[m]=-1;
    }
    for(m=0; m<10; m++)
    {
        if(NumAllCats[m]<0)
            continue;
        memcpy(SwapCats, AllCats[m], 100*sizeof(int32_t));
        for(i=0; i<NumAllCats[m]-1; i++)
        {
            for(j=i+1; j<NumAllCats[m]; j++)
            {
                if(SwapCats[j]<SwapCats[i])
                {
                    SwapCats[100]=SwapCats[i];
                    SwapCats[i]=SwapCats[j];
                    SwapCats[j]=SwapCats[100];
                }
            }
        }
        AllCats[m][0]=0;
        for(i=0; i<NumAllCats[m]; i++)
            AllCats[m][i+1]=SwapCats[i];
        minval[m]=AllCats[m][1];  // zero slot is for nodata value
        if(minval[m]<0)
            minval[m]=0;
    }
}

