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
	return 1;
}

//Given a (hopefully realistic) callsign and (hopefully accurate) ICAO type, return the best installed FSX model 
//name to use, which is string after "title=" in the aircraft.cfg file. Example -- Callsign = DAL123, Type = B738. 
//Strings are zero-terminated, output string szFSXModelName must be < 256 characters including terminating 0.
int CFSXModelResolver::GetBestModelForCallsignAndType(char *szCallsign, char *szICAOType, char *szFSXModelName)
{
	strcpy_s(szFSXModelName, 256, "Boeing 737-800 Paint1");
	return 1;
}
