#pragma once

#include "stdafx.h"
#include "CParser.h"

typedef struct LiveryStruct
{
	std::string SpawnName;        //"title=" name to spawn it within FSX
	std::string ReportedAirline;  //Reported "atc_airline" name from aircraft.cfg file, e.g. "Delta" or "Kenmore Air". 
	std::string ICAOAirline;      //3-letter ICAO airline name we either derived from ReportedAirline or SpawnName, 
								  //  e.g. DAL or UAL, or GA meaning general aviation/none/unknown 
} LiveryStruct;

typedef class CFSXModelResolver
{
public:

	CFSXModelResolver();
	~CFSXModelResolver();

	//Initialize
	int Initialize();

	//Read in aircraft.cfg data given Parser opened to that file, returning atc model (which is hopefully the ICAO
	//aircraft type but no guarantee), number of engines, whether or not it's a jet, wingspan, rough length in 
	//feet (derived length which is 2x distance from forward-most wheel to rear-most), and an array
	//of installed liveries (fltsim.X section) which contain the actual title= name to spawn it.
	int ParseAircraftInfo(CParser &Parser, char *ATCModel, int *piNumEngines, bool *pbIsJet, float *pfWingspanFt,
		float *pfApproxLengthFt, CSimpleArray<LiveryStruct *> &apLiveries);

	void ReadFltSimSection(CParser &Parser, CSimpleArray<LiveryStruct *> &apLiveries);
	void ReadGeneralEngineSection(CParser &Parser, int *piNumEngines, bool *pbIsJet);
	void ReadGeneralSection(CParser &Parser, char *ATCModel);
	void ReadContactSection(CParser &Parser, float *pfApproxLengthFt);
	void ReadGeometrySection(CParser &Parser, float *pfWingspanFt);

	//Given a (hopefully realistic) callsign and (hopefully accurate) ICAO type, return the best installed FSX model 
	//name to use, which is the string after "title=" in the aircraft.cfg file. Example: Callsign = DAL123, ICAOType = B738. 
	//Strings are zero-terminated, output string szFSXModelName must be >= 256 characters including terminating 0.
	//This method should return a default valid aircraft if no match is found, so should always return 1 (success). 
	int GetBestModelForCallsignAndType(char *szCallsign, char *szICAOType, char *szFSXModelName, double *pdGearHeightFt);

} CFSXModelResolver;
