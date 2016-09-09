#pragma once

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the GUIFSX_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// GUIFSX_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef GUIFSX_EXPORTS
#define GUIFSX_API __declspec(dllexport)
#else
#define GUIFSX_API __declspec(dllimport)
#endif

GUIFSX_API void DLLStart();
GUIFSX_API void DLLStop();
//Simulator events for simconnect
typedef enum eFSXEvent
{
	//GUI-FSX.cpp handle these
	EVENT_FRAME,
	EVENT_SIM_RUNNING,

	//GUI handles this
	EVENT_ADDONMENU_SELECTED,

	//CFSXObjects handle these
	EVENT_ADDED_AIRCRAFT,
	EVENT_REMOVED_AIRCRAFT,

	//We send these events to FSX (in CFSXObjects::SendFSXEvent)
	EVENT_FREEZELAT,
	EVENT_FREEZEALT,
	EVENT_FREEZEATT,
	EVENT_ENGINESON,
	EVENT_ENGINESOFF,
	EVENT_GEARSET,
	EVENT_FLAPSUP,
	EVENT_FLAPS1,
	EVENT_FLAPS2,
	EVENT_FLAPS3,
	EVENT_FLAPSFULL,
	EVENT_LANDLIGHTSSET,        //data field 1 or 0 (on or off)
	EVENT_STROBESSET,           //data field 1 or 0
	EVENT_NAVLIGHTSTOGGLE,
	EVENT_TAXILIGHTSTOGGLE,
	EVENT_BEACONLIGHTSTOGGLE,
	EVENT_LOGOLIGHTSTOGGLE,
	EVENT_TOGGLEJETWAY,

	EVENT_HIGHEST
} eFSXEvent;