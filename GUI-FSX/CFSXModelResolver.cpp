#include "stdafx.h"
#include "CFSXModelResolver.h"


CFSXModelResolver::CFSXModelResolver()
{
}

CFSXModelResolver::~CFSXModelResolver()
{
}

//Initialize
int CFSXModelResolver::Initialize()
{
	//DEBUG (does work)
	/*
	CParser P;
	P.SetFileAsInput("D:\\aircraft.cfg");
	char ATCModel[8];
	int iNumEngines;
	bool bIsJet;
	float fWingspanFt, fApproxLengthFt;
	CSimpleArray<LiveryStruct *> aLiveries;
	int r = ParseAircraftInfo(P, ATCModel, &iNumEngines, &bIsJet, &fWingspanFt, &fApproxLengthFt, aLiveries);

	LiveryStruct *p = aLiveries[0];
	*/

	return 1;
}

//Read in aircraft.cfg data given Parser opened to that file, returning atc model (which is hopefully the ICAO
//aircraft type but no guarantee), number of engines, whether or not it's a jet, wingspan, rough length in 
//feet (derived length which is 2x distance from forward-most wheel to rear-most), and an array
//of installed liveries (fltsim.X section) which contain the actual title= name to spawn it, and atc_airline name
//if any (general aviation/generic liveries usually don't contain that). Returns 1 if all needed sections were 
//found, or returns 0 if 1+ sections weren't found (although some of the other sections may have been and their 
//data may or not be valid). 
//
//Special case: if returned ATCModel is "VIP", it means this is a VIP model set where each aLiveries->SpawnName should be
//in the VIP3 naming convention where the string contains both ICAO type and ICAO airline name, while NumEngines, bIsJet
//etc should not be used. (VIP base model set is over 2000 liveries). **Note there could be multiple "VIP" model sets 
//installed** 
int CFSXModelResolver::ParseAircraftInfo(CParser &Parser, char *ATCModel, int *piNumEngines, bool *pbIsJet,
	float *pfWingspanFt, float *pfApproxLengthFt, CSimpleArray<LiveryStruct *> &apLiveries)

{
	char LineBuff[512], DummyBuff[512];
	int res, NumSectionsParsed = 0;

	ATCModel[0] = 0;
	*piNumEngines = 0;
	*pbIsJet = true;
	*pfWingspanFt = 0.0f;
	*pfApproxLengthFt = 0.0f;

	do
	{
		res = Parser.SkipToNextSection(LineBuff, 512);
		if (res)
		{
			//Skip over closing ]
			res = Parser.GetRestOfLine(DummyBuff, 512);

			if (strstr(LineBuff, "FLTSIM"))
				ReadFltSimSection(Parser, apLiveries);
			else
			{
				//Count number of needed sections so we can stop parsing this file once
				//we've read them all (we don't count FLTSIM because there can be multiple 
				//of those)
				NumSectionsParsed++;

				if (strstr(LineBuff, "GENERALENGINEDATA"))
					ReadGeneralEngineSection(Parser, piNumEngines, pbIsJet);
				else if (strstr(LineBuff, "GENERAL"))
					ReadGeneralSection(Parser, ATCModel);
				else if (strstr(LineBuff, "CONTACT"))
					ReadContactSection(Parser, pfApproxLengthFt);
				else if (strstr(LineBuff, "GEOMETRY"))
					ReadGeometrySection(Parser, pfWingspanFt);
				else
					NumSectionsParsed--;

			}

		}
	} while (res && NumSectionsParsed < 4);

	//Check for special case VIP model set
	if (ATCModel[0] == 0)
	{
		if (apLiveries.GetSize() > 0 && apLiveries[0]->SpawnName.find("VIP_") != std::string::npos)
			strcpy_s(ATCModel, 4, "VIP");
	}

	if (NumSectionsParsed == 5)
		return 1;
	return 0;
}

//All of the "Read X Section" methods below will either leave Parser at EOF or pointing to next section
//header. When called, it's assumed parser is pointing to next spot after closing ] from section header.

//Read in one [fltsim.X] section and add it to given aLiveries array
void CFSXModelResolver::ReadFltSimSection(CParser &Parser, CSimpleArray<LiveryStruct *> &apLiveries)
{
	char LineBuff[512];
	int res;
	LiveryStruct *pLivery = new LiveryStruct;
	do
	{
		res = Parser.PeekString(LineBuff, 512);
		if (res && !strstr(LineBuff, "["))
		{
			if (strstr(LineBuff, "TITLE"))
			{
				Parser.GetString(LineBuff, 512);  //Get the string "TITLE" since we just peeked it
				Parser.GetRestOfLine(LineBuff, 512);  //Get the actual value
				pLivery->SpawnName = LineBuff;
			}
			else if (strstr(LineBuff, "ATC_AIR")) //atc_airline
			{
				Parser.GetString(LineBuff, 512);
				Parser.GetRestOfLine(LineBuff, 512);
				pLivery->ReportedAirline = LineBuff;
			}
			else
				res = Parser.GetRestOfLine(LineBuff, 512);
		}
		else if (res)  //i.e. reached new section header or EOF
			res = 0;
	} while (res);
	
	if (pLivery->SpawnName.length() > 0)
		apLiveries.Add(pLivery);
	else
		delete pLivery;

	return;
}

void CFSXModelResolver::ReadGeneralEngineSection(CParser &Parser, int *piNumEngines, bool *pbIsJet)
{
	char LineBuff[512];
	int res, val = 1;
	do
	{
		res = Parser.PeekString(LineBuff, 512);
		if (res && !strstr(LineBuff, "["))
		{
			if (strstr(LineBuff, "_TYPE"))
			{
				Parser.GetString(LineBuff, 512);  //Get the string "ENGINE_TYPE" since we just peeked it
				Parser.GetInt(&val);  //Get the actual value
				if (val != 1)
					*pbIsJet = false;
				else
					*pbIsJet = true;
			}
			else if (strstr(LineBuff, "ENGINE."))
			{
				Parser.GetRestOfLine(LineBuff, 512);  //get and discard the line
				(*piNumEngines)++;
			}
			else
				res = Parser.GetRestOfLine(LineBuff, 512);
		}
		else if (res)  //i.e. reached new section header
			res = 0;
	} while (res);


	return;
}

//ATCModel string must be at least 8 chars
void CFSXModelResolver::ReadGeneralSection(CParser &Parser, char *ATCModel)
{
	char LineBuff[512];
	int res;
	do
	{
		res = Parser.PeekString(LineBuff, 512);
		if (res && !strstr(LineBuff, "["))
		{
			if (strstr(LineBuff, "ATC_M"))
			{
				Parser.GetString(LineBuff, 512);  //Get the string "ATC_MODEL" since we just peeked it
				Parser.GetRestOfLine(LineBuff, 512);  //Get the actual value
				LineBuff[5] = 0;  //just in case
				strcpy_s(ATCModel, 8, LineBuff);   //we are assuming string is at least 8 chars...
				return;
			}
			else
				res = Parser.GetRestOfLine(LineBuff, 512);
		}
		else if (res)  //i.e. reached new section header
			res = 0;
	} while (res);

	return;
}

void CFSXModelResolver::ReadContactSection(CParser &Parser, float *pfApproxLengthFt)
{
	char LineBuff[512];
	int res, val = 1;
	float fVal, fHighest = -999999.0f, fLowest = 999999.0f;
	do
	{
		res = Parser.PeekString(LineBuff, 512);
		if (res && !strstr(LineBuff, "["))
		{
			if (strstr(LineBuff, "POINT."))
			{
				res = Parser.GetString(LineBuff, 512);  //Get the string "POINT.X" since we just peeked it
				res = Parser.GetInt(&val);  //Contact type

				//Landing gear/skid/float?
				if (res && (val == 1 || val == 3 || val == 4))
				{
					//Get Longitudinal position
					if (Parser.GetFloat(&fVal))
					{
						if (fVal > fHighest)
							fHighest = fVal;
						if (fVal < fLowest)
							fLowest = fVal;
					}
				}
				res = Parser.GetRestOfLine(LineBuff, 512);
			}
			else
				res = Parser.GetRestOfLine(LineBuff, 512);
		}
		else if (res)  //i.e. reached new section header
			res = 0;
	} while (res);
	*pfApproxLengthFt = 2.0f * (fHighest - fLowest);
	if (*pfApproxLengthFt > 5000.0f || *pfApproxLengthFt < 0)
		*pfApproxLengthFt = 0.0f; 

	return;
}

void CFSXModelResolver::ReadGeometrySection(CParser &Parser, float *pfWingspanFt)
{
	char LineBuff[512];
	float fVal;
	*pfWingspanFt = 0.0f;
	int res;
	do
	{
		res = Parser.PeekString(LineBuff, 512);
		if (res && !strstr(LineBuff, "["))
		{
			if (strstr(LineBuff, "WING_SPAN"))
			{
				Parser.GetString(LineBuff, 512);  //Get the string "WING_SPAN" since we just peeked it
				res = Parser.GetFloat(&fVal);    //Get the actual value
				if (res)
					*pfWingspanFt = fVal;
			}
			else
				Parser.GetRestOfLine(LineBuff, 512);
		}
		else if (res)  //i.e. reached new section header
			res = 0;
	} while (res);

	return;
}


//Given a (hopefully realistic) callsign and (hopefully accurate) ICAO type, return the best installed model 
//name to use, which is string after "title=" in the aircraft.cfg file. Example -- Callsign = DAL123, Type = B738. 
//Strings are zero-terminated, output string szFSXModelName must be >= 256 bytes including terminating 0. (The
//returned name is what is given to FSX to spawn that particular model and livery).
int CFSXModelResolver::GetBestModelForCallsignAndType(char *szCallsign, char *szICAOType, char *szFSXModelName)
{
	strcpy_s(szFSXModelName, 256, "Beechcraft Bonanza F33A"); //DEBUG

	//TODO: iterate through all installed aircraft files, use above routines to parse their aircraft.cfg, assign
	//ICAO a/c types to each one (they may or may not be labelled correctly so use database of common ICAO types
	//and type and number of engines and a/c length to find a match); then find what common ICAO types don't have
	//an exact match to what's installed and find the closest that does. At the end we should have a list of 
	//common ICAO aircraft types and each should have an associated "either exact match or close enough" installed
	//a/c name (and list of installed ICAO liveries).
	//
	return 1;
}
