#pragma once

#include "stdafx.h"

typedef class CFSXModelResolver
{
public:

	CFSXModelResolver();
	~CFSXModelResolver();

	//Initialize
	int Initialize();

	//Given a (hopefully realistic) callsign and (hopefully accurate) ICAO type, return the best installed FSX model 
	//name to use, which is the string after "title=" in the aircraft.cfg file. Example: Callsign = DAL123, ICAOType = B738. 
	//Strings are zero-terminated, output string szFSXModelName must be < 256 characters including terminating 0.
	//This method should return a default valid aircraft if no match is found, so should always return 1 (success). 
	int GetBestModelForCallsignAndType(char *szCallsign, char *szICAOType, char *szFSXModelName);

} CFSXModelResolver;
