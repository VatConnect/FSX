#pragma once

#include <math.h>
#include "Packets.h"
#include "CPacketSender.h"
#include "CTime.h"

typedef enum eMode
{
	TOUCH_AND_GO,       //aircraft spawns on downwind to reference point and direction (landing goal), going to 5-mile base turn point
	TAXI                //aircraft spawns east of reference point and wanders around with frequent 90 degree turns
}eMode;

typedef enum eLagType
{
	NO_LAG,             //no simulated lag 
	TYPICAL_LAG,        //e.g. 40-80ms to server, 1-20 at server, 40-80 to client
	HIGH_LAG            //e.g. 200-300 to server, 80-120 to next server, 2-50 at servers, 200-300 to client
} eLagType;

typedef enum eState
{
	GOINGTO_BASETURN,     //Touch-and-go aircraft heads to baseturn point, then to outer marker (OM), then descends to reference point (touchdown point);
	GOINGTO_OM,           //   levels at ground elevation, rotates nose down, increases +10 knots, rotates up, climbs to 1500 and at 500 AGL turns
	GOINGTO_TOUCHDOWN,    //   direct baseturn point
	ROTATING_DOWN,
	ROLLING,
	ROTATING_UP,
	CLIMBING_STRAIGHT,
	TAXI_FAST,            //Taxi aircraft heads toward goal at TAXI_FAST speed, then at goal slows to TAXI_SLOW, then 
	TAXI_SLOWING,         //   determines new goal random left 90 / right 90 degrees, turns toward it, then goes back to TAXI_FAST state
} eState;

typedef struct Position
{
	double dN;          //north/east/up in meters
	double dE;
	double dAlt;
} Position;

typedef class CTestAircraft
{
public:
	CTestAircraft();
	~CTestAircraft();

	//Initialize (or re-initialize) give the callsign, type, reference point (i.e. the user's position on the runway, facing in
	//the direction of the runway), mode (doing touch-and-goes or taxiing around), kind of lag to simulate, update rate (e.g. 1.0 or 5.0),
	//pointer to the packet sender to use, and sequence number for spacing out spawn positions
	void Initialize(char *Callsign, char *ICAOType, double dInitPtLat, double dInitPtLon, double dInitPtHeadingDegTrue, 
		double dGroundElevFt, eMode FlightMode, eLagType LagType, double dUpdateRateSecs, CPacketSender *pSender, long lSpawnNumber);
	
	//Needs to be called regularly
	void Update();

	//Stop updating and just wait to be reinitialized (update() may still be called after this)
	void Stop();

	UpdateObjectPacket m_WaitingPacket;   //"Sent" packet we're holding on to until m_TimeToSendWaitingPacket (to simulate lag)

protected:

	char   m_cCallsign[32];
	double m_dReferenceLatDegN;        //Initial 0.0/0.0 point (runway touchdown if in air, taxi reference point if taxiing) 
	double m_dReferenceLonDegE;    
	double m_dReferenceHdgRads;        //If touch-and-go, this is the runway heading
	double m_dGroundElevMeters;	       
	CTime  m_Time;                     //Our clock
	CPacketSender *m_pSender;         
	bool   m_bInitialized;             //true if we're initialized and running, false if we're waiting for initialization
	eLagType m_LagType;                //what kind of lag we're supposed to simulate
	eState m_State;                    //current state
	eMode  m_Mode;                     //what kind of aircraft behavior we're simulating, TOUCH_AND_GO or TAXI

	bool m_bWaitingPacketValid;            //True if we have an outgoing packet in m_WaitingPacket
	double	m_dTimeToSendWaitingPacket;  

	Position m_Pos;                      //current state (all in meters)
	double m_dGndSpeed;                   //meters/sec
	double m_dHdg;                        //radians
	double m_dPitch;
	double m_dRoll;
	double m_dVelN;                        //velocity vector
	double m_dVelE;
	double m_dVelU; 
	double m_dLastUpdateTime;             //m_Time of our last state update

	Position m_GoalPos;                   //taxi or flight goal we're heading toward
	double m_dGoalSpd;                    //goal groundspeed in m/s
	double m_dGoalPitchBias;              //goal pitch bias in radians, added on top of derived pitch  

	Position m_BaseTurnPos;               //If touch-and-go, this is the base turn point (our goal after taking off)
	Position m_OuterMarkerPos;            //when touch-and-go, this is the point to turn to final. We fly to BaseTurnPos->OuterMarkerPos->TouchdownPos
	Position m_TouchdownPos;

	double m_dTimeToSendNextUpdate;       //m_Time to send out the next position update
	double m_dSendInterval;               //Our send interval. We vary it a little bit based on lag type. 

	//Lat/lon conversion to and from our north/east/up coordinates
	void LLToNEU(double dLatDegN, double dLonDegE, double dAltFt, double* pdNm, double* pdEm, double* pdUm);
	void NEUToLL(double dN, double dE, double dU, double* pdLatDegN, double *pdLonDegE, double *pfAltFt);
	
	void   UpdateState();                 //Check if time to change states or update goal position
	void   UpdatePosAndOrient();          //move aircraft
	double NormalizeRads(double dRads);   //Normalize given angle in radians into -PI to PI
	double NormalizeDegs(double dDeg);    //Normalize given degrees -180 to 180 into 0-360
	double GetHeadingFromVector(double N, double E);  //Return the heading -PI to PI for given northing/easting vector

} CTestAircraft;

