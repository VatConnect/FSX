//-----------------------------------------------------------------------------
//
// CFSWSimConnect class released under MIT license.
//
// Remainder Copyright (c) Microsoft Corporation. All Rights Reserved. Used 
//
//-----------------------------------------------------------------------------

#ifndef _SIMCONNECTFSW_H_
#define _SIMCONNECTFSW_H_

#pragma once

#ifndef DWORD_MAX
#define DWORD_MAX 0xFFFFFFFF
#endif

#include <float.h>

typedef DWORD SIMCONNECT_OBJECT_ID;

//----------------------------------------------------------------------------
//        Constants
//----------------------------------------------------------------------------

static const DWORD SIMCONNECT_UNUSED           = DWORD_MAX;   // special value to indicate unused event, ID
static const DWORD SIMCONNECT_OBJECT_ID_USER   = 0;           // proxy value for User vehicle ObjectID

static const float SIMCONNECT_CAMERA_IGNORE_FIELD   = FLT_MAX;  //Used to tell the Camera API to NOT modify the value in this part of the argument.

static const DWORD SIMCONNECT_CLIENTDATA_MAX_SIZE = 8192;     // maximum value for SimConnect_CreateClientData dwSize parameter


// Notification Group priority values
static const DWORD SIMCONNECT_GROUP_PRIORITY_HIGHEST              =          1;      // highest priority
static const DWORD SIMCONNECT_GROUP_PRIORITY_HIGHEST_MASKABLE     =   10000000;      // highest priority that allows events to be masked
static const DWORD SIMCONNECT_GROUP_PRIORITY_STANDARD             = 1900000000;      // standard priority
static const DWORD SIMCONNECT_GROUP_PRIORITY_DEFAULT              = 2000000000;      // default priority
static const DWORD SIMCONNECT_GROUP_PRIORITY_LOWEST               = 4000000000;      // priorities lower than this will be ignored

//Weather observations Metar strings
static const DWORD MAX_METAR_LENGTH = 2000;

// Maximum thermal size is 100 km.
static const float MAX_THERMAL_SIZE = 100000;
static const float MAX_THERMAL_RATE = 1000;

// SIMCONNECT_DATA_INITPOSITION.Airspeed
static const DWORD INITPOSITION_AIRSPEED_CRUISE = -1;       // aircraft's cruise airspeed
static const DWORD INITPOSITION_AIRSPEED_KEEP = -2;         // keep current airspeed

// AddToClientDataDefinition dwSizeOrType parameter type values
static const DWORD SIMCONNECT_CLIENTDATATYPE_INT8       = -1;   //  8-bit integer number
static const DWORD SIMCONNECT_CLIENTDATATYPE_INT16      = -2;   // 16-bit integer number
static const DWORD SIMCONNECT_CLIENTDATATYPE_INT32      = -3;   // 32-bit integer number
static const DWORD SIMCONNECT_CLIENTDATATYPE_INT64      = -4;   // 64-bit integer number
static const DWORD SIMCONNECT_CLIENTDATATYPE_FLOAT32    = -5;   // 32-bit floating-point number (float)
static const DWORD SIMCONNECT_CLIENTDATATYPE_FLOAT64    = -6;   // 64-bit floating-point number (double)

// AddToClientDataDefinition dwOffset parameter special values
static const DWORD SIMCONNECT_CLIENTDATAOFFSET_AUTO    = -1;   // automatically compute offset of the ClientData variable

// Open ConfigIndex parameter special value
static const DWORD SIMCONNECT_OPEN_CONFIGINDEX_LOCAL   = -1;   // ignore SimConnect.cfg settings, and force local connection

//----------------------------------------------------------------------------
//        Enum definitions
//----------------------------------------------------------------------------

// Receive data types
enum SIMCONNECT_RECV_ID {
    SIMCONNECT_RECV_ID_NULL,
    SIMCONNECT_RECV_ID_EXCEPTION,
    SIMCONNECT_RECV_ID_OPEN,
    SIMCONNECT_RECV_ID_QUIT,
    SIMCONNECT_RECV_ID_EVENT,
    SIMCONNECT_RECV_ID_EVENT_OBJECT_ADDREMOVE,
    SIMCONNECT_RECV_ID_EVENT_FILENAME,
    SIMCONNECT_RECV_ID_EVENT_FRAME,
    SIMCONNECT_RECV_ID_SIMOBJECT_DATA,
    SIMCONNECT_RECV_ID_SIMOBJECT_DATA_BYTYPE,
    SIMCONNECT_RECV_ID_WEATHER_OBSERVATION,
    SIMCONNECT_RECV_ID_CLOUD_STATE,
    SIMCONNECT_RECV_ID_ASSIGNED_OBJECT_ID,
    SIMCONNECT_RECV_ID_RESERVED_KEY,
    SIMCONNECT_RECV_ID_CUSTOM_ACTION,
    SIMCONNECT_RECV_ID_SYSTEM_STATE,
    SIMCONNECT_RECV_ID_CLIENT_DATA,
    SIMCONNECT_RECV_ID_EVENT_WEATHER_MODE,
    SIMCONNECT_RECV_ID_AIRPORT_LIST,
    SIMCONNECT_RECV_ID_VOR_LIST,
    SIMCONNECT_RECV_ID_NDB_LIST,
    SIMCONNECT_RECV_ID_WAYPOINT_LIST,
    SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_SERVER_STARTED,
    SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_CLIENT_STARTED,
    SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_SESSION_ENDED,
    SIMCONNECT_RECV_ID_EVENT_RACE_END,
    SIMCONNECT_RECV_ID_EVENT_RACE_LAP,

};



// Data data types
enum SIMCONNECT_DATATYPE {
    SIMCONNECT_DATATYPE_INVALID,        // invalid data type
    SIMCONNECT_DATATYPE_INT32,          // 32-bit integer number
    SIMCONNECT_DATATYPE_INT64,          // 64-bit integer number
    SIMCONNECT_DATATYPE_FLOAT32,        // 32-bit floating-point number (float)
    SIMCONNECT_DATATYPE_FLOAT64,        // 64-bit floating-point number (double)
    SIMCONNECT_DATATYPE_STRING8,        // 8-byte string
    SIMCONNECT_DATATYPE_STRING32,       // 32-byte string
    SIMCONNECT_DATATYPE_STRING64,       // 64-byte string
    SIMCONNECT_DATATYPE_STRING128,      // 128-byte string
    SIMCONNECT_DATATYPE_STRING256,      // 256-byte string
    SIMCONNECT_DATATYPE_STRING260,      // 260-byte string
    SIMCONNECT_DATATYPE_STRINGV,        // variable-length string

    SIMCONNECT_DATATYPE_INITPOSITION,   // see SIMCONNECT_DATA_INITPOSITION
    SIMCONNECT_DATATYPE_MARKERSTATE,    // see SIMCONNECT_DATA_MARKERSTATE
    SIMCONNECT_DATATYPE_WAYPOINT,       // see SIMCONNECT_DATA_WAYPOINT
    SIMCONNECT_DATATYPE_LATLONALT,      // see SIMCONNECT_DATA_LATLONALT
    SIMCONNECT_DATATYPE_XYZ,            // see SIMCONNECT_DATA_XYZ

    SIMCONNECT_DATATYPE_MAX             // enum limit
};

// Exception error types
enum SIMCONNECT_EXCEPTION {
    SIMCONNECT_EXCEPTION_NONE,

    SIMCONNECT_EXCEPTION_ERROR,
    SIMCONNECT_EXCEPTION_SIZE_MISMATCH,
    SIMCONNECT_EXCEPTION_UNRECOGNIZED_ID,
    SIMCONNECT_EXCEPTION_UNOPENED,
    SIMCONNECT_EXCEPTION_VERSION_MISMATCH,
    SIMCONNECT_EXCEPTION_TOO_MANY_GROUPS,
    SIMCONNECT_EXCEPTION_NAME_UNRECOGNIZED,
    SIMCONNECT_EXCEPTION_TOO_MANY_EVENT_NAMES,
    SIMCONNECT_EXCEPTION_EVENT_ID_DUPLICATE,
    SIMCONNECT_EXCEPTION_TOO_MANY_MAPS,
    SIMCONNECT_EXCEPTION_TOO_MANY_OBJECTS,
    SIMCONNECT_EXCEPTION_TOO_MANY_REQUESTS,
    SIMCONNECT_EXCEPTION_WEATHER_INVALID_PORT,
    SIMCONNECT_EXCEPTION_WEATHER_INVALID_METAR,
    SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_GET_OBSERVATION,
    SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_CREATE_STATION,
    SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_REMOVE_STATION,
    SIMCONNECT_EXCEPTION_INVALID_DATA_TYPE,
    SIMCONNECT_EXCEPTION_INVALID_DATA_SIZE,
    SIMCONNECT_EXCEPTION_DATA_ERROR,
    SIMCONNECT_EXCEPTION_INVALID_ARRAY,
    SIMCONNECT_EXCEPTION_CREATE_OBJECT_FAILED,
    SIMCONNECT_EXCEPTION_LOAD_FLIGHTPLAN_FAILED,
    SIMCONNECT_EXCEPTION_OPERATION_INVALID_FOR_OBJECT_TYPE,
    SIMCONNECT_EXCEPTION_ILLEGAL_OPERATION,
    SIMCONNECT_EXCEPTION_ALREADY_SUBSCRIBED,
    SIMCONNECT_EXCEPTION_INVALID_ENUM,
    SIMCONNECT_EXCEPTION_DEFINITION_ERROR,
    SIMCONNECT_EXCEPTION_DUPLICATE_ID,
    SIMCONNECT_EXCEPTION_DATUM_ID,
    SIMCONNECT_EXCEPTION_OUT_OF_BOUNDS,
    SIMCONNECT_EXCEPTION_ALREADY_CREATED,
    SIMCONNECT_EXCEPTION_OBJECT_OUTSIDE_REALITY_BUBBLE,
    SIMCONNECT_EXCEPTION_OBJECT_CONTAINER,
    SIMCONNECT_EXCEPTION_OBJECT_AI,
    SIMCONNECT_EXCEPTION_OBJECT_ATC,
    SIMCONNECT_EXCEPTION_OBJECT_SCHEDULE,
};

// Object types
enum SIMCONNECT_SIMOBJECT_TYPE {
    SIMCONNECT_SIMOBJECT_TYPE_USER,
    SIMCONNECT_SIMOBJECT_TYPE_ALL,
    SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT,
    SIMCONNECT_SIMOBJECT_TYPE_HELICOPTER,
    SIMCONNECT_SIMOBJECT_TYPE_BOAT,
    SIMCONNECT_SIMOBJECT_TYPE_GROUND,
};

// EventState values
enum SIMCONNECT_STATE {
    SIMCONNECT_STATE_OFF,
    SIMCONNECT_STATE_ON,
};

// Object Data Request Period values
enum SIMCONNECT_PERIOD {
    SIMCONNECT_PERIOD_NEVER,
    SIMCONNECT_PERIOD_ONCE,
    SIMCONNECT_PERIOD_VISUAL_FRAME,
    SIMCONNECT_PERIOD_SIM_FRAME,
    SIMCONNECT_PERIOD_SECOND,
};


enum SIMCONNECT_MISSION_END {
    SIMCONNECT_MISSION_FAILED,
    SIMCONNECT_MISSION_CRASHED,
    SIMCONNECT_MISSION_SUCCEEDED
};

// ClientData Request Period values
enum SIMCONNECT_CLIENT_DATA_PERIOD {
    SIMCONNECT_CLIENT_DATA_PERIOD_NEVER,
    SIMCONNECT_CLIENT_DATA_PERIOD_ONCE,
    SIMCONNECT_CLIENT_DATA_PERIOD_VISUAL_FRAME,
    SIMCONNECT_CLIENT_DATA_PERIOD_ON_SET,
    SIMCONNECT_CLIENT_DATA_PERIOD_SECOND,
};

enum SIMCONNECT_TEXT_TYPE {
    SIMCONNECT_TEXT_TYPE_SCROLL_BLACK,
    SIMCONNECT_TEXT_TYPE_SCROLL_WHITE,
    SIMCONNECT_TEXT_TYPE_SCROLL_RED,
    SIMCONNECT_TEXT_TYPE_SCROLL_GREEN,
    SIMCONNECT_TEXT_TYPE_SCROLL_BLUE,
    SIMCONNECT_TEXT_TYPE_SCROLL_YELLOW,
    SIMCONNECT_TEXT_TYPE_SCROLL_MAGENTA,
    SIMCONNECT_TEXT_TYPE_SCROLL_CYAN,
    SIMCONNECT_TEXT_TYPE_PRINT_BLACK=0x0100,
    SIMCONNECT_TEXT_TYPE_PRINT_WHITE,
    SIMCONNECT_TEXT_TYPE_PRINT_RED,
    SIMCONNECT_TEXT_TYPE_PRINT_GREEN,
    SIMCONNECT_TEXT_TYPE_PRINT_BLUE,
    SIMCONNECT_TEXT_TYPE_PRINT_YELLOW,
    SIMCONNECT_TEXT_TYPE_PRINT_MAGENTA,
    SIMCONNECT_TEXT_TYPE_PRINT_CYAN,
    SIMCONNECT_TEXT_TYPE_MENU=0x0200,
};

enum SIMCONNECT_TEXT_RESULT {
    SIMCONNECT_TEXT_RESULT_MENU_SELECT_1,
    SIMCONNECT_TEXT_RESULT_MENU_SELECT_2,
    SIMCONNECT_TEXT_RESULT_MENU_SELECT_3,
    SIMCONNECT_TEXT_RESULT_MENU_SELECT_4,
    SIMCONNECT_TEXT_RESULT_MENU_SELECT_5,
    SIMCONNECT_TEXT_RESULT_MENU_SELECT_6,
    SIMCONNECT_TEXT_RESULT_MENU_SELECT_7,
    SIMCONNECT_TEXT_RESULT_MENU_SELECT_8,
    SIMCONNECT_TEXT_RESULT_MENU_SELECT_9,
    SIMCONNECT_TEXT_RESULT_MENU_SELECT_10,
    SIMCONNECT_TEXT_RESULT_DISPLAYED = 0x00010000,
    SIMCONNECT_TEXT_RESULT_QUEUED,
    SIMCONNECT_TEXT_RESULT_REMOVED,
    SIMCONNECT_TEXT_RESULT_REPLACED,
    SIMCONNECT_TEXT_RESULT_TIMEOUT,
};

enum SIMCONNECT_WEATHER_MODE {
    SIMCONNECT_WEATHER_MODE_THEME,
    SIMCONNECT_WEATHER_MODE_RWW,
    SIMCONNECT_WEATHER_MODE_CUSTOM,
    SIMCONNECT_WEATHER_MODE_GLOBAL,
};

enum SIMCONNECT_FACILITY_LIST_TYPE {
    SIMCONNECT_FACILITY_LIST_TYPE_AIRPORT,
    SIMCONNECT_FACILITY_LIST_TYPE_WAYPOINT,
    SIMCONNECT_FACILITY_LIST_TYPE_NDB,
    SIMCONNECT_FACILITY_LIST_TYPE_VOR,
    SIMCONNECT_FACILITY_LIST_TYPE_COUNT // invalid 
};

typedef DWORD SIMCONNECT_VOR_FLAGS;            // flags for SIMCONNECT_RECV_ID_VOR_LIST 

    static const DWORD SIMCONNECT_RECV_ID_VOR_LIST_HAS_NAV_SIGNAL  = 0x00000001;   // Has Nav signal
    static const DWORD SIMCONNECT_RECV_ID_VOR_LIST_HAS_LOCALIZER   = 0x00000002;   // Has localizer
    static const DWORD SIMCONNECT_RECV_ID_VOR_LIST_HAS_GLIDE_SLOPE = 0x00000004;   // Has Nav signal
    static const DWORD SIMCONNECT_RECV_ID_VOR_LIST_HAS_DME         = 0x00000008;   // Station has DME



// bits for the Waypoint Flags field: may be combined
typedef DWORD SIMCONNECT_WAYPOINT_FLAGS;

    static const DWORD SIMCONNECT_WAYPOINT_NONE                    = 0x00;
    static const DWORD SIMCONNECT_WAYPOINT_SPEED_REQUESTED         = 0x04;    // requested speed at waypoint is valid
    static const DWORD SIMCONNECT_WAYPOINT_THROTTLE_REQUESTED      = 0x08;    // request a specific throttle percentage
    static const DWORD SIMCONNECT_WAYPOINT_COMPUTE_VERTICAL_SPEED  = 0x10;    // compute vertical to speed to reach waypoint altitude when crossing the waypoint
    static const DWORD SIMCONNECT_WAYPOINT_ALTITUDE_IS_AGL         = 0x20;    // AltitudeIsAGL
    static const DWORD SIMCONNECT_WAYPOINT_ON_GROUND               = 0x00100000;   // place this waypoint on the ground
    static const DWORD SIMCONNECT_WAYPOINT_REVERSE                 = 0x00200000;   // Back up to this waypoint. Only valid on first waypoint
    static const DWORD SIMCONNECT_WAYPOINT_WRAP_TO_FIRST           = 0x00400000;   // Wrap around back to first waypoint. Only valid on last waypoint.


typedef DWORD SIMCONNECT_EVENT_FLAG;

    static const DWORD SIMCONNECT_EVENT_FLAG_DEFAULT                  = 0x00000000;
    static const DWORD SIMCONNECT_EVENT_FLAG_FAST_REPEAT_TIMER        = 0x00000001;      // set event repeat timer to simulate fast repeat
    static const DWORD SIMCONNECT_EVENT_FLAG_SLOW_REPEAT_TIMER        = 0x00000002;      // set event repeat timer to simulate slow repeat
    static const DWORD SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY      = 0x00000010;      // interpret GroupID parameter as priority value


typedef DWORD SIMCONNECT_DATA_REQUEST_FLAG;

    static const DWORD SIMCONNECT_DATA_REQUEST_FLAG_DEFAULT           = 0x00000000;
    static const DWORD SIMCONNECT_DATA_REQUEST_FLAG_CHANGED           = 0x00000001;      // send requested data when value(s) change
    static const DWORD SIMCONNECT_DATA_REQUEST_FLAG_TAGGED            = 0x00000002;      // send requested data in tagged format


typedef DWORD SIMCONNECT_DATA_SET_FLAG;

    static const DWORD SIMCONNECT_DATA_SET_FLAG_DEFAULT               = 0x00000000;
    static const DWORD SIMCONNECT_DATA_SET_FLAG_TAGGED                = 0x00000001;      // data is in tagged format


typedef DWORD SIMCONNECT_CREATE_CLIENT_DATA_FLAG;

    static const DWORD SIMCONNECT_CREATE_CLIENT_DATA_FLAG_DEFAULT     = 0x00000000;
    static const DWORD SIMCONNECT_CREATE_CLIENT_DATA_FLAG_READ_ONLY   = 0x00000001;      // permit only ClientData creator to write into ClientData


typedef DWORD SIMCONNECT_CLIENT_DATA_REQUEST_FLAG;

    static const DWORD SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_DEFAULT    = 0x00000000;
    static const DWORD SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_CHANGED    = 0x00000001;      // send requested ClientData when value(s) change
    static const DWORD SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_TAGGED     = 0x00000002;      // send requested ClientData in tagged format


typedef DWORD SIMCONNECT_CLIENT_DATA_SET_FLAG;

    static const DWORD SIMCONNECT_CLIENT_DATA_SET_FLAG_DEFAULT        = 0x00000000;
    static const DWORD SIMCONNECT_CLIENT_DATA_SET_FLAG_TAGGED         = 0x00000001;      // data is in tagged format


typedef DWORD SIMCONNECT_VIEW_SYSTEM_EVENT_DATA;                  // dwData contains these flags for the "View" System Event

    static const DWORD SIMCONNECT_VIEW_SYSTEM_EVENT_DATA_COCKPIT_2D      = 0x00000001;      // 2D Panels in cockpit view
    static const DWORD SIMCONNECT_VIEW_SYSTEM_EVENT_DATA_COCKPIT_VIRTUAL = 0x00000002;      // Virtual (3D) panels in cockpit view
    static const DWORD SIMCONNECT_VIEW_SYSTEM_EVENT_DATA_ORTHOGONAL      = 0x00000004;      // Orthogonal (Map) view


typedef DWORD SIMCONNECT_SOUND_SYSTEM_EVENT_DATA;            // dwData contains these flags for the "Sound" System Event

    static const DWORD SIMCONNECT_SOUND_SYSTEM_EVENT_DATA_MASTER    = 0x00000001;      // Sound Master




//----------------------------------------------------------------------------
//        User-defined enums
//----------------------------------------------------------------------------

typedef DWORD SIMCONNECT_NOTIFICATION_GROUP_ID;     //client-defined notification group ID
typedef DWORD SIMCONNECT_INPUT_GROUP_ID;            //client-defined input group ID
typedef DWORD SIMCONNECT_DATA_DEFINITION_ID;        //client-defined data definition ID
typedef DWORD SIMCONNECT_DATA_REQUEST_ID;           //client-defined request data ID
 
typedef DWORD SIMCONNECT_CLIENT_EVENT_ID;           //client-defined client event ID
typedef DWORD SIMCONNECT_CLIENT_DATA_ID;            //client-defined client data ID
typedef DWORD SIMCONNECT_CLIENT_DATA_DEFINITION_ID; //client-defined client data definition ID


//----------------------------------------------------------------------------
//        Struct definitions
//----------------------------------------------------------------------------

#pragma pack(push, 1)

struct SIMCONNECT_RECV
{
    DWORD   dwSize;         // record size
    DWORD   dwVersion;      // interface version
    DWORD   dwID;           // see SIMCONNECT_RECV_ID
};

struct SIMCONNECT_RECV_EXCEPTION : public SIMCONNECT_RECV   // when dwID == SIMCONNECT_RECV_ID_EXCEPTION
{
    DWORD   dwException;    // see SIMCONNECT_EXCEPTION
    //static const DWORD UNKNOWN_SENDID = 0;
    DWORD   dwSendID;       // see SimConnect_GetLastSentPacketID
    //static const UNKNOWN_INDEX = DWORD_MAX;
    DWORD   dwIndex;        // index of parameter that was source of error
};

struct SIMCONNECT_RECV_OPEN : public SIMCONNECT_RECV   // when dwID == SIMCONNECT_RECV_ID_OPEN
{
    char    szApplicationName[256];
    DWORD   dwApplicationVersionMajor;
    DWORD   dwApplicationVersionMinor;
    DWORD   dwApplicationBuildMajor;
    DWORD   dwApplicationBuildMinor;
    DWORD   dwSimConnectVersionMajor;
    DWORD   dwSimConnectVersionMinor;
    DWORD   dwSimConnectBuildMajor;
    DWORD   dwSimConnectBuildMinor;
    DWORD   dwReserved1;
    DWORD   dwReserved2;
};

struct SIMCONNECT_RECV_QUIT : public SIMCONNECT_RECV   // when dwID == SIMCONNECT_RECV_ID_QUIT
{
};

struct SIMCONNECT_RECV_EVENT : public SIMCONNECT_RECV       // when dwID == SIMCONNECT_RECV_ID_EVENT
{
    //static const DWORD UNKNOWN_GROUP = DWORD_MAX;
    DWORD   uGroupID;
    DWORD   uEventID; 
    DWORD   dwData;       // uEventID-dependent context
};

struct SIMCONNECT_RECV_EVENT_FILENAME : public SIMCONNECT_RECV_EVENT       // when dwID == SIMCONNECT_RECV_ID_EVENT_FILENAME
{
    char    szFileName[MAX_PATH];   // uEventID-dependent context
    DWORD   dwFlags;
};

struct SIMCONNECT_RECV_EVENT_OBJECT_ADDREMOVE : public SIMCONNECT_RECV_EVENT       // when dwID == SIMCONNECT_RECV_ID_EVENT_FILENAME
{
    SIMCONNECT_SIMOBJECT_TYPE   eObjType;
};

struct SIMCONNECT_RECV_EVENT_FRAME : public SIMCONNECT_RECV_EVENT       // when dwID == SIMCONNECT_RECV_ID_EVENT_FRAME
{
    float   fFrameRate;
    float   fSimSpeed;
};

struct SIMCONNECT_RECV_EVENT_MULTIPLAYER_SERVER_STARTED : public SIMCONNECT_RECV_EVENT       // when dwID == SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_SERVER_STARTED
{
    // No event specific data, for now
};

struct SIMCONNECT_RECV_EVENT_MULTIPLAYER_CLIENT_STARTED : public SIMCONNECT_RECV_EVENT       // when dwID == SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_CLIENT_STARTED
{
    // No event specific data, for now
};

struct SIMCONNECT_RECV_EVENT_MULTIPLAYER_SESSION_ENDED : public SIMCONNECT_RECV_EVENT       // when dwID == SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_SESSION_ENDED
{
    // No event specific data, for now
};

// SIMCONNECT_DATA_RACE_RESULT
struct SIMCONNECT_DATA_RACE_RESULT
{
    DWORD   dwNumberOfRacers;                         // The total number of racers
    GUID MissionGUID;                      // The name of the mission to execute, NULL if no mission
    char szPlayerName[MAX_PATH];       // The name of the player
    char szSessionType[MAX_PATH];      // The type of the multiplayer session: "LAN", "GAMESPY")
    char szAircraft[MAX_PATH];         // The aircraft type 
    char szPlayerRole[MAX_PATH];       // The player role in the mission
    double   fTotalTime;                              // Total time in seconds, 0 means DNF
    double   fPenaltyTime;                            // Total penalty time in seconds
    DWORD   dwIsDisqualified;                         // non 0 - disqualified, 0 - not disqualified
};

struct SIMCONNECT_RECV_EVENT_RACE_END : public SIMCONNECT_RECV_EVENT       // when dwID == SIMCONNECT_RECV_ID_EVENT_RACE_END
{
    DWORD   dwRacerNumber;                            // The index of the racer the results are for
    SIMCONNECT_DATA_RACE_RESULT RacerData;
};

struct SIMCONNECT_RECV_EVENT_RACE_LAP : public SIMCONNECT_RECV_EVENT       // when dwID == SIMCONNECT_RECV_ID_EVENT_RACE_LAP
{
    DWORD   dwLapIndex;                               // The index of the lap the results are for
    SIMCONNECT_DATA_RACE_RESULT RacerData;
};

struct SIMCONNECT_RECV_SIMOBJECT_DATA : public SIMCONNECT_RECV           // when dwID == SIMCONNECT_RECV_ID_SIMOBJECT_DATA
{
    DWORD   dwRequestID;
    DWORD   dwObjectID;
    DWORD   dwDefineID;
    DWORD   dwFlags;            // SIMCONNECT_DATA_REQUEST_FLAG
    DWORD   dwentrynumber;      // if multiple objects returned, this is number <entrynumber> out of <outof>.
    DWORD   dwoutof;            // note: starts with 1, not 0.          
    DWORD   dwDefineCount;      // data count (number of datums, *not* byte count)
    DWORD   dwData;             // data begins here, dwDefineCount data items
};

struct SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE : public SIMCONNECT_RECV_SIMOBJECT_DATA           // when dwID == SIMCONNECT_RECV_ID_SIMOBJECT_DATA_BYTYPE
{
};

struct SIMCONNECT_RECV_CLIENT_DATA : public SIMCONNECT_RECV_SIMOBJECT_DATA    // when dwID == SIMCONNECT_RECV_ID_CLIENT_DATA
{
};

struct SIMCONNECT_RECV_WEATHER_OBSERVATION : public SIMCONNECT_RECV // when dwID == SIMCONNECT_RECV_ID_WEATHER_OBSERVATION
{
    DWORD   dwRequestID;
    char szMetar[1];      // Variable length string whose maximum size is MAX_METAR_LENGTH
};

static const int SIMCONNECT_CLOUD_STATE_ARRAY_WIDTH = 64;
static const int SIMCONNECT_CLOUD_STATE_ARRAY_SIZE = SIMCONNECT_CLOUD_STATE_ARRAY_WIDTH*SIMCONNECT_CLOUD_STATE_ARRAY_WIDTH;

struct SIMCONNECT_RECV_CLOUD_STATE : public SIMCONNECT_RECV // when dwID == SIMCONNECT_RECV_ID_CLOUD_STATE
{
    DWORD   dwRequestID;
    DWORD   dwArraySize;
    BYTE    rgbData[1];
};

struct SIMCONNECT_RECV_ASSIGNED_OBJECT_ID : public SIMCONNECT_RECV // when dwID == SIMCONNECT_RECV_ID_ASSIGNED_OBJECT_ID
{
    DWORD   dwRequestID;
    DWORD   dwObjectID;
};

struct SIMCONNECT_RECV_RESERVED_KEY : public SIMCONNECT_RECV // when dwID == SIMCONNECT_RECV_ID_RESERVED_KEY
{
    char    szChoiceReserved[30];
    char    szReservedKey[50];
};

struct SIMCONNECT_RECV_SYSTEM_STATE : public SIMCONNECT_RECV // when dwID == SIMCONNECT_RECV_ID_SYSTEM_STATE
{
    DWORD   dwRequestID;
    DWORD   dwInteger;
    float   fFloat;
    char    szString[MAX_PATH];
};

struct SIMCONNECT_RECV_CUSTOM_ACTION : public SIMCONNECT_RECV_EVENT
{
    GUID guidInstanceId;      // Instance id of the action that executed
    DWORD dwWaitForCompletion;           // Wait for completion flag on the action
    char szPayLoad[1];      // Variable length string payload associated with the mission action.  
};

struct SIMCONNECT_RECV_EVENT_WEATHER_MODE : public SIMCONNECT_RECV_EVENT
{
    // No event specific data - the new weather mode is in the base structure dwData member.
};

// SIMCONNECT_RECV_FACILITIES_LIST
struct SIMCONNECT_RECV_FACILITIES_LIST : public SIMCONNECT_RECV
{
    DWORD   dwRequestID;
    DWORD   dwArraySize;
    DWORD   dwEntryNumber;  // when the array of items is too big for one send, which send this is (0..dwOutOf-1)
    DWORD   dwOutOf;        // total number of transmissions the list is chopped into
};

// SIMCONNECT_DATA_FACILITY_AIRPORT
struct SIMCONNECT_DATA_FACILITY_AIRPORT
{
    char Icao[9];     // ICAO of the object
    double  Latitude;               // degrees
    double  Longitude;              // degrees
    double  Altitude;               // meters   
};

// SIMCONNECT_RECV_AIRPORT_LIST
struct SIMCONNECT_RECV_AIRPORT_LIST : public SIMCONNECT_RECV_FACILITIES_LIST
{
    SIMCONNECT_DATA_FACILITY_AIRPORT rgData[1];
};


// SIMCONNECT_DATA_FACILITY_WAYPOINT
struct SIMCONNECT_DATA_FACILITY_WAYPOINT : public SIMCONNECT_DATA_FACILITY_AIRPORT
{
    float   fMagVar;                // Magvar in degrees
};

// SIMCONNECT_RECV_WAYPOINT_LIST
struct SIMCONNECT_RECV_WAYPOINT_LIST : public SIMCONNECT_RECV_FACILITIES_LIST
{
    SIMCONNECT_DATA_FACILITY_WAYPOINT rgData[1];
};

// SIMCONNECT_DATA_FACILITY_NDB
struct SIMCONNECT_DATA_FACILITY_NDB : public SIMCONNECT_DATA_FACILITY_WAYPOINT
{
    DWORD   fFrequency;             // frequency in Hz
};

// SIMCONNECT_RECV_NDB_LIST
struct SIMCONNECT_RECV_NDB_LIST : public SIMCONNECT_RECV_FACILITIES_LIST
{
    SIMCONNECT_DATA_FACILITY_NDB rgData[1];
};

// SIMCONNECT_DATA_FACILITY_VOR
struct SIMCONNECT_DATA_FACILITY_VOR : public SIMCONNECT_DATA_FACILITY_NDB
{
    DWORD   Flags;                  // SIMCONNECT_VOR_FLAGS
    float   fLocalizer;             // Localizer in degrees
    double  GlideLat;               // Glide Slope Location (deg, deg, meters)
    double  GlideLon;
    double  GlideAlt;
    float   fGlideSlopeAngle;       // Glide Slope in degrees
};

// SIMCONNECT_RECV_VOR_LIST
struct SIMCONNECT_RECV_VOR_LIST : public SIMCONNECT_RECV_FACILITIES_LIST
{
    SIMCONNECT_DATA_FACILITY_VOR rgData[1];
};




// SIMCONNECT_DATATYPE_INITPOSITION
struct SIMCONNECT_DATA_INITPOSITION
{
    double  Latitude;   // degrees
    double  Longitude;  // degrees
    double  Altitude;   // feet   
    double  Pitch;      // degrees
    double  Bank;       // degrees
    double  Heading;    // degrees
    DWORD   OnGround;   // 1=force to be on the ground
    DWORD   Airspeed;   // knots
};


// SIMCONNECT_DATATYPE_MARKERSTATE
struct SIMCONNECT_DATA_MARKERSTATE
{
    char    szMarkerName[64];
    DWORD   dwMarkerState;
};

// SIMCONNECT_DATATYPE_WAYPOINT
struct SIMCONNECT_DATA_WAYPOINT
{
    double          Latitude;   // degrees
    double          Longitude;  // degrees
    double          Altitude;   // feet   
    unsigned long   Flags;
    double          ktsSpeed;   // knots
    double          percentThrottle;
};

// SIMCONNECT_DATA_LATLONALT
struct SIMCONNECT_DATA_LATLONALT
{
    double  Latitude;
    double  Longitude;
    double  Altitude;
};

// SIMCONNECT_DATA_XYZ
struct SIMCONNECT_DATA_XYZ
{
    double  x;
    double  y;
    double  z;
};

#pragma pack(pop)

typedef void (CALLBACK *DispatchProc)(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext);

//----------------------------------------------------------------------------
//        End of Struct definitions
//----------------------------------------------------------------------------


class CFSWSimConnect
{
	typedef HRESULT(__stdcall *t_SimConnect_MapClientEventToSimEvent)(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID EventID, const char * EventName);
	typedef HRESULT(__stdcall *t_SimConnect_TransmitClientEvent)(HANDLE hSimConnect, SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_CLIENT_EVENT_ID EventID, DWORD dwData, SIMCONNECT_NOTIFICATION_GROUP_ID GroupID, SIMCONNECT_EVENT_FLAG Flags);
	typedef HRESULT(__stdcall *t_SimConnect_SetSystemEventState)(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID EventID, SIMCONNECT_STATE dwState);
	typedef HRESULT(__stdcall *t_SimConnect_AddClientEventToNotificationGroup)(HANDLE hSimConnect, SIMCONNECT_NOTIFICATION_GROUP_ID GroupID, SIMCONNECT_CLIENT_EVENT_ID EventID, BOOL bMaskable);
	typedef HRESULT(__stdcall *t_SimConnect_RemoveClientEvent)(HANDLE hSimConnect, SIMCONNECT_NOTIFICATION_GROUP_ID GroupID, SIMCONNECT_CLIENT_EVENT_ID EventID);
	typedef HRESULT(__stdcall *t_SimConnect_SetNotificationGroupPriority)(HANDLE hSimConnect, SIMCONNECT_NOTIFICATION_GROUP_ID GroupID, DWORD uPriority);
	typedef HRESULT(__stdcall *t_SimConnect_ClearNotificationGroup)(HANDLE hSimConnect, SIMCONNECT_NOTIFICATION_GROUP_ID GroupID);
	typedef HRESULT(__stdcall *t_SimConnect_RequestNotificationGroup)(HANDLE hSimConnect, SIMCONNECT_NOTIFICATION_GROUP_ID GroupID, DWORD dwReserved, DWORD Flags);
	typedef HRESULT(__stdcall *t_SimConnect_AddToDataDefinition)(HANDLE hSimConnect, SIMCONNECT_DATA_DEFINITION_ID DefineID, const char * DatumName, const char * UnitsName, SIMCONNECT_DATATYPE DatumType, float fEpsilon, DWORD DatumID);
	typedef HRESULT(__stdcall *t_SimConnect_ClearDataDefinition)(HANDLE hSimConnect, SIMCONNECT_DATA_DEFINITION_ID DefineID);
	typedef HRESULT(__stdcall *t_SimConnect_RequestDataOnSimObject)(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID, SIMCONNECT_DATA_DEFINITION_ID DefineID, SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_PERIOD Period, SIMCONNECT_DATA_REQUEST_FLAG Flags, DWORD origin, DWORD interval, DWORD limit);
	typedef HRESULT(__stdcall *t_SimConnect_RequestDataOnSimObjectType)(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID, SIMCONNECT_DATA_DEFINITION_ID DefineID, DWORD dwRadiusMeters, SIMCONNECT_SIMOBJECT_TYPE type);	
	typedef HRESULT(__stdcall *t_SimConnect_SetDataOnSimObject)(HANDLE hSimConnect, SIMCONNECT_DATA_DEFINITION_ID DefineID, SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_DATA_SET_FLAG Flags, DWORD ArrayCount, DWORD cbUnitSize, void * pDataSet);
	typedef HRESULT(__stdcall *t_SimConnect_MapInputEventToClientEvent)(HANDLE hSimConnect, SIMCONNECT_INPUT_GROUP_ID GroupID, const char * szInputDefinition, SIMCONNECT_CLIENT_EVENT_ID DownEventID, DWORD DownValue, SIMCONNECT_CLIENT_EVENT_ID UpEventID, DWORD UpValue, BOOL bMaskable);
	typedef HRESULT(__stdcall *t_SimConnect_SetInputGroupPriority)(HANDLE hSimConnect, SIMCONNECT_INPUT_GROUP_ID GroupID, DWORD uPriority);
	typedef HRESULT(__stdcall *t_SimConnect_RemoveInputEvent)(HANDLE hSimConnect, SIMCONNECT_INPUT_GROUP_ID GroupID, const char * szInputDefinition);
	typedef HRESULT(__stdcall *t_SimConnect_ClearInputGroup)(HANDLE hSimConnect, SIMCONNECT_INPUT_GROUP_ID GroupID);
	typedef HRESULT(__stdcall *t_SimConnect_SetInputGroupState)(HANDLE hSimConnect, SIMCONNECT_INPUT_GROUP_ID GroupID, DWORD dwState);
	typedef HRESULT(__stdcall *t_SimConnect_RequestReservedKey)(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID EventID, const char * szKeyChoice1, const char * szKeyChoice2, const char * szKeyChoice3);
	typedef HRESULT(__stdcall *t_SimConnect_SubscribeToSystemEvent)(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID EventID, const char * SystemEventName);
	typedef HRESULT(__stdcall *t_SimConnect_UnsubscribeFromSystemEvent)(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID EventID);
	typedef HRESULT(__stdcall *t_SimConnect_WeatherRequestInterpolatedObservation)(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID, float lat, float lon, float alt);
	typedef HRESULT(__stdcall *t_SimConnect_WeatherRequestObservationAtStation)(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID, const char * szICAO);
	typedef HRESULT(__stdcall *t_SimConnect_WeatherRequestObservationAtNearestStation)(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID, float lat, float lon);
	typedef HRESULT(__stdcall *t_SimConnect_WeatherCreateStation)(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID, const char * szICAO, const char * szName, float lat, float lon, float alt);
	typedef HRESULT(__stdcall *t_SimConnect_WeatherRemoveStation)(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID, const char * szICAO);
	typedef HRESULT(__stdcall *t_SimConnect_WeatherSetObservation)(HANDLE hSimConnect, DWORD Seconds, const char * szMETAR);
	typedef HRESULT(__stdcall *t_SimConnect_WeatherSetModeServer)(HANDLE hSimConnect, DWORD dwPort, DWORD dwSeconds);
	typedef HRESULT(__stdcall *t_SimConnect_WeatherSetModeTheme)(HANDLE hSimConnect, const char * szThemeName);
	typedef HRESULT(__stdcall *t_SimConnect_WeatherSetModeGlobal)(HANDLE hSimConnect);
	typedef HRESULT(__stdcall *t_SimConnect_WeatherSetModeCustom)(HANDLE hSimConnect);
	typedef HRESULT(__stdcall *t_SimConnect_WeatherSetDynamicUpdateRate)(HANDLE hSimConnect, DWORD dwRate);
	typedef HRESULT(__stdcall *t_SimConnect_WeatherRequestCloudState)(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID, float minLat, float minLon, float minAlt, float maxLat, float maxLon, float maxAlt, DWORD dwFlags);
	typedef HRESULT(__stdcall *t_SimConnect_WeatherCreateThermal)(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID, float lat, float lon, float alt, float radius, float height, float coreRate, float coreTurbulence, float sinkRate, float sinkTurbulence, float coreSize, float coreTransitionSize, float sinkLayerSize, float sinkTransitionSize);
	typedef HRESULT(__stdcall *t_SimConnect_WeatherRemoveThermal)(HANDLE hSimConnect, SIMCONNECT_OBJECT_ID ObjectID);
	typedef HRESULT(__stdcall *t_SimConnect_AICreateParkedATCAircraft)(HANDLE hSimConnect, const char * szContainerTitle, const char * szTailNumber, const char * szAirportID, SIMCONNECT_DATA_REQUEST_ID RequestID);
	typedef HRESULT(__stdcall *t_SimConnect_AICreateEnrouteATCAircraft)(HANDLE hSimConnect, const char * szContainerTitle, const char * szTailNumber, int iFlightNumber, const char * szFlightPlanPath, double dFlightPlanPosition, BOOL bTouchAndGo, SIMCONNECT_DATA_REQUEST_ID RequestID);
	typedef HRESULT(__stdcall *t_SimConnect_AICreateNonATCAircraft)(HANDLE hSimConnect, const char * szContainerTitle, const char * szTailNumber, SIMCONNECT_DATA_INITPOSITION InitPos, SIMCONNECT_DATA_REQUEST_ID RequestID);
	typedef HRESULT(__stdcall *t_SimConnect_AICreateSimulatedObject)(HANDLE hSimConnect, const char * szContainerTitle, SIMCONNECT_DATA_INITPOSITION InitPos, SIMCONNECT_DATA_REQUEST_ID RequestID);
	typedef HRESULT(__stdcall *t_SimConnect_AIReleaseControl)(HANDLE hSimConnect, SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_DATA_REQUEST_ID RequestID);
	typedef HRESULT(__stdcall *t_SimConnect_AIRemoveObject)(HANDLE hSimConnect, SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_DATA_REQUEST_ID RequestID);
	typedef HRESULT(__stdcall *t_SimConnect_AISetAircraftFlightPlan)(HANDLE hSimConnect, SIMCONNECT_OBJECT_ID ObjectID, const char * szFlightPlanPath, SIMCONNECT_DATA_REQUEST_ID RequestID);
	typedef HRESULT(__stdcall *t_SimConnect_ExecuteMissionAction)(HANDLE hSimConnect, const GUID guidInstanceId);
	typedef HRESULT(__stdcall *t_SimConnect_CompleteCustomMissionAction)(HANDLE hSimConnect, const GUID guidInstanceId);
	typedef HRESULT(__stdcall *t_SimConnect_Close)(HANDLE hSimConnect);
	typedef HRESULT(__stdcall *t_SimConnect_RetrieveString)(SIMCONNECT_RECV * pData, DWORD cbData, void * pStringV, char ** pszString, DWORD * pcbString);
	typedef HRESULT(__stdcall *t_SimConnect_GetLastSentPacketID)(HANDLE hSimConnect, DWORD * pdwError);
	typedef HRESULT(__stdcall *t_SimConnect_Open)(HANDLE * phSimConnect, LPCSTR szName, HWND hWnd, DWORD UserEventWin32, HANDLE hEventHandle, DWORD ConfigIndex);
	typedef HRESULT(__stdcall *t_SimConnect_CallDispatch)(HANDLE hSimConnect, DispatchProc pfcnDispatch, void * pContext);
	typedef HRESULT(__stdcall *t_SimConnect_GetNextDispatch)(HANDLE hSimConnect, SIMCONNECT_RECV ** ppData, DWORD * pcbData);
	typedef HRESULT(__stdcall *t_SimConnect_RequestResponseTimes)(HANDLE hSimConnect, DWORD nCount, float * fElapsedSeconds);
	typedef HRESULT(__stdcall *t_SimConnect_InsertString)(char * pDest, DWORD cbDest, void ** ppEnd, DWORD * pcbStringV, const char * pSource);
	typedef HRESULT(__stdcall *t_SimConnect_CameraSetRelative6DOF)(HANDLE hSimConnect, float fDeltaX, float fDeltaY, float fDeltaZ, float fPitchDeg, float fBankDeg, float fHeadingDeg);
	typedef HRESULT(__stdcall *t_SimConnect_MenuAddItem)(HANDLE hSimConnect, const char * szMenuItem, SIMCONNECT_CLIENT_EVENT_ID MenuEventID, DWORD dwData);
	typedef HRESULT(__stdcall *t_SimConnect_MenuDeleteItem)(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID MenuEventID);
	typedef HRESULT(__stdcall *t_SimConnect_MenuAddSubItem)(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID MenuEventID, const char * szMenuItem, SIMCONNECT_CLIENT_EVENT_ID SubMenuEventID, DWORD dwData);
	typedef HRESULT(__stdcall *t_SimConnect_MenuDeleteSubItem)(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID MenuEventID, const SIMCONNECT_CLIENT_EVENT_ID SubMenuEventID);
	typedef HRESULT(__stdcall *t_SimConnect_RequestSystemState)(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID, const char * szState);
	typedef HRESULT(__stdcall *t_SimConnect_SetSystemState)(HANDLE hSimConnect, const char * szState, DWORD dwInteger, float fFloat, const char * szString);
	typedef HRESULT(__stdcall *t_SimConnect_MapClientDataNameToID)(HANDLE hSimConnect, const char * szClientDataName, SIMCONNECT_CLIENT_DATA_ID ClientDataID);
	typedef HRESULT(__stdcall *t_SimConnect_CreateClientData)(HANDLE hSimConnect, SIMCONNECT_CLIENT_DATA_ID ClientDataID, DWORD dwSize, SIMCONNECT_CREATE_CLIENT_DATA_FLAG Flags);
	typedef HRESULT(__stdcall *t_SimConnect_AddToClientDataDefinition)(HANDLE hSimConnect, SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID, DWORD dwOffset, DWORD dwSizeOrType, float fEpsilon, DWORD DatumID);
	typedef HRESULT(__stdcall *t_SimConnect_ClearClientDataDefinition)(HANDLE hSimConnect, SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID);
	typedef HRESULT(__stdcall *t_SimConnect_RequestClientData)(HANDLE hSimConnect, SIMCONNECT_CLIENT_DATA_ID ClientDataID, SIMCONNECT_DATA_REQUEST_ID RequestID, SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID, SIMCONNECT_CLIENT_DATA_PERIOD Period, SIMCONNECT_CLIENT_DATA_REQUEST_FLAG Flags, DWORD origin, DWORD interval, DWORD limit);
	typedef HRESULT(__stdcall *t_SimConnect_SetClientData)(HANDLE hSimConnect, SIMCONNECT_CLIENT_DATA_ID ClientDataID, SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID, SIMCONNECT_CLIENT_DATA_SET_FLAG Flags, DWORD dwReserved, DWORD cbUnitSize, void * pDataSet);
	typedef HRESULT(__stdcall *t_SimConnect_FlightLoad)(HANDLE hSimConnect, const char * szFileName);
	typedef HRESULT(__stdcall *t_SimConnect_FlightSave)(HANDLE hSimConnect, const char * szFileName, const char * szTitle, const char * szDescription, DWORD Flags);
	typedef HRESULT(__stdcall *t_SimConnect_FlightPlanLoad)(HANDLE hSimConnect, const char * szFileName);
	typedef HRESULT(__stdcall *t_SimConnect_Text)(HANDLE hSimConnect, SIMCONNECT_TEXT_TYPE type, float fTimeSeconds, SIMCONNECT_CLIENT_EVENT_ID EventID, DWORD cbUnitSize, void * pDataSet);
	typedef HRESULT(__stdcall *t_SimConnect_SubscribeToFacilities)(HANDLE hSimConnect, SIMCONNECT_FACILITY_LIST_TYPE type, SIMCONNECT_DATA_REQUEST_ID RequestID);
	typedef HRESULT(__stdcall *t_SimConnect_UnsubscribeToFacilities)(HANDLE hSimConnect, SIMCONNECT_FACILITY_LIST_TYPE type);
	typedef HRESULT(__stdcall *t_SimConnect_RequestFacilitiesList)(HANDLE hSimConnect, SIMCONNECT_FACILITY_LIST_TYPE type, SIMCONNECT_DATA_REQUEST_ID RequestID);

public:
	CFSWSimConnect() : m_bInitialized(false), m_hSimConnect(NULL), m_hSimConnectDLL(false),
		m_pSimConnect_MapClientEventToSimEvent(nullptr),
		m_pSimConnect_TransmitClientEvent(nullptr),
		m_pSimConnect_SetSystemEventState(nullptr),
		m_pSimConnect_AddClientEventToNotificationGroup(nullptr),
		m_pSimConnect_RemoveClientEvent(nullptr),
		m_pSimConnect_SetNotificationGroupPriority(nullptr),
		m_pSimConnect_ClearNotificationGroup(nullptr),
		m_pSimConnect_RequestNotificationGroup(nullptr),
		m_pSimConnect_AddToDataDefinition(nullptr),
		m_pSimConnect_ClearDataDefinition(nullptr),
		m_pSimConnect_RequestDataOnSimObject(nullptr),
		m_pSimConnect_RequestDataOnSimObjectType(nullptr),
		m_pSimConnect_SetDataOnSimObject(nullptr),
		m_pSimConnect_MapInputEventToClientEvent(nullptr),
		m_pSimConnect_SetInputGroupPriority(nullptr),
		m_pSimConnect_RemoveInputEvent(nullptr),
		m_pSimConnect_ClearInputGroup(nullptr),
		m_pSimConnect_SetInputGroupState(nullptr),
		m_pSimConnect_RequestReservedKey(nullptr),
		m_pSimConnect_SubscribeToSystemEvent(nullptr),
		m_pSimConnect_UnsubscribeFromSystemEvent(nullptr),
		m_pSimConnect_WeatherRequestInterpolatedObservation(nullptr),
		m_pSimConnect_WeatherRequestObservationAtStation(nullptr),
		m_pSimConnect_WeatherRequestObservationAtNearestStation(nullptr),
		m_pSimConnect_WeatherCreateStation(nullptr),
		m_pSimConnect_WeatherRemoveStation(nullptr),
		m_pSimConnect_WeatherSetObservation(nullptr),
		m_pSimConnect_WeatherSetModeServer(nullptr),
		m_pSimConnect_WeatherSetModeTheme(nullptr),
		m_pSimConnect_WeatherSetModeGlobal(nullptr),
		m_pSimConnect_WeatherSetModeCustom(nullptr),
		m_pSimConnect_WeatherSetDynamicUpdateRate(nullptr),
		m_pSimConnect_WeatherRequestCloudState(nullptr),
		m_pSimConnect_WeatherCreateThermal(nullptr),
		m_pSimConnect_WeatherRemoveThermal(nullptr),
		m_pSimConnect_AICreateParkedATCAircraft(nullptr),
		m_pSimConnect_AICreateEnrouteATCAircraft(nullptr),
		m_pSimConnect_AICreateNonATCAircraft(nullptr),
		m_pSimConnect_AICreateSimulatedObject(nullptr),
		m_pSimConnect_AIReleaseControl(nullptr),
		m_pSimConnect_AIRemoveObject(nullptr),
		m_pSimConnect_AISetAircraftFlightPlan(nullptr),
		m_pSimConnect_ExecuteMissionAction(nullptr),
		m_pSimConnect_CompleteCustomMissionAction(nullptr),
		m_pSimConnect_Close(nullptr),
		m_pSimConnect_RetrieveString(nullptr),
		m_pSimConnect_GetLastSentPacketID(nullptr),
		m_pSimConnect_Open(nullptr),
		m_pSimConnect_CallDispatch(nullptr),
		m_pSimConnect_GetNextDispatch(nullptr),
		m_pSimConnect_RequestResponseTimes(nullptr),
		m_pSimConnect_InsertString(nullptr),
		m_pSimConnect_CameraSetRelative6DOF(nullptr),
		m_pSimConnect_MenuAddItem(nullptr),
		m_pSimConnect_MenuDeleteItem(nullptr),
		m_pSimConnect_MenuAddSubItem(nullptr),
		m_pSimConnect_MenuDeleteSubItem(nullptr),
		m_pSimConnect_RequestSystemState(nullptr),
		m_pSimConnect_SetSystemState(nullptr),
		m_pSimConnect_MapClientDataNameToID(nullptr),
		m_pSimConnect_CreateClientData(nullptr),
		m_pSimConnect_AddToClientDataDefinition(nullptr),
		m_pSimConnect_ClearClientDataDefinition(nullptr),
		m_pSimConnect_RequestClientData(nullptr),
		m_pSimConnect_SetClientData(nullptr),
		m_pSimConnect_FlightLoad(nullptr),
		m_pSimConnect_FlightSave(nullptr),
		m_pSimConnect_FlightPlanLoad(nullptr),
		m_pSimConnect_Text(nullptr),
		m_pSimConnect_SubscribeToFacilities(nullptr),
		m_pSimConnect_UnsubscribeToFacilities(nullptr),
		m_pSimConnect_RequestFacilitiesList(nullptr) {};
	~CFSWSimConnect() { Close(); };

	HRESULT MapClientEventToSimEvent(SIMCONNECT_CLIENT_EVENT_ID EventID, const char * EventName = "")
	{
		if (m_pSimConnect_MapClientEventToSimEvent)
			return m_pSimConnect_MapClientEventToSimEvent(m_hSimConnect, EventID, EventName);
		return E_POINTER;
	}
	HRESULT TransmitClientEvent(SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_CLIENT_EVENT_ID EventID, DWORD dwData, SIMCONNECT_NOTIFICATION_GROUP_ID GroupID, SIMCONNECT_EVENT_FLAG Flags)
	{
		if (m_pSimConnect_TransmitClientEvent)
			return m_pSimConnect_TransmitClientEvent(m_hSimConnect, ObjectID, EventID, dwData, GroupID, Flags);
		return E_POINTER;
	}
	HRESULT SetSystemEventState(SIMCONNECT_CLIENT_EVENT_ID EventID, SIMCONNECT_STATE dwState)
	{
		if (m_pSimConnect_SetSystemEventState)
			return m_pSimConnect_SetSystemEventState(m_hSimConnect, EventID, dwState);
		return E_POINTER;
	}
	HRESULT AddClientEventToNotificationGroup(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID, SIMCONNECT_CLIENT_EVENT_ID EventID, BOOL bMaskable = FALSE)
	{
		if (m_pSimConnect_AddClientEventToNotificationGroup)
			return m_pSimConnect_AddClientEventToNotificationGroup(m_hSimConnect, GroupID, EventID, bMaskable);
		return E_POINTER;
	}
	HRESULT RemoveClientEvent(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID, SIMCONNECT_CLIENT_EVENT_ID EventID)
	{
		if (m_pSimConnect_RemoveClientEvent)
			return m_pSimConnect_RemoveClientEvent(m_hSimConnect, GroupID, EventID);
		return E_POINTER;
	}
	HRESULT SetNotificationGroupPriority(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID, DWORD uPriority)
	{
		if (m_pSimConnect_SetNotificationGroupPriority)
			return m_pSimConnect_SetNotificationGroupPriority(m_hSimConnect, GroupID, uPriority);
		return E_POINTER;
	}
	HRESULT ClearNotificationGroup(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID)
	{
		if (m_pSimConnect_ClearNotificationGroup)
			return m_pSimConnect_ClearNotificationGroup(m_hSimConnect, GroupID);
		return E_POINTER;
	}
	HRESULT RequestNotificationGroup(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID, DWORD dwReserved = 0, DWORD Flags = 0)
	{
		if (m_pSimConnect_RequestNotificationGroup)
			return m_pSimConnect_RequestNotificationGroup(m_hSimConnect, GroupID, dwReserved, Flags);
		return E_POINTER;
	}
	HRESULT AddToDataDefinition(SIMCONNECT_DATA_DEFINITION_ID DefineID, const char * DatumName, const char * UnitsName, SIMCONNECT_DATATYPE DatumType = SIMCONNECT_DATATYPE_FLOAT64, float fEpsilon = 0, DWORD DatumID = SIMCONNECT_UNUSED)
	{
		if (m_pSimConnect_AddToDataDefinition)
			return m_pSimConnect_AddToDataDefinition(m_hSimConnect, DefineID, DatumName, UnitsName, DatumType, fEpsilon, DatumID);
		return E_POINTER;
	}
	HRESULT ClearDataDefinition(SIMCONNECT_DATA_DEFINITION_ID DefineID)
	{
		if (m_pSimConnect_ClearDataDefinition)
			return m_pSimConnect_ClearDataDefinition(m_hSimConnect, DefineID);
		return E_POINTER;
	}
	HRESULT RequestDataOnSimObject(SIMCONNECT_DATA_REQUEST_ID RequestID, SIMCONNECT_DATA_DEFINITION_ID DefineID, SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_PERIOD Period, SIMCONNECT_DATA_REQUEST_FLAG Flags = 0, DWORD origin = 0, DWORD interval = 0, DWORD limit = 0)
	{
		if (m_pSimConnect_RequestDataOnSimObject)
			return m_pSimConnect_RequestDataOnSimObject(m_hSimConnect, RequestID, DefineID, ObjectID, Period, Flags, origin, interval, limit);
		return E_POINTER;
	}
	HRESULT RequestDataOnSimObjectType(SIMCONNECT_DATA_REQUEST_ID RequestID, SIMCONNECT_DATA_DEFINITION_ID DefineID, DWORD dwRadiusMeters, SIMCONNECT_SIMOBJECT_TYPE type)
	{
		if (m_pSimConnect_RequestDataOnSimObjectType)
			return m_pSimConnect_RequestDataOnSimObjectType(m_hSimConnect, RequestID, DefineID, dwRadiusMeters, type);
		return E_POINTER;
	}
	HRESULT SetDataOnSimObject(SIMCONNECT_DATA_DEFINITION_ID DefineID, SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_DATA_SET_FLAG Flags, DWORD ArrayCount, DWORD cbUnitSize, void * pDataSet)
	{
		if (m_pSimConnect_SetDataOnSimObject)
			return m_pSimConnect_SetDataOnSimObject(m_hSimConnect, DefineID, ObjectID, Flags, ArrayCount, cbUnitSize, pDataSet);
		return E_POINTER;
	}
	HRESULT MapInputEventToClientEvent(SIMCONNECT_INPUT_GROUP_ID GroupID, const char * szInputDefinition, SIMCONNECT_CLIENT_EVENT_ID DownEventID, DWORD DownValue = 0, SIMCONNECT_CLIENT_EVENT_ID UpEventID = (SIMCONNECT_CLIENT_EVENT_ID)SIMCONNECT_UNUSED, DWORD UpValue = 0, BOOL bMaskable = FALSE)
	{
		if (m_pSimConnect_MapInputEventToClientEvent)
			return m_pSimConnect_MapInputEventToClientEvent(m_hSimConnect, GroupID, szInputDefinition, DownEventID, DownValue, UpEventID, UpValue, bMaskable);
		return E_POINTER;
	}
	HRESULT SetInputGroupPriority(SIMCONNECT_INPUT_GROUP_ID GroupID, DWORD uPriority)
	{
		if (m_pSimConnect_SetInputGroupPriority)
			return m_pSimConnect_SetInputGroupPriority(m_hSimConnect, GroupID, uPriority);
		return E_POINTER;
	}
	HRESULT RemoveInputEvent(SIMCONNECT_INPUT_GROUP_ID GroupID, const char * szInputDefinition)
	{
		if (m_pSimConnect_RemoveInputEvent)
			return m_pSimConnect_RemoveInputEvent(m_hSimConnect, GroupID, szInputDefinition);
		return E_POINTER;
	}
	HRESULT ClearInputGroup(SIMCONNECT_INPUT_GROUP_ID GroupID)
	{
		if (m_pSimConnect_ClearInputGroup)
			return m_pSimConnect_ClearInputGroup(m_hSimConnect, GroupID);
		return E_POINTER;
	}
	HRESULT SetInputGroupState(SIMCONNECT_INPUT_GROUP_ID GroupID, DWORD dwState)
	{
		if (m_pSimConnect_SetInputGroupState)
			return m_pSimConnect_SetInputGroupState(m_hSimConnect, GroupID, dwState);
		return E_POINTER;
	}
	HRESULT RequestReservedKey(SIMCONNECT_CLIENT_EVENT_ID EventID, const char * szKeyChoice1 = "", const char * szKeyChoice2 = "", const char * szKeyChoice3 = "")
	{
		if (m_pSimConnect_RequestReservedKey)
			return m_pSimConnect_RequestReservedKey(m_hSimConnect, EventID, szKeyChoice1, szKeyChoice2, szKeyChoice3);
		return E_POINTER;
	}
	HRESULT SubscribeToSystemEvent(SIMCONNECT_CLIENT_EVENT_ID EventID, const char * SystemEventName)
	{
		if (m_pSimConnect_SubscribeToSystemEvent)
			return m_pSimConnect_SubscribeToSystemEvent(m_hSimConnect, EventID, SystemEventName);
		return E_POINTER;
	}
	HRESULT UnsubscribeFromSystemEvent(SIMCONNECT_CLIENT_EVENT_ID EventID)
	{
		if (m_pSimConnect_UnsubscribeFromSystemEvent)
			return m_pSimConnect_UnsubscribeFromSystemEvent(m_hSimConnect, EventID);
		return E_POINTER;
	}
	HRESULT WeatherRequestInterpolatedObservation(SIMCONNECT_DATA_REQUEST_ID RequestID, float lat, float lon, float alt)
	{
		if (m_pSimConnect_WeatherRequestInterpolatedObservation)
			return m_pSimConnect_WeatherRequestInterpolatedObservation(m_hSimConnect, RequestID, lat, lon, alt);
		return E_POINTER;
	}
	HRESULT WeatherRequestObservationAtStation(SIMCONNECT_DATA_REQUEST_ID RequestID, const char * szICAO)
	{
		if (m_pSimConnect_WeatherRequestObservationAtStation)
			return m_pSimConnect_WeatherRequestObservationAtStation(m_hSimConnect, RequestID, szICAO);
		return E_POINTER;
	}
	HRESULT WeatherRequestObservationAtNearestStation(SIMCONNECT_DATA_REQUEST_ID RequestID, float lat, float lon)
	{
		if (m_pSimConnect_WeatherRequestObservationAtNearestStation)
			return m_pSimConnect_WeatherRequestObservationAtNearestStation(m_hSimConnect, RequestID, lat, lon);
		return E_POINTER;
	}
	HRESULT WeatherCreateStation(SIMCONNECT_DATA_REQUEST_ID RequestID, const char * szICAO, const char * szName, float lat, float lon, float alt)
	{
		if (m_pSimConnect_WeatherCreateStation)
			return m_pSimConnect_WeatherCreateStation(m_hSimConnect, RequestID, szICAO, szName, lat, lon, alt);
		return E_POINTER;
	}
	HRESULT WeatherRemoveStation(SIMCONNECT_DATA_REQUEST_ID RequestID, const char * szICAO)
	{
		if (m_pSimConnect_WeatherRemoveStation)
			return m_pSimConnect_WeatherRemoveStation(m_hSimConnect, RequestID, szICAO);
		return E_POINTER;
	}
	HRESULT WeatherSetObservation(DWORD Seconds, const char * szMETAR)
	{
		if (m_pSimConnect_WeatherSetObservation)
			return m_pSimConnect_WeatherSetObservation(m_hSimConnect, Seconds, szMETAR);
		return E_POINTER;
	}
	HRESULT WeatherSetModeServer(DWORD dwPort, DWORD dwSeconds)
	{
		if (m_pSimConnect_WeatherSetModeServer)
			return m_pSimConnect_WeatherSetModeServer(m_hSimConnect, dwPort, dwSeconds);
		return E_POINTER;
	}
	HRESULT WeatherSetModeTheme(const char * szThemeName)
	{
		if (m_pSimConnect_WeatherSetModeTheme)
			return m_pSimConnect_WeatherSetModeTheme(m_hSimConnect, szThemeName);
		return E_POINTER;
	}
	HRESULT WeatherSetModeGlobal()
	{
		if (m_pSimConnect_WeatherSetModeGlobal)
			return m_pSimConnect_WeatherSetModeGlobal(m_hSimConnect);
		return E_POINTER;
	}
	HRESULT WeatherSetModeCustom()
	{
		if (m_pSimConnect_WeatherSetModeCustom)
			return m_pSimConnect_WeatherSetModeCustom(m_hSimConnect);
		return E_POINTER;
	}
	HRESULT WeatherSetDynamicUpdateRate(DWORD dwRate)
	{
		if (m_pSimConnect_WeatherSetDynamicUpdateRate)
			return m_pSimConnect_WeatherSetDynamicUpdateRate(m_hSimConnect, dwRate);
		return E_POINTER;
	}
	HRESULT WeatherRequestCloudState(SIMCONNECT_DATA_REQUEST_ID RequestID, float minLat, float minLon, float minAlt, float maxLat, float maxLon, float maxAlt, DWORD dwFlags = 0)
	{
		if (m_pSimConnect_WeatherRequestCloudState)
			return m_pSimConnect_WeatherRequestCloudState(m_hSimConnect, RequestID, minLat, minLon, minAlt, maxLat, maxLon, maxAlt, dwFlags);
		return E_POINTER;
	}
	HRESULT WeatherCreateThermal(SIMCONNECT_DATA_REQUEST_ID RequestID, float lat, float lon, float alt, float radius, float height, float coreRate = 3.0f, float coreTurbulence = 0.05f, float sinkRate = 3.0f, float sinkTurbulence = 0.2f, float coreSize = 0.4f, float coreTransitionSize = 0.1f, float sinkLayerSize = 0.4f, float sinkTransitionSize = 0.1f)
	{
		if (m_pSimConnect_WeatherCreateThermal)
			return m_pSimConnect_WeatherCreateThermal(m_hSimConnect, RequestID, lat, lon, alt, radius, height, coreRate, coreTurbulence, sinkRate, sinkTurbulence, coreSize, coreTransitionSize, sinkLayerSize, sinkTransitionSize);
		return E_POINTER;
	}
	HRESULT WeatherRemoveThermal(SIMCONNECT_OBJECT_ID ObjectID)
	{
		if (m_pSimConnect_WeatherRemoveThermal)
			return m_pSimConnect_WeatherRemoveThermal(m_hSimConnect, ObjectID);
		return E_POINTER;
	}
	HRESULT AICreateParkedATCAircraft(const char * szContainerTitle, const char * szTailNumber, const char * szAirportID, SIMCONNECT_DATA_REQUEST_ID RequestID)
	{
		if (m_pSimConnect_AICreateParkedATCAircraft)
			return m_pSimConnect_AICreateParkedATCAircraft(m_hSimConnect, szContainerTitle, szTailNumber, szAirportID, RequestID);
		return E_POINTER;
	}
	HRESULT AICreateEnrouteATCAircraft(const char * szContainerTitle, const char * szTailNumber, int iFlightNumber, const char * szFlightPlanPath, double dFlightPlanPosition, BOOL bTouchAndGo, SIMCONNECT_DATA_REQUEST_ID RequestID)
	{
		if (m_pSimConnect_AICreateEnrouteATCAircraft)
			return m_pSimConnect_AICreateEnrouteATCAircraft(m_hSimConnect, szContainerTitle, szTailNumber, iFlightNumber, szFlightPlanPath, dFlightPlanPosition, bTouchAndGo, RequestID);
		return E_POINTER;
	}
	HRESULT AICreateNonATCAircraft(const char * szContainerTitle, const char * szTailNumber, SIMCONNECT_DATA_INITPOSITION InitPos, SIMCONNECT_DATA_REQUEST_ID RequestID)
	{
		if (m_pSimConnect_AICreateNonATCAircraft)
			return m_pSimConnect_AICreateNonATCAircraft(m_hSimConnect, szContainerTitle, szTailNumber, InitPos, RequestID);
		return E_POINTER;
	}
	HRESULT AICreateSimulatedObject(const char * szContainerTitle, SIMCONNECT_DATA_INITPOSITION InitPos, SIMCONNECT_DATA_REQUEST_ID RequestID)
	{
		if (m_pSimConnect_AICreateSimulatedObject)
			return m_pSimConnect_AICreateSimulatedObject(m_hSimConnect, szContainerTitle, InitPos, RequestID);
		return E_POINTER;
	}
	HRESULT AIReleaseControl(SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_DATA_REQUEST_ID RequestID)
	{
		if (m_pSimConnect_AIReleaseControl)
			return m_pSimConnect_AIReleaseControl(m_hSimConnect, ObjectID, RequestID);
		return E_POINTER;
	}
	HRESULT AIRemoveObject(SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_DATA_REQUEST_ID RequestID)
	{
		if (m_pSimConnect_AIRemoveObject)
			return m_pSimConnect_AIRemoveObject(m_hSimConnect, ObjectID, RequestID);
		return E_POINTER;
	}
	HRESULT AISetAircraftFlightPlan(SIMCONNECT_OBJECT_ID ObjectID, const char * szFlightPlanPath, SIMCONNECT_DATA_REQUEST_ID RequestID)
	{
		if (m_pSimConnect_AISetAircraftFlightPlan)
			return m_pSimConnect_AISetAircraftFlightPlan(m_hSimConnect, ObjectID, szFlightPlanPath, RequestID);
		return E_POINTER;
	}
	HRESULT ExecuteMissionAction(const GUID guidInstanceId)
	{
		if (m_pSimConnect_ExecuteMissionAction)
			return m_pSimConnect_ExecuteMissionAction(m_hSimConnect, guidInstanceId);
		return E_POINTER;
	}
	HRESULT CompleteCustomMissionAction(const GUID guidInstanceId)
	{
		if (m_pSimConnect_CompleteCustomMissionAction)
			return m_pSimConnect_CompleteCustomMissionAction(m_hSimConnect, guidInstanceId);
		return E_POINTER;
	}
	HRESULT Close()
	{
		if (!m_bInitialized)
			return S_OK;

		if (m_pSimConnect_Close)
		{
			HRESULT hr = m_pSimConnect_Close(m_hSimConnect);
			if (SUCCEEDED(hr))
			{
				FreeLibrary(m_hSimConnectDLL);
				m_hSimConnect = NULL;
				m_hSimConnectDLL = NULL;
				m_bInitialized = false;
				m_pSimConnect_MapClientEventToSimEvent = nullptr;
				m_pSimConnect_TransmitClientEvent = nullptr;
				m_pSimConnect_SetSystemEventState = nullptr;
				m_pSimConnect_AddClientEventToNotificationGroup = nullptr;
				m_pSimConnect_RemoveClientEvent = nullptr;
				m_pSimConnect_SetNotificationGroupPriority = nullptr;
				m_pSimConnect_ClearNotificationGroup = nullptr;
				m_pSimConnect_RequestNotificationGroup = nullptr;
				m_pSimConnect_AddToDataDefinition = nullptr;
				m_pSimConnect_ClearDataDefinition = nullptr;
				m_pSimConnect_RequestDataOnSimObject = nullptr;
				m_pSimConnect_RequestDataOnSimObjectType = nullptr;
				m_pSimConnect_SetDataOnSimObject = nullptr;
				m_pSimConnect_MapInputEventToClientEvent = nullptr;
				m_pSimConnect_SetInputGroupPriority = nullptr;
				m_pSimConnect_RemoveInputEvent = nullptr;
				m_pSimConnect_ClearInputGroup = nullptr;
				m_pSimConnect_SetInputGroupState = nullptr;
				m_pSimConnect_RequestReservedKey = nullptr;
				m_pSimConnect_SubscribeToSystemEvent = nullptr;
				m_pSimConnect_UnsubscribeFromSystemEvent = nullptr;
				m_pSimConnect_WeatherRequestInterpolatedObservation = nullptr;
				m_pSimConnect_WeatherRequestObservationAtStation = nullptr;
				m_pSimConnect_WeatherRequestObservationAtNearestStation = nullptr;
				m_pSimConnect_WeatherCreateStation = nullptr;
				m_pSimConnect_WeatherRemoveStation = nullptr;
				m_pSimConnect_WeatherSetObservation = nullptr;
				m_pSimConnect_WeatherSetModeServer = nullptr;
				m_pSimConnect_WeatherSetModeTheme = nullptr;
				m_pSimConnect_WeatherSetModeGlobal = nullptr;
				m_pSimConnect_WeatherSetModeCustom = nullptr;
				m_pSimConnect_WeatherSetDynamicUpdateRate = nullptr;
				m_pSimConnect_WeatherRequestCloudState = nullptr;
				m_pSimConnect_WeatherCreateThermal = nullptr;
				m_pSimConnect_WeatherRemoveThermal = nullptr;
				m_pSimConnect_AICreateParkedATCAircraft = nullptr;
				m_pSimConnect_AICreateEnrouteATCAircraft = nullptr;
				m_pSimConnect_AICreateNonATCAircraft = nullptr;
				m_pSimConnect_AICreateSimulatedObject = nullptr;
				m_pSimConnect_AIReleaseControl = nullptr;
				m_pSimConnect_AIRemoveObject = nullptr;
				m_pSimConnect_AISetAircraftFlightPlan = nullptr;
				m_pSimConnect_ExecuteMissionAction = nullptr;
				m_pSimConnect_CompleteCustomMissionAction = nullptr;
				m_pSimConnect_Close = nullptr;
				m_pSimConnect_RetrieveString = nullptr;
				m_pSimConnect_GetLastSentPacketID = nullptr;
				m_pSimConnect_Open = nullptr;
				m_pSimConnect_CallDispatch = nullptr;
				m_pSimConnect_GetNextDispatch = nullptr;
				m_pSimConnect_RequestResponseTimes = nullptr;
				m_pSimConnect_InsertString = nullptr;
				m_pSimConnect_CameraSetRelative6DOF = nullptr;
				m_pSimConnect_MenuAddItem = nullptr;
				m_pSimConnect_MenuDeleteItem = nullptr;
				m_pSimConnect_MenuAddSubItem = nullptr;
				m_pSimConnect_MenuDeleteSubItem = nullptr;
				m_pSimConnect_RequestSystemState = nullptr;
				m_pSimConnect_SetSystemState = nullptr;
				m_pSimConnect_MapClientDataNameToID = nullptr;
				m_pSimConnect_CreateClientData = nullptr;
				m_pSimConnect_AddToClientDataDefinition = nullptr;
				m_pSimConnect_ClearClientDataDefinition = nullptr;
				m_pSimConnect_RequestClientData = nullptr;
				m_pSimConnect_SetClientData = nullptr;
				m_pSimConnect_FlightLoad = nullptr;
				m_pSimConnect_FlightSave = nullptr;
				m_pSimConnect_FlightPlanLoad = nullptr;
				m_pSimConnect_Text = nullptr;
				m_pSimConnect_SubscribeToFacilities = nullptr;
				m_pSimConnect_UnsubscribeToFacilities = nullptr;
				m_pSimConnect_RequestFacilitiesList = nullptr;
			}
			return hr;
		}
		return E_POINTER;
	}
	HRESULT RetrieveString(SIMCONNECT_RECV * pData, DWORD cbData, void * pStringV, char ** pszString, DWORD * pcbString)
	{
		if (m_pSimConnect_RetrieveString)
			return m_pSimConnect_RetrieveString(pData, cbData, pStringV, pszString, pcbString);
		return E_POINTER;
	}
	HRESULT GetLastSentPacketID(DWORD * pdwError)
	{
		if (m_pSimConnect_GetLastSentPacketID)
			return m_pSimConnect_GetLastSentPacketID(m_hSimConnect, pdwError);
		return E_POINTER;
	}
	HRESULT Open(LPCSTR szName, HWND hWnd, DWORD UserEventWin32, HANDLE hEventHandle, DWORD ConfigIndex)
	{
		if (m_bInitialized)
			return S_OK;
		if (m_pSimConnect_Open)
			return m_pSimConnect_Open(&m_hSimConnect, szName, hWnd, UserEventWin32, hEventHandle, ConfigIndex);
		return E_POINTER;
	}
	HRESULT CallDispatch(DispatchProc pfcnDispatch, void * pContext)
	{
		if (m_pSimConnect_CallDispatch)
			return m_pSimConnect_CallDispatch(m_hSimConnect, pfcnDispatch, pContext);
		return E_POINTER;
	}
	HRESULT GetNextDispatch(SIMCONNECT_RECV ** ppData, DWORD * pcbData)
	{
		if (m_pSimConnect_GetNextDispatch)
			return m_pSimConnect_GetNextDispatch(m_hSimConnect, ppData, pcbData);
		return E_POINTER;
	}
	HRESULT RequestResponseTimes(DWORD nCount, float * fElapsedSeconds)
	{
		if (m_pSimConnect_RequestResponseTimes)
			return m_pSimConnect_RequestResponseTimes(m_hSimConnect, nCount, fElapsedSeconds);
		return E_POINTER;
	}
	HRESULT InsertString(char * pDest, DWORD cbDest, void ** ppEnd, DWORD * pcbStringV, const char * pSource)
	{
		if (m_pSimConnect_InsertString)
			return m_pSimConnect_InsertString(pDest, cbDest, ppEnd, pcbStringV, pSource);
		return E_POINTER;
	}
	HRESULT CameraSetRelative6DOF(float fDeltaX, float fDeltaY, float fDeltaZ, float fPitchDeg, float fBankDeg, float fHeadingDeg)
	{
		if (m_pSimConnect_CameraSetRelative6DOF)
			return m_pSimConnect_CameraSetRelative6DOF(m_hSimConnect, fDeltaX, fDeltaY, fDeltaZ, fPitchDeg, fBankDeg, fHeadingDeg);
		return E_POINTER;
	}
	HRESULT MenuAddItem(const char * szMenuItem, SIMCONNECT_CLIENT_EVENT_ID MenuEventID, DWORD dwData)
	{
		if (m_pSimConnect_MenuAddItem)
			return m_pSimConnect_MenuAddItem(m_hSimConnect, szMenuItem, MenuEventID, dwData);
		return E_POINTER;
	}
	HRESULT MenuDeleteItem(SIMCONNECT_CLIENT_EVENT_ID MenuEventID)
	{
		if (m_pSimConnect_MenuDeleteItem)
			return m_pSimConnect_MenuDeleteItem(m_hSimConnect, MenuEventID);
		return E_POINTER;
	}
	HRESULT MenuAddSubItem(SIMCONNECT_CLIENT_EVENT_ID MenuEventID, const char * szMenuItem, SIMCONNECT_CLIENT_EVENT_ID SubMenuEventID, DWORD dwData)
	{
		if (m_pSimConnect_MenuAddSubItem)
			return m_pSimConnect_MenuAddSubItem(m_hSimConnect, MenuEventID, szMenuItem, SubMenuEventID, dwData);
		return E_POINTER;
	}
	HRESULT MenuDeleteSubItem(SIMCONNECT_CLIENT_EVENT_ID MenuEventID, const SIMCONNECT_CLIENT_EVENT_ID SubMenuEventID)
	{
		if (m_pSimConnect_MenuDeleteSubItem)
			return m_pSimConnect_MenuDeleteSubItem(m_hSimConnect, MenuEventID, SubMenuEventID);
		return E_POINTER;
	}
	HRESULT RequestSystemState(SIMCONNECT_DATA_REQUEST_ID RequestID, const char * szState)
	{
		if (m_pSimConnect_RequestSystemState)
			return m_pSimConnect_RequestSystemState(m_hSimConnect, RequestID, szState);
		return E_POINTER;
	}
	HRESULT SetSystemState(const char * szState, DWORD dwInteger, float fFloat, const char * szString)
	{
		if (m_pSimConnect_SetSystemState)
			return m_pSimConnect_SetSystemState(m_hSimConnect, szState, dwInteger, fFloat, szString);
		return E_POINTER;
	}
	HRESULT MapClientDataNameToID(const char * szClientDataName, SIMCONNECT_CLIENT_DATA_ID ClientDataID)
	{
		if (m_pSimConnect_MapClientDataNameToID)
			return m_pSimConnect_MapClientDataNameToID(m_hSimConnect, szClientDataName, ClientDataID);
		return E_POINTER;
	}
	HRESULT CreateClientData(SIMCONNECT_CLIENT_DATA_ID ClientDataID, DWORD dwSize, SIMCONNECT_CREATE_CLIENT_DATA_FLAG Flags)
	{
		if (m_pSimConnect_CreateClientData)
			return m_pSimConnect_CreateClientData(m_hSimConnect, ClientDataID, dwSize, Flags);
		return E_POINTER;
	}
	HRESULT AddToClientDataDefinition(SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID, DWORD dwOffset, DWORD dwSizeOrType, float fEpsilon = 0, DWORD DatumID = SIMCONNECT_UNUSED)
	{
		if (m_pSimConnect_AddToClientDataDefinition)
			return m_pSimConnect_AddToClientDataDefinition(m_hSimConnect, DefineID, dwOffset, dwSizeOrType, fEpsilon, DatumID);
		return E_POINTER;
	}
	HRESULT ClearClientDataDefinition(SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID)
	{
		if (m_pSimConnect_ClearClientDataDefinition)
			return m_pSimConnect_ClearClientDataDefinition(m_hSimConnect, DefineID);
		return E_POINTER;
	}
	HRESULT RequestClientData(SIMCONNECT_CLIENT_DATA_ID ClientDataID, SIMCONNECT_DATA_REQUEST_ID RequestID, SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID, SIMCONNECT_CLIENT_DATA_PERIOD Period = SIMCONNECT_CLIENT_DATA_PERIOD_ONCE, SIMCONNECT_CLIENT_DATA_REQUEST_FLAG Flags = 0, DWORD origin = 0, DWORD interval = 0, DWORD limit = 0)
	{
		if (m_pSimConnect_RequestClientData)
			return m_pSimConnect_RequestClientData(m_hSimConnect, ClientDataID, RequestID, DefineID, Period, Flags, origin, interval, limit);
		return E_POINTER;
	}
	HRESULT SetClientData(SIMCONNECT_CLIENT_DATA_ID ClientDataID, SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID, SIMCONNECT_CLIENT_DATA_SET_FLAG Flags, DWORD dwReserved, DWORD cbUnitSize, void * pDataSet)
	{
		if (m_pSimConnect_SetClientData)
			return m_pSimConnect_SetClientData(m_hSimConnect, ClientDataID, DefineID, Flags, dwReserved, cbUnitSize, pDataSet);
		return E_POINTER;
	}
	HRESULT FlightLoad(const char * szFileName)
	{
		if (m_pSimConnect_FlightLoad)
			return m_pSimConnect_FlightLoad(m_hSimConnect, szFileName);
		return E_POINTER;
	}
	HRESULT FlightSave(const char * szFileName, const char * szTitle, const char * szDescription, DWORD Flags)
	{
		if (m_pSimConnect_FlightSave)
			return m_pSimConnect_FlightSave(m_hSimConnect, szFileName, szTitle, szDescription, Flags);
		return E_POINTER;
	}
	HRESULT FlightPlanLoad(const char * szFileName)
	{
		if (m_pSimConnect_FlightPlanLoad)
			return m_pSimConnect_FlightPlanLoad(m_hSimConnect, szFileName);
		return E_POINTER;
	}
	HRESULT Text(SIMCONNECT_TEXT_TYPE type, float fTimeSeconds, SIMCONNECT_CLIENT_EVENT_ID EventID, DWORD cbUnitSize, void * pDataSet)
	{
		if (m_pSimConnect_Text)
			return m_pSimConnect_Text(m_hSimConnect, type, fTimeSeconds, EventID, cbUnitSize, pDataSet);
		return E_POINTER;
	}
	HRESULT SubscribeToFacilities(SIMCONNECT_FACILITY_LIST_TYPE type, SIMCONNECT_DATA_REQUEST_ID RequestID)
	{
		if (m_pSimConnect_SubscribeToFacilities)
			return m_pSimConnect_SubscribeToFacilities(m_hSimConnect, type, RequestID);
		return E_POINTER;
	}
	HRESULT UnsubscribeToFacilities(SIMCONNECT_FACILITY_LIST_TYPE type)
	{
		if (m_pSimConnect_UnsubscribeToFacilities)
			return m_pSimConnect_UnsubscribeToFacilities(m_hSimConnect, type);
		return E_POINTER;
	}
	HRESULT RequestFacilitiesList(SIMCONNECT_FACILITY_LIST_TYPE type, SIMCONNECT_DATA_REQUEST_ID RequestID)
	{
		if (m_pSimConnect_RequestFacilitiesList)
			return m_pSimConnect_RequestFacilitiesList(m_hSimConnect, type, RequestID);
		return E_POINTER;
	}

protected:

	bool LoadDLL()
	{
		m_hSimConnectDLL = LoadLibrary(L"SimConnect.dll");
		if (!m_hSimConnectDLL)
			return false; //couldn't find SimConnect

		m_pSimConnect_MapClientEventToSimEvent = (t_SimConnect_MapClientEventToSimEvent)GetProcAddress(m_hSimConnectDLL, "SimConnect_MapClientEventToSimEvent");
		m_pSimConnect_TransmitClientEvent = (t_SimConnect_TransmitClientEvent)GetProcAddress(m_hSimConnectDLL, "SimConnect_TransmitClientEvent");
		m_pSimConnect_SetSystemEventState = (t_SimConnect_SetSystemEventState)GetProcAddress(m_hSimConnectDLL, "SimConnect_SetSystemEventState");
		m_pSimConnect_AddClientEventToNotificationGroup = (t_SimConnect_AddClientEventToNotificationGroup)GetProcAddress(m_hSimConnectDLL, "SimConnect_AddClientEventToNotificationGroup");
		m_pSimConnect_RemoveClientEvent = (t_SimConnect_RemoveClientEvent)GetProcAddress(m_hSimConnectDLL, "SimConnect_RemoveClientEvent");
		m_pSimConnect_SetNotificationGroupPriority = (t_SimConnect_SetNotificationGroupPriority)GetProcAddress(m_hSimConnectDLL, "SimConnect_SetNotificationGroupPriority");
		m_pSimConnect_ClearNotificationGroup = (t_SimConnect_ClearNotificationGroup)GetProcAddress(m_hSimConnectDLL, "SimConnect_ClearNotificationGroup");
		m_pSimConnect_RequestNotificationGroup = (t_SimConnect_RequestNotificationGroup)GetProcAddress(m_hSimConnectDLL, "SimConnect_RequestNotificationGroup");
		m_pSimConnect_AddToDataDefinition = (t_SimConnect_AddToDataDefinition)GetProcAddress(m_hSimConnectDLL, "SimConnect_AddToDataDefinition");
		m_pSimConnect_ClearDataDefinition = (t_SimConnect_ClearDataDefinition)GetProcAddress(m_hSimConnectDLL, "SimConnect_ClearDataDefinition");
		m_pSimConnect_RequestDataOnSimObject = (t_SimConnect_RequestDataOnSimObject)GetProcAddress(m_hSimConnectDLL, "SimConnect_RequestDataOnSimObject");
		m_pSimConnect_RequestDataOnSimObjectType = (t_SimConnect_RequestDataOnSimObjectType)GetProcAddress(m_hSimConnectDLL, "SimConnect_RequestDataOnSimObjectType");
		m_pSimConnect_SetDataOnSimObject = (t_SimConnect_SetDataOnSimObject)GetProcAddress(m_hSimConnectDLL, "SimConnect_SetDataOnSimObject");
		m_pSimConnect_MapInputEventToClientEvent = (t_SimConnect_MapInputEventToClientEvent)GetProcAddress(m_hSimConnectDLL, "SimConnect_MapInputEventToClientEvent");
		m_pSimConnect_SetInputGroupPriority = (t_SimConnect_SetInputGroupPriority)GetProcAddress(m_hSimConnectDLL, "SimConnect_SetInputGroupPriority");
		m_pSimConnect_RemoveInputEvent = (t_SimConnect_RemoveInputEvent)GetProcAddress(m_hSimConnectDLL, "SimConnect_RemoveInputEvent");
		m_pSimConnect_ClearInputGroup = (t_SimConnect_ClearInputGroup)GetProcAddress(m_hSimConnectDLL, "SimConnect_ClearInputGroup");
		m_pSimConnect_SetInputGroupState = (t_SimConnect_SetInputGroupState)GetProcAddress(m_hSimConnectDLL, "SimConnect_SetInputGroupState");
		m_pSimConnect_RequestReservedKey = (t_SimConnect_RequestReservedKey)GetProcAddress(m_hSimConnectDLL, "SimConnect_RequestReservedKey");
		m_pSimConnect_SubscribeToSystemEvent = (t_SimConnect_SubscribeToSystemEvent)GetProcAddress(m_hSimConnectDLL, "SimConnect_SubscribeToSystemEvent");
		m_pSimConnect_UnsubscribeFromSystemEvent = (t_SimConnect_UnsubscribeFromSystemEvent)GetProcAddress(m_hSimConnectDLL, "SimConnect_UnsubscribeFromSystemEvent");
		m_pSimConnect_WeatherRequestInterpolatedObservation = (t_SimConnect_WeatherRequestInterpolatedObservation)GetProcAddress(m_hSimConnectDLL, "SimConnect_WeatherRequestInterpolatedObservation");
		m_pSimConnect_WeatherRequestObservationAtStation = (t_SimConnect_WeatherRequestObservationAtStation)GetProcAddress(m_hSimConnectDLL, "SimConnect_WeatherRequestObservationAtStation");
		m_pSimConnect_WeatherRequestObservationAtNearestStation = (t_SimConnect_WeatherRequestObservationAtNearestStation)GetProcAddress(m_hSimConnectDLL, "SimConnect_WeatherRequestObservationAtNearestStation");
		m_pSimConnect_WeatherCreateStation = (t_SimConnect_WeatherCreateStation)GetProcAddress(m_hSimConnectDLL, "SimConnect_WeatherCreateStation");
		m_pSimConnect_WeatherRemoveStation = (t_SimConnect_WeatherRemoveStation)GetProcAddress(m_hSimConnectDLL, "SimConnect_WeatherRemoveStation");
		m_pSimConnect_WeatherSetObservation = (t_SimConnect_WeatherSetObservation)GetProcAddress(m_hSimConnectDLL, "SimConnect_WeatherSetObservation");
		m_pSimConnect_WeatherSetModeServer = (t_SimConnect_WeatherSetModeServer)GetProcAddress(m_hSimConnectDLL, "SimConnect_WeatherSetModeServer");
		m_pSimConnect_WeatherSetModeTheme = (t_SimConnect_WeatherSetModeTheme)GetProcAddress(m_hSimConnectDLL, "SimConnect_WeatherSetModeTheme");
		m_pSimConnect_WeatherSetModeGlobal = (t_SimConnect_WeatherSetModeGlobal)GetProcAddress(m_hSimConnectDLL, "SimConnect_WeatherSetModeGlobal");
		m_pSimConnect_WeatherSetModeCustom = (t_SimConnect_WeatherSetModeCustom)GetProcAddress(m_hSimConnectDLL, "SimConnect_WeatherSetModeCustom");
		m_pSimConnect_WeatherSetDynamicUpdateRate = (t_SimConnect_WeatherSetDynamicUpdateRate)GetProcAddress(m_hSimConnectDLL, "SimConnect_WeatherSetDynamicUpdateRate");
		m_pSimConnect_WeatherRequestCloudState = (t_SimConnect_WeatherRequestCloudState)GetProcAddress(m_hSimConnectDLL, "SimConnect_WeatherRequestCloudState");
		m_pSimConnect_WeatherCreateThermal = (t_SimConnect_WeatherCreateThermal)GetProcAddress(m_hSimConnectDLL, "SimConnect_WeatherCreateThermal");
		m_pSimConnect_WeatherRemoveThermal = (t_SimConnect_WeatherRemoveThermal)GetProcAddress(m_hSimConnectDLL, "SimConnect_WeatherRemoveThermal");
		m_pSimConnect_AICreateParkedATCAircraft = (t_SimConnect_AICreateParkedATCAircraft)GetProcAddress(m_hSimConnectDLL, "SimConnect_AICreateParkedATCAircraft");
		m_pSimConnect_AICreateEnrouteATCAircraft = (t_SimConnect_AICreateEnrouteATCAircraft)GetProcAddress(m_hSimConnectDLL, "SimConnect_AICreateEnrouteATCAircraft");
		m_pSimConnect_AICreateNonATCAircraft = (t_SimConnect_AICreateNonATCAircraft)GetProcAddress(m_hSimConnectDLL, "SimConnect_AICreateNonATCAircraft");
		m_pSimConnect_AICreateSimulatedObject = (t_SimConnect_AICreateSimulatedObject)GetProcAddress(m_hSimConnectDLL, "SimConnect_AICreateSimulatedObject");
		m_pSimConnect_AIReleaseControl = (t_SimConnect_AIReleaseControl)GetProcAddress(m_hSimConnectDLL, "SimConnect_AIReleaseControl");
		m_pSimConnect_AIRemoveObject = (t_SimConnect_AIRemoveObject)GetProcAddress(m_hSimConnectDLL, "SimConnect_AIRemoveObject");
		m_pSimConnect_AISetAircraftFlightPlan = (t_SimConnect_AISetAircraftFlightPlan)GetProcAddress(m_hSimConnectDLL, "SimConnect_AISetAircraftFlightPlan");
		m_pSimConnect_ExecuteMissionAction = (t_SimConnect_ExecuteMissionAction)GetProcAddress(m_hSimConnectDLL, "SimConnect_ExecuteMissionAction");
		m_pSimConnect_CompleteCustomMissionAction = (t_SimConnect_CompleteCustomMissionAction)GetProcAddress(m_hSimConnectDLL, "SimConnect_CompleteCustomMissionAction");
		m_pSimConnect_Close = (t_SimConnect_Close)GetProcAddress(m_hSimConnectDLL, "SimConnect_Close");
		m_pSimConnect_RetrieveString = (t_SimConnect_RetrieveString)GetProcAddress(m_hSimConnectDLL, "SimConnect_RetrieveString");
		m_pSimConnect_GetLastSentPacketID = (t_SimConnect_GetLastSentPacketID)GetProcAddress(m_hSimConnectDLL, "SimConnect_GetLastSentPacketID");
		m_pSimConnect_Open = (t_SimConnect_Open)GetProcAddress(m_hSimConnectDLL, "SimConnect_Open");
		m_pSimConnect_CallDispatch = (t_SimConnect_CallDispatch)GetProcAddress(m_hSimConnectDLL, "SimConnect_CallDispatch");
		m_pSimConnect_GetNextDispatch = (t_SimConnect_GetNextDispatch)GetProcAddress(m_hSimConnectDLL, "SimConnect_GetNextDispatch");
		m_pSimConnect_RequestResponseTimes = (t_SimConnect_RequestResponseTimes)GetProcAddress(m_hSimConnectDLL, "SimConnect_RequestResponseTimes");
		m_pSimConnect_InsertString = (t_SimConnect_InsertString)GetProcAddress(m_hSimConnectDLL, "SimConnect_InsertString");
		m_pSimConnect_CameraSetRelative6DOF = (t_SimConnect_CameraSetRelative6DOF)GetProcAddress(m_hSimConnectDLL, "SimConnect_CameraSetRelative6DOF");
		m_pSimConnect_MenuAddItem = (t_SimConnect_MenuAddItem)GetProcAddress(m_hSimConnectDLL, "SimConnect_MenuAddItem");
		m_pSimConnect_MenuDeleteItem = (t_SimConnect_MenuDeleteItem)GetProcAddress(m_hSimConnectDLL, "SimConnect_MenuDeleteItem");
		m_pSimConnect_MenuAddSubItem = (t_SimConnect_MenuAddSubItem)GetProcAddress(m_hSimConnectDLL, "SimConnect_MenuAddSubItem");
		m_pSimConnect_MenuDeleteSubItem = (t_SimConnect_MenuDeleteSubItem)GetProcAddress(m_hSimConnectDLL, "SimConnect_MenuDeleteSubItem");
		m_pSimConnect_RequestSystemState = (t_SimConnect_RequestSystemState)GetProcAddress(m_hSimConnectDLL, "SimConnect_RequestSystemState");
		m_pSimConnect_SetSystemState = (t_SimConnect_SetSystemState)GetProcAddress(m_hSimConnectDLL, "SimConnect_SetSystemState");
		m_pSimConnect_MapClientDataNameToID = (t_SimConnect_MapClientDataNameToID)GetProcAddress(m_hSimConnectDLL, "SimConnect_MapClientDataNameToID");
		m_pSimConnect_CreateClientData = (t_SimConnect_CreateClientData)GetProcAddress(m_hSimConnectDLL, "SimConnect_CreateClientData");
		m_pSimConnect_AddToClientDataDefinition = (t_SimConnect_AddToClientDataDefinition)GetProcAddress(m_hSimConnectDLL, "SimConnect_AddToClientDataDefinition");
		m_pSimConnect_ClearClientDataDefinition = (t_SimConnect_ClearClientDataDefinition)GetProcAddress(m_hSimConnectDLL, "SimConnect_ClearClientDataDefinition");
		m_pSimConnect_RequestClientData = (t_SimConnect_RequestClientData)GetProcAddress(m_hSimConnectDLL, "SimConnect_RequestClientData");
		m_pSimConnect_SetClientData = (t_SimConnect_SetClientData)GetProcAddress(m_hSimConnectDLL, "SimConnect_SetClientData");
		m_pSimConnect_FlightLoad = (t_SimConnect_FlightLoad)GetProcAddress(m_hSimConnectDLL, "SimConnect_FlightLoad");
		m_pSimConnect_FlightSave = (t_SimConnect_FlightSave)GetProcAddress(m_hSimConnectDLL, "SimConnect_FlightSave");
		m_pSimConnect_FlightPlanLoad = (t_SimConnect_FlightPlanLoad)GetProcAddress(m_hSimConnectDLL, "SimConnect_FlightPlanLoad");
		m_pSimConnect_Text = (t_SimConnect_Text)GetProcAddress(m_hSimConnectDLL, "SimConnect_Text");
		m_pSimConnect_SubscribeToFacilities = (t_SimConnect_SubscribeToFacilities)GetProcAddress(m_hSimConnectDLL, "SimConnect_SubscribeToFacilities");
		m_pSimConnect_UnsubscribeToFacilities = (t_SimConnect_UnsubscribeToFacilities)GetProcAddress(m_hSimConnectDLL, "SimConnect_UnsubscribeToFacilities");
		m_pSimConnect_RequestFacilitiesList = (t_SimConnect_RequestFacilitiesList)GetProcAddress(m_hSimConnectDLL, "SimConnect_RequestFacilitiesList");

		return true;

		/* use this if manifest problems...
		static LPCTSTR _manifest_name_lut[] = { L"SIMC_SP1_MANIFEST", L"SIMC_RTM_MANIFEST", 0 };

		// store the module name, used to open the SimConnect interface
		TCHAR* szModule = new TCHAR[_MAX_PATH];
		GetModuleFileName(hInst, szModule, MAX_PATH);
		ModuleName = szModule;
		delete szModule;

		// try to load the latest SimConnect version
		long manifestidx = 0;
		while (!Valid && _manifest_name_lut[manifestidx])
		{
		ACTCTX act = { 0 };
		HANDLE hctx;
		act.cbSize = sizeof(act);
		act.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID; //|ACTCTX_FLAG_HMODULE_VALID;
		act.lpSource = ModuleName.c_str(); // seems it does not work with hInst only so use the module filename
		act.lpResourceName = _manifest_name_lut[manifestidx];
		act.hModule = hInst;
		hctx = CreateActCtx (&act);
		if (hctx != INVALID_HANDLE_VALUE)
		{
		ULONG_PTR ulCookie = 0;
		if (ActivateActCtx(hctx, &ulCookie))
		{
		m_hSimConnectDLL = LoadLibrary(L"SimConnect.dll");
		if (m_hSimConnectDLL)
		{
		pSimConnect_Open = (t_SimConnect_Open)GetProcAddress(m_hSimConnectDLL,"SimConnect_Open");
		pSimConnect_Close = (t_SimConnect_Close)GetProcAddress(m_hSimConnectDLL,"SimConnect_Close");
		pSimConnect_SubscribeToSystemEvent = (t_SimConnect_SubscribeToSystemEvent)GetProcAddress(m_hSimConnectDLL,"SimConnect_SubscribeToSystemEvent");
		pSimConnect_CallDispatch = (t_SimConnect_CallDispatch)GetProcAddress(m_hSimConnectDLL,"SimConnect_CallDispatch");
		Valid = (SUCCEEDED(Open(ModuleName.c_str()))) ? 1 : 0;
		}
		DeactivateActCtx(0, ulCookie);
		}
		ReleaseActCtx(hctx);
		}
		manifestidx++;
		}
		return (Valid) ? true : false;
		*/
	}

	HMODULE m_hSimConnectDLL;
	HANDLE m_hSimConnect;
	bool m_bInitialized;

	t_SimConnect_MapClientEventToSimEvent m_pSimConnect_MapClientEventToSimEvent;
	t_SimConnect_TransmitClientEvent m_pSimConnect_TransmitClientEvent;
	t_SimConnect_SetSystemEventState m_pSimConnect_SetSystemEventState;
	t_SimConnect_AddClientEventToNotificationGroup 	m_pSimConnect_AddClientEventToNotificationGroup;
	t_SimConnect_RemoveClientEvent m_pSimConnect_RemoveClientEvent;
	t_SimConnect_SetNotificationGroupPriority m_pSimConnect_SetNotificationGroupPriority;
	t_SimConnect_ClearNotificationGroup m_pSimConnect_ClearNotificationGroup;
	t_SimConnect_RequestNotificationGroup m_pSimConnect_RequestNotificationGroup;
	t_SimConnect_AddToDataDefinition m_pSimConnect_AddToDataDefinition;
	t_SimConnect_ClearDataDefinition m_pSimConnect_ClearDataDefinition;
	t_SimConnect_RequestDataOnSimObject m_pSimConnect_RequestDataOnSimObject;
	t_SimConnect_RequestDataOnSimObjectType m_pSimConnect_RequestDataOnSimObjectType;
	t_SimConnect_SetDataOnSimObject	m_pSimConnect_SetDataOnSimObject;
	t_SimConnect_MapInputEventToClientEvent m_pSimConnect_MapInputEventToClientEvent;
	t_SimConnect_SetInputGroupPriority m_pSimConnect_SetInputGroupPriority;
	t_SimConnect_RemoveInputEvent SimConnect_RemoveInputEvent;
	t_SimConnect_RemoveInputEvent m_pSimConnect_RemoveInputEvent;
	t_SimConnect_ClearInputGroup m_pSimConnect_ClearInputGroup;
	t_SimConnect_SetInputGroupState m_pSimConnect_SetInputGroupState;
	t_SimConnect_RequestReservedKey m_pSimConnect_RequestReservedKey;
	t_SimConnect_SubscribeToSystemEvent m_pSimConnect_SubscribeToSystemEvent;
	t_SimConnect_UnsubscribeFromSystemEvent m_pSimConnect_UnsubscribeFromSystemEvent;
	t_SimConnect_WeatherRequestInterpolatedObservation m_pSimConnect_WeatherRequestInterpolatedObservation;
	t_SimConnect_WeatherRequestObservationAtStation m_pSimConnect_WeatherRequestObservationAtStation;
	t_SimConnect_WeatherRequestObservationAtNearestStation m_pSimConnect_WeatherRequestObservationAtNearestStation;
	t_SimConnect_WeatherCreateStation m_pSimConnect_WeatherCreateStation;
	t_SimConnect_WeatherRemoveStation m_pSimConnect_WeatherRemoveStation;
	t_SimConnect_WeatherSetObservation m_pSimConnect_WeatherSetObservation;
	t_SimConnect_WeatherSetModeServer m_pSimConnect_WeatherSetModeServer;
	t_SimConnect_WeatherSetModeTheme m_pSimConnect_WeatherSetModeTheme;
	t_SimConnect_WeatherSetModeGlobal m_pSimConnect_WeatherSetModeGlobal;
	t_SimConnect_WeatherSetModeCustom m_pSimConnect_WeatherSetModeCustom;
	t_SimConnect_WeatherSetDynamicUpdateRate m_pSimConnect_WeatherSetDynamicUpdateRate;
	t_SimConnect_WeatherRequestCloudState m_pSimConnect_WeatherRequestCloudState;
	t_SimConnect_WeatherCreateThermal m_pSimConnect_WeatherCreateThermal;
	t_SimConnect_WeatherRemoveThermal m_pSimConnect_WeatherRemoveThermal;
	t_SimConnect_AICreateParkedATCAircraft m_pSimConnect_AICreateParkedATCAircraft;
	t_SimConnect_AICreateEnrouteATCAircraft m_pSimConnect_AICreateEnrouteATCAircraft;
	t_SimConnect_AICreateNonATCAircraft m_pSimConnect_AICreateNonATCAircraft;
	t_SimConnect_AICreateSimulatedObject m_pSimConnect_AICreateSimulatedObject;
	t_SimConnect_AIReleaseControl m_pSimConnect_AIReleaseControl;
	t_SimConnect_AIRemoveObject m_pSimConnect_AIRemoveObject;
	t_SimConnect_AISetAircraftFlightPlan m_pSimConnect_AISetAircraftFlightPlan;
	t_SimConnect_ExecuteMissionAction m_pSimConnect_ExecuteMissionAction;
	t_SimConnect_CompleteCustomMissionAction m_pSimConnect_CompleteCustomMissionAction;
	t_SimConnect_Close m_pSimConnect_Close;
	t_SimConnect_RetrieveString m_pSimConnect_RetrieveString;
	t_SimConnect_GetLastSentPacketID m_pSimConnect_GetLastSentPacketID;
	t_SimConnect_Open m_pSimConnect_Open;
	t_SimConnect_CallDispatch m_pSimConnect_CallDispatch;
	t_SimConnect_GetNextDispatch m_pSimConnect_GetNextDispatch;
	t_SimConnect_RequestResponseTimes m_pSimConnect_RequestResponseTimes;
	t_SimConnect_InsertString m_pSimConnect_InsertString;
	t_SimConnect_CameraSetRelative6DOF m_pSimConnect_CameraSetRelative6DOF;
	t_SimConnect_MenuAddItem m_pSimConnect_MenuAddItem;
	t_SimConnect_MenuDeleteItem m_pSimConnect_MenuDeleteItem;
	t_SimConnect_MenuAddSubItem m_pSimConnect_MenuAddSubItem;
	t_SimConnect_MenuDeleteSubItem m_pSimConnect_MenuDeleteSubItem;
	t_SimConnect_RequestSystemState m_pSimConnect_RequestSystemState;
	t_SimConnect_SetSystemState m_pSimConnect_SetSystemState;
	t_SimConnect_MapClientDataNameToID m_pSimConnect_MapClientDataNameToID;
	t_SimConnect_CreateClientData m_pSimConnect_CreateClientData;
	t_SimConnect_AddToClientDataDefinition m_pSimConnect_AddToClientDataDefinition;
	t_SimConnect_ClearClientDataDefinition m_pSimConnect_ClearClientDataDefinition;
	t_SimConnect_RequestClientData m_pSimConnect_RequestClientData;
	t_SimConnect_SetClientData m_pSimConnect_SetClientData;
	t_SimConnect_FlightLoad m_pSimConnect_FlightLoad;
	t_SimConnect_FlightSave m_pSimConnect_FlightSave;
	t_SimConnect_FlightPlanLoad m_pSimConnect_FlightPlanLoad;
	t_SimConnect_Text m_pSimConnect_Text;
	t_SimConnect_SubscribeToFacilities m_pSimConnect_SubscribeToFacilities;
	t_SimConnect_UnsubscribeToFacilities m_pSimConnect_UnsubscribeToFacilities;
	t_SimConnect_RequestFacilitiesList m_pSimConnect_RequestFacilitiesList;
};

#endif


