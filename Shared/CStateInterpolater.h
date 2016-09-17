//THIS IS A SHARED FILE
//BE CAREFUL ABOUT CHANGING!
//

//This class is used to interpolate position and orientation of a moving object, when updates to the object are sparse
//yet it needs to move and be drawn much more frequently. 
//
//As updates come in, regardless of interval (say, once a second), they're pushed in with UpdateState(). Meanwhile callers 
//can call GetState() at any other interval (doesn't matter, say, 60 times a second) and the returned values will hopefully 
//be close predictions. Updates are pushed in with a time hopefully reported by the object. We don't care what the value is, 
//we just care about the intervals between that and the previous updates. This removes network or server lag effects. However, 
//if the object is not reporting its time with each update, the second best is to have the server report its time on receipt 
//(which removes the lag effects from server to us, but not object to the server). If that's not available, the caller should 
//report a time of -9999.0 so we know there are variable lag effects and derived velocities and accelerations are probably 
//really bad. In that case we use reported groundspeed if available, in preference to derived velocity.
//
//The more frequent the updates come in, the more accurate the interpolated positions will be. 10HZ is great, 1HZ may
//have a little warping visible when close, and above that will have increasingly more.
//
//Objects are pushed in and reported in lat/lon, altitude in feet and orientation in degrees, although internally we work with 
//it in orthogonal coordinates (Northing/Easting/Up, orientation in radians). Technically the lat/lon are based on the WGS-84
//squashed earth model, but for simplicity we use a spherical model based on the closest approximation. For most purposes this 
//is probably perfectly okay. 
//
//To use, just push in updates as you get them, with UpdateState(), and pull out extrapolated positions with GetStateNow(). If
//you call GetStateNow() before the first UpdateState(), it'll return error but set the position to 0 degrees north and east.
//
#pragma once

#include <memory.h>
#include <math.h>
#include <stdio.h>
#include "CTime.h"

//Vector for north/east/up
typedef struct PosVector
{
	double N;          //northing in meters
	double E;          //easting in meters
	double U;          //up in meters MSL
} PosVector;

//Vector for lat/lon/alt
typedef struct LLVector
{
	double DegN;
	double DegE;
	double AltFtMSL;
} LLVector;

//Vector for heading/pitch/roll
typedef struct OrientVector
{
	double H;          //heading (true) in radians
	double P;          //pitch in radians, positive going up
	double R;          //roll in radians, positive going clockwise (looking out from object)
} OrientVector;

//Position and orientation 
typedef struct PosOrientStruct
{
	LLVector	 vecPos;
	OrientVector vecOrient;
} PosOrientStruct;

//Information on a state 
typedef struct StateStruct
{
	double			dStateTimeSecs;       //Object's clock time when it was at this state (actual value unimportant, we use difference in seconds between states)
	double          dOurTimeSecs;         //Our clock time corresponding to this state 
	LLVector        vecPos;				  //position in lat/lon/altitude
	OrientVector	vecOrient;			  //orientation 
	OrientVector	vecAngVel;            //angular velocity
	PosVector		vecVel;               //positional velocity in meters per second
	PosVector		vecAccel;             //positional acceleration in m/s^2, derived from velocity change
	PosVector		vecPosErr;            //error between last update and extrapolated position and orientation at that time. We do this to 
	OrientVector	vecOrientErr;         //   smooth to the new position instead of snapping to it.
	double		    dRepGroundspeedMS;    //object's self-reported groundspeed, in m/s
} StateStruct;

typedef class CStateInterpolater
{
public:
	CStateInterpolater();
	~CStateInterpolater();

	//Update the state as of the object's reported time (i.e. I was at this state according to this time on my clock). returns 1 success, 0 fail.
	//If object's time is unknown, set to -9999.0 or less and it will be derived. Roll is positive to the right, looking forward
	//out of the object. The groundspeed should be the one reported by the object. 
	int UpdateState(double dTimeSeconds, double dLatDegN, double dLonDegE, double dAltMSLFt, double dHeadingDegTrue, double dPitchDegPosUp, 
		double dRollDegPosRight, double dGroundSpeedKts);

	//Get the state as of now, which is a best guess of where the object is based on interpolating from previous updates. 1 success, 0 fail. 
	int GetStateNow(double *pdLatDegN, double *pdLonDegE, double *pdAltMSLFt, double *pdHeadingDegTrue, double *pdPitchDegPosUp, double *pdRollDegPosRight);

	//Get the current vertical speed in feet per minute (positive up)
	double GetVerticalSpeedFPM();

protected:

	//Calculate the difference (north/east/up) between two lat/long/alt points (Pt2 - Pt1, result in D)
	void CalcLLDelta(LLVector &Pt1, LLVector &Pt2, PosVector &D);

	//Return the lat/lon from a given lat/lon point plus a north/east/up offset (Pt1 + V, result in Offset)
	void CalcLLOffset(LLVector &Pt1, PosVector &V, LLVector &Offset);

	//Return the 2D magnitude (north and east only) for the given vector
	double VecMag2D(PosVector &V);

	//Normalize given angle in radians into -PI to PI
	double NormalizeRads(double r);

	//Normalize given degrees +/- 360 to +/- 90 (meant for normalizing pitch, so e.g. +120 normalized to +60)
	double NormalizePitch(double Deg);

	//Set the initial state (i.e. object was at this position and orientation at the given object's clock time).
	//-9999.0 if there is not an object reported time. Pitch positive upward, roll positive going to the right 
	//(looking out from the object). Return 1 if success, 0 if fail. 
	int Initialize(double dTimeSeconds, double dLatDegN, double dLonDegE, double dAltMSLFt, double dHeadingDegTrue, double dPitchDegPosUp, 
		double dRollDegPosRight, double dGroundSpeedKts);

	//Given a state (with our timestamp), and knowing our current time in m_OurTime, extrapolate the position and
	//orientation for the given time based on that state, using its velocity and acceleration vectors
	void ExtrapPosOrientFromState(double dOurTimeSecs, StateStruct *pState, PosOrientStruct *pExtrap);

	//Return the difference in radians, max +/- PI. Idea is even if OldRads is +179 degrees
	//(speaking in degrees) and new is -179 degrees it should return +2 degrees and not -358 degrees  
	double CalcAngDiff(double NewRads, double OldRads);

	CTime m_OurTime;
	StateStruct m_State[2];         //State updates we have received. Done as an array with pointers for now in case we need to store more states
	StateStruct *m_pLatestUpdate;   //Most recent state update
	StateStruct *m_pPreviousUpdate; //Previous update. When new updates come in we just swap pointers but we could memcpy the array down one.
	bool m_bStateDataValid;         //false if we haven't been initialized yet
	bool m_bDerivingTimes;          //true if object isn't reporting a time, so we need to account for server and network lag
} 
CStateInterpolater;

