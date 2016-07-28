//This class manages the test aircraft that get spawned when the first user state
//is received.

#include "stdafx.h"
#include "CTestAircraft.h"

//Constants and conversions
const double DEG_TO_RAD = 4.0 * atan(1.0) / 180.0; 
const double RAD_TO_DEG = 180.0 / (4.0 * atan(1.0));
const double PI = 4.0 * atan(1.0);
const double TWOPI	= 2.0 * PI;
#define METERS_PER_DEG 111120.0 
#define EARTH_RADIUS_M 6367445.0 
#define M_TO_FT	   3.28084
#define FT_TO_M	   0.3048
#define KTS_TO_MS  0.5144444444 
#define MS_TO_KTS  1.9438445
#define NM_TO_M    1852.0

//Aircraft constants
#define FAST_TAXI_SPD_KTS  20.0
#define SLOW_TAXI_SPD_KTS  10.0
#define DOWNWIND_SPD_KTS  180.0   
#define LAND_SPD_KTS      140.0   //slows to this after outer marker
#define AIR_SPACING_NM      1.5   //how far apart to spawn touch-and-go aircraft on downwind
#define GND_SPACING_M     200.0   //how far apart to spawn taxi aircraft
#define GND_TAXI_DIST_M   100.0   //how far between 90 degree turn points, for taxiing aircraft
#define TURN_LEAD_DIST_M  1870.0   //how soon before goal point to start turn (90 degree turn) for airborne aircraft
#define AIR_ACCEL_MS        0.5   //about 1 kts / sec^2
#define GND_ACCEL_MS        1.0   //2 kts/sec^2
#define AIR_MAX_TURN_DEG    3.0   //3 degrees per second max turn rate in air
#define GND_MAX_TURN_DEG   15.0   //15 degrees per second max turnrate on ground
#define MAX_ROLL_DEG       40.0   //max bank
#define MAX_ROLLRATE_DEG   15.0   //degrees per second
#define MAX_PITCHRATE_DEG   3.0   //degrees per second

//Random # between 0.0 and 1.0
#define drand() ((double)rand() / (double)(RAND_MAX + 1.0))

CTestAircraft::CTestAircraft() : m_pSender(NULL), m_bWaitingPacketValid(false), m_bInitialized(false)
{
}

CTestAircraft::~CTestAircraft()
{
}

//Initialize (or re-initialize) give the callsign, type, reference point (i.e. the user's position on the runway, facing in
//the direction of the runway), mode (doing touch-and-goes or taxiing around), kind of lag to simulate, update rate in seconds 
//(e.g. 1.0 or 5.0), pointer to the packet sender to use, and sequence number (0 upward) for spacing out spawn positions
void CTestAircraft::Initialize(char *Callsign, char *ICAOType, double dInitPtLat, double dInitPtLon, double dInitPtHeadingDegTrue, 
		double dGroundElevFt, eMode FlightMode, eLagType LagType, double dUpdateRateSecs, CPacketSender *pSender, long lSpawnNumber)
{
	strcpy_s(m_cCallsign, sizeof(m_cCallsign), Callsign);
	m_dReferenceLatDegN = dInitPtLat;
	m_dReferenceLonDegE = dInitPtLon;
	m_dReferenceHdgRads = NormalizeRads(dInitPtHeadingDegTrue * DEG_TO_RAD);
	m_dGroundElevMeters = dGroundElevFt * FT_TO_M;
	m_pSender = pSender;
	m_bInitialized = true;
	m_LagType = LagType;
	m_Mode = FlightMode;
	m_dLastUpdateTime = m_Time.GetTimeSeconds();
	m_dSendInterval = dUpdateRateSecs;
	m_dTimeToSendNextUpdate = m_dLastUpdateTime + dUpdateRateSecs * drand();

	LLToNEU(dInitPtLat, dInitPtLon, dGroundElevFt, &m_TouchdownPos.dN, &m_TouchdownPos.dE, &m_TouchdownPos.dAlt);

	//Determine spawn point and initial state
	if (m_Mode == TOUCH_AND_GO)
	{
		//Determine outer marker / final turn point (5 miles out)
		m_OuterMarkerPos.dN = 5.0 * NM_TO_M * cos(m_dReferenceHdgRads - PI) + m_TouchdownPos.dN;
		m_OuterMarkerPos.dE = 5.0 * NM_TO_M * sin(m_dReferenceHdgRads - PI) + m_TouchdownPos.dE;
		m_OuterMarkerPos.dAlt = 1500 * FT_TO_M + dGroundElevFt;

		//Determine base turn point (3 miles abeam of OM -- left traffic)
		m_BaseTurnPos.dN = 3.0 * NM_TO_M * cos(m_dReferenceHdgRads - PI / 2.0) + m_OuterMarkerPos.dN;
		m_BaseTurnPos.dE = 3.0 * NM_TO_M * sin(m_dReferenceHdgRads - PI / 2.0) + m_OuterMarkerPos.dE;
		m_BaseTurnPos.dAlt = m_OuterMarkerPos.dAlt;

		//Determine spawn point, initial attitude and goal position. Spread out from previous depending on spawn #
		m_Pos.dN = (1.0 + AIR_SPACING_NM * (double)lSpawnNumber) * NM_TO_M * cos(m_dReferenceHdgRads) + m_BaseTurnPos.dN;
		m_Pos.dE = (1.0 + AIR_SPACING_NM * (double)lSpawnNumber) * NM_TO_M * sin(m_dReferenceHdgRads) + m_BaseTurnPos.dE;
		m_Pos.dAlt = m_BaseTurnPos.dAlt;
		m_dHdg = NormalizeRads(m_dReferenceHdgRads - PI);
		m_dPitch = 0.0;
		m_dRoll = 0.0;
		m_dGndSpeed = DOWNWIND_SPD_KTS * KTS_TO_MS; 
		m_dGoalSpd = m_dGndSpeed;
		m_dGoalPitchBias = 0.0;
		memcpy_s(&m_GoalPos, sizeof(Position), &m_BaseTurnPos, sizeof(Position));
		
		m_State = GOINGTO_BASETURN;
	}
	//Taxiing around
	else
	{
		//Determine spawn point and orientation
		m_Pos.dN = m_TouchdownPos.dN + (300.0 + GND_SPACING_M * (double)lSpawnNumber) * cos(m_dReferenceHdgRads + PI / 2.0);
		m_Pos.dE = m_TouchdownPos.dE + (300.0 + GND_SPACING_M * (double)lSpawnNumber) * sin(m_dReferenceHdgRads + PI / 2.0);
		m_Pos.dAlt = m_dGroundElevMeters;
		m_dHdg = m_dReferenceHdgRads;
		m_dPitch = 0.0;
		m_dRoll = 0.0;
		m_dGndSpeed = 0.0;

		//Determine initial goal (straight ahead)
		m_GoalPos.dN = m_Pos.dN + GND_TAXI_DIST_M * cos(m_dReferenceHdgRads);
		m_GoalPos.dE = m_Pos.dE + GND_TAXI_DIST_M * sin(m_dReferenceHdgRads);
		m_GoalPos.dAlt = m_dGroundElevMeters;
		m_dGoalSpd = FAST_TAXI_SPD_KTS * KTS_TO_MS;
		m_dGoalPitchBias = 0.0;

		m_State =TAXI_FAST;
	}

	//Determine initial velocity vector

	m_dVelN = cos(m_dHdg) * m_dGndSpeed;
	m_dVelE = sin(m_dHdg) * m_dGndSpeed;
	m_dVelU = 0.0;

	//Send add-object packet. 
	double dLat, dLon, dAltFt;
	NEUToLL(m_Pos.dN, m_Pos.dE, m_Pos.dAlt, &dLat, &dLon, &dAltFt);

	AddObjectPacket P;
	strcpy_s(P.szCallsign, 32, m_cCallsign);
	strcpy_s(P.szICAOType, 32, ICAOType);
	P.LatDegN = dLat;
	P.LonDegE = dLon;  
	P.AltFtMSL = dAltFt;
	P.PitchDegUp = 0.0;
	P.HdgDegTrue = NormalizeDegs(m_dHdg * RAD_TO_DEG); 
	P.RollDegRight = 0.0;
	P.GroundSpeedKts = m_dGndSpeed * MS_TO_KTS;
	m_pSender->Send(&P);
	printf("Sent ADD_OBJECT: ");
	printf(m_cCallsign);
	printf("\n");
	
	return;
}

//Update aircraft position, check if pending outgoing packet should be sent,
//and check if new update needs to be put in the pending outgoing packet. We
//"send" the updates to the pending packet first, to simulate lag, then actually send
//it once the lag is up.
void CTestAircraft::Update()
{
	if (!m_bInitialized)
		return;

	//Check if time to send outgoing packet
	double Time = m_Time.GetTimeSeconds();
	if (m_bWaitingPacketValid && Time >= m_dTimeToSendWaitingPacket)
	{
		m_pSender->Send(&m_WaitingPacket);
		m_bWaitingPacketValid = false;
	}

	//Update aircraft state, position and orientation 
	UpdateState();
	UpdatePosAndOrient();

	//Check if time to send position update (actually to put it in the waiting
	//packet to simulate lag) 
	if (Time >= m_dTimeToSendNextUpdate && !m_bWaitingPacketValid)
	{
		//Get our position in lat/lon
		double dLat, dLon, dAltFt;
		NEUToLL(m_Pos.dN, m_Pos.dE, m_Pos.dAlt, &dLat, &dLon, &dAltFt);
		
		//Fill out the update state packet (m_WaitingPacket)
		strcpy_s(m_WaitingPacket.szCallsign, 32, m_cCallsign);
		m_WaitingPacket.LatDegN = dLat;
		m_WaitingPacket.LonDegE = dLon;
		m_WaitingPacket.AltFtMSL = dAltFt;
		m_WaitingPacket.PitchDegUp = m_dPitch * RAD_TO_DEG;
		m_WaitingPacket.HdgDegTrue = NormalizeDegs(m_dHdg * RAD_TO_DEG);
		m_WaitingPacket.RollDegRight = m_dRoll * RAD_TO_DEG;
		m_WaitingPacket.GroundSpeedKts = m_dGndSpeed * MS_TO_KTS;
		m_WaitingPacket.bExtendedDataValid = false;
		m_bWaitingPacketValid = true;

		//Determine time to send it, accounting for lag. (If update intervals are less than the max lag
		//we're simulating, we should probably make the waiting packets an array). 
		switch (m_LagType)
		{
		case NO_LAG: 
			m_dTimeToSendWaitingPacket = Time;
			break;
		case TYPICAL_LAG:
			//40-80ms to server, 1-20ms at server, 40-80ms to client calculated along a bell curve (three "dice rolls" added up)
			m_dTimeToSendWaitingPacket = Time + (40.0 + drand() * (40.0 / 3.0) + drand() * (40.0 / 3.0) + drand() * (40.0 / 3.0) +
				                                  1.0 + drand() * (20.0 / 3.0) + drand() * (20.0 / 3.0) + drand() * (20.0 / 3.0) +
												 40.0 + drand() * (40.0 / 3.0) + drand() * (40.0 / 3.0) + drand() * (40.0 / 3.0)) / 1000.0;
			break;
			//200-300 to server, 80-120 to next server, 2-50 at servers, 200-300 to client
		case HIGH_LAG:
			m_dTimeToSendWaitingPacket = Time + (200.0 + drand() * (100.0 / 3.0) + drand() * (100.0 / 3.0) + drand() * (100.0 / 3.0) +
				                                  80.0 + drand() * ( 40.0 / 3.0) + drand() * ( 40.0 / 3.0) + drand() * ( 40.0 / 3.0) +
												   2.0 + drand() * ( 48.0 / 3.0) + drand() * ( 48.0 / 3.0) + drand() * ( 48.0 / 3.0) +
											     200.0 + drand() * (100.0 / 3.0) + drand() * (100.0 / 3.0) + drand() * (100.0 / 3.0)) / 1000.0;
			break;
		}

		//Calculate time for next update. We use the straight send interval because random variation in a client's send interval is 
		//pretty much accounted for with the randomness of the lag.
		m_dTimeToSendNextUpdate = Time + m_dSendInterval;
	}
	return;
}

void CTestAircraft::Stop()
{
	m_bInitialized = false;
}

//Check if time to switch states, and update goal altitude, speed and pitch (goal heading is always toward goal position)
void CTestAircraft::UpdateState()
{
	double VecToGoalN = (m_GoalPos.dN - m_Pos.dN);
	double VecToGoalE = (m_GoalPos.dE - m_Pos.dE);
	//double VecToGoalU = (m_GoalPos.dAlt - m_Pos.dAlt);
	double dDistToGoal = sqrt(VecToGoalN * VecToGoalN + VecToGoalE * VecToGoalE);
	bool bGoalIsBehind = (VecToGoalN * m_dVelN + VecToGoalE * m_dVelE) < 0.0? true : false;

	switch(m_State)
	{
	case GOINGTO_BASETURN:

		//Time to turn base?
		if (dDistToGoal <= TURN_LEAD_DIST_M)
		{
			//head for outer marker
			memcpy_s(&m_GoalPos, sizeof(Position), &m_OuterMarkerPos, sizeof(Position));
			m_State = GOINGTO_OM;
		}
		break;
	
	case GOINGTO_OM:

		//Time to turn final?
		if (dDistToGoal <= TURN_LEAD_DIST_M)
		{
			//head for touchdown point
			memcpy_s(&m_GoalPos, sizeof(Position), &m_TouchdownPos, sizeof(Position));
			m_State = GOINGTO_TOUCHDOWN;
			m_dGoalPitchBias = 8.0 * DEG_TO_RAD;
			m_GoalPos.dAlt = m_dGroundElevMeters;
		}
		break;
	
	case GOINGTO_TOUCHDOWN:

		//Touched down?
		if (m_Pos.dAlt <= m_dGroundElevMeters)
		{
			m_State = ROTATING_DOWN;
			m_dGoalSpd = 130.0 * KTS_TO_MS;
			m_dGoalPitchBias = 0.0;
		}
		else
		{
			//Goal speed is 140 kts plus 10 knots per NM out from touchdown point
			m_dGoalSpd = (140.0  + 10.0 * dDistToGoal / NM_TO_M) * KTS_TO_MS;
		}
		break;

	case ROTATING_DOWN:

		if (m_dPitch <= 0.05)  //REVISIT this is based on the default MS B737-800 model
		{
			m_State = ROLLING;
			m_dGoalSpd = 180.0 * KTS_TO_MS;
		}
		break;

	case ROLLING:
		
		if (m_dGndSpeed >= (150.0 * KTS_TO_MS))
		{
			m_State = ROTATING_UP;
			m_dGoalPitchBias = 8.0 * DEG_TO_RAD;
		}
		break;

	case ROTATING_UP:
		
		if (m_dPitch >= (8.0 * DEG_TO_RAD))
		{
			m_State = CLIMBING_STRAIGHT;
			m_GoalPos.dAlt = 1500.0 * FT_TO_M;
		}
		break;

	case CLIMBING_STRAIGHT:

		if (m_Pos.dAlt >= (500.0 * FT_TO_M + m_dGroundElevMeters)) 
		{
			m_State = GOINGTO_BASETURN;
			memcpy_s(&m_GoalPos, sizeof(Position), &m_BaseTurnPos, sizeof(Position));
			m_dGoalPitchBias = 0.0;
		}
		break;

	case TAXI_FAST:

		if (bGoalIsBehind && dDistToGoal < 10.0)
		{
			m_State = TAXI_SLOWING;
			m_dGoalSpd = SLOW_TAXI_SPD_KTS * KTS_TO_MS;
		}
		break;

	case TAXI_SLOWING:

		if (m_dGndSpeed <= SLOW_TAXI_SPD_KTS)
		{
			//Pick new goal point -- ~70 degrees left or right so by time we
			//turn toward it, it's about 90 degrees off our previous course
			double dir;
			if (drand() < 0.5)  
				dir = m_dHdg - 0.72 * PI / 2.0;
			else
				dir = m_dHdg + 0.72 * PI / 2.0;
			
			m_GoalPos.dN = m_Pos.dN + cos(dir) * GND_TAXI_DIST_M;
			m_GoalPos.dE = m_Pos.dE + sin(dir) * GND_TAXI_DIST_M;
			m_GoalPos.dAlt = m_dGroundElevMeters;
			m_dGoalSpd = FAST_TAXI_SPD_KTS * KTS_TO_MS;
			m_State = TAXI_FAST;
		}
		break;
	}
	return;
}

//Update aircraft's position and orientation
void CTestAircraft::UpdatePosAndOrient()
{
	double VecToGoalN = (m_GoalPos.dN - m_Pos.dN);
	double VecToGoalE = (m_GoalPos.dE - m_Pos.dE);
	double dDistToGoal = sqrt(VecToGoalN * VecToGoalN + VecToGoalE * VecToGoalE);
	bool bOnGround = (m_State == TAXI_FAST || m_State == TAXI_SLOWING)? true : false;

	//Get delta time since last update
	double dT = m_Time.GetTimeSeconds() - m_dLastUpdateTime;
	if (dT < 1e-8)
		return;

	//If not in locked heading state, update heading and roll 
	double dIdealRollDegs = 0.0;
	if (!(m_State == ROTATING_DOWN || m_State == ROLLING || m_State == ROTATING_UP || m_State == CLIMBING_STRAIGHT || m_State == TAXI_SLOWING))
	{
		//Get angle between current and goal
		double dHdgToGoal = GetHeadingFromVector(VecToGoalN, VecToGoalE);
		double dAngDiff = NormalizeRads(dHdgToGoal - m_dHdg);

		//Determine max amount for this delta time and clamp to that. Also
		//determine ideal roll which is proportional to the turn rate.
		if (fabs(dAngDiff) > 1E-3)
		{
			double dMaxDiff, dOrigDiff = dAngDiff;
			if (bOnGround)
				dMaxDiff = GND_MAX_TURN_DEG * DEG_TO_RAD * dT;
			else
				dMaxDiff = AIR_MAX_TURN_DEG * DEG_TO_RAD * dT;
			if (fabs(dAngDiff) > dMaxDiff)
				dAngDiff *= (dMaxDiff / fabs(dAngDiff));
			double dNewVelHdg = m_dHdg + dAngDiff / dT;
			m_dHdg += dAngDiff;

			//Determine ideal roll from sideways acceleration due to this change in heading, but only
			//if heading change is significant. (This is reverse from a flight sim where you determine
			//change in heading from amount of roll, but it's simpler to move aircraft around this way). 
			//This is just trigonometry, where the old velocity vector is the base of the triangle, new 
			//velocity vector the hypotenuse, and acceleration the difference between them. That forms a
			//triangle, so here we calculate the magnitude of that third side (the acceleration vector) 
			//and scale the roll proportional to it.
			if (fabs(dOrigDiff) > (5.0 * DEG_TO_RAD)) 
			{
				double dNewVelN = cos(dNewVelHdg) * m_dGndSpeed;
				double dNewVelE = sin(dNewVelHdg) * m_dGndSpeed;
				double dAccelN = dNewVelN - m_dVelN;
				double dAccelE = dNewVelE - m_dVelE;
				double LatAccelSq = dAccelN * dAccelN + dAccelE * dAccelE;
				if (LatAccelSq > 1E-9 && m_dGndSpeed > 1E-9)
				{
					double Vn = m_dVelN / m_dGndSpeed;
					double Ve = m_dVelE / m_dGndSpeed;
					double Temp = Vn * dAccelN + Ve * dAccelE;
					double SinSq = 1.0 - Temp * Temp / LatAccelSq; 
					if (SinSq > 1E-9)
					{
						dIdealRollDegs = sqrt(SinSq * LatAccelSq) * 6.0 * DEG_TO_RAD;  //constant is factor for amount of roll per change in heading
						if ((Vn * dAccelE - dAccelN * Ve) < 0.0) //cross product to determine if new velocity left or right of old velocity
							dIdealRollDegs *= -1.0;
					}
				}
			}
			
			dIdealRollDegs = NormalizeRads(dIdealRollDegs) * RAD_TO_DEG;

			if (m_State == TAXI_FAST || m_State == TAXI_SLOWING)
				dIdealRollDegs = 0.0;
		}
		else
			dIdealRollDegs = 0.0;
	}
	else
		dIdealRollDegs = 0.0;

	//Clamp change in roll to max and update roll
	double dDeltaRoll = NormalizeRads(dIdealRollDegs * DEG_TO_RAD - m_dRoll);
	double dMaxRollDelta = MAX_ROLLRATE_DEG * DEG_TO_RAD * dT;
	if (dDeltaRoll > 1E-8)
	{
		if (fabs(dDeltaRoll) > dMaxRollDelta)
			dDeltaRoll *= (dMaxRollDelta / fabs(dDeltaRoll));
	}
	m_dRoll += dDeltaRoll;

	//Update speed and clamp to max acceleration
	double dDeltaSpd = m_dGoalSpd - m_dGndSpeed;
	double dMaxSpdChange;
	if (bOnGround)
		dMaxSpdChange = GND_ACCEL_MS * dT;
	else
		dMaxSpdChange = AIR_ACCEL_MS * dT;
	if (fabs(dDeltaSpd) > dMaxSpdChange)
		dDeltaSpd *= (dMaxSpdChange / fabs(dDeltaSpd));
	m_dGndSpeed += dDeltaSpd; 

	//Update altitude -- if on final, descend linearly to runway,
	//otherwise clamp max rate change to 1500 FPM
	double dDeltaAlt = m_GoalPos.dAlt - m_Pos.dAlt;
	if (m_State == GOINGTO_TOUCHDOWN)
	{
		if (dDistToGoal < 1E-8 || m_dGndSpeed < 1E-8)
			dDeltaAlt = 0.0;
		else
     		dDeltaAlt = dT * m_dGndSpeed * dDeltaAlt / dDistToGoal;
	}
	else
	{
		double dMaxDeltaAlt = 1500 / 60.0 * FT_TO_M * dT;
		if (fabs(dDeltaAlt) > dMaxDeltaAlt)
			dDeltaAlt *= (dMaxDeltaAlt / fabs(dDeltaAlt));
	}
	m_Pos.dAlt += dDeltaAlt;

	//Update Pitch
	double dIdealPitchRads = atan2((dDeltaAlt / dT), (m_dGndSpeed > 1E-8? m_dGndSpeed : 1.0)) + 2.0 * DEG_TO_RAD + m_dGoalPitchBias;
	if (bOnGround)
		dIdealPitchRads = 0.0;

	//Clamp change in pitch to max
	double dPitchDiff = dIdealPitchRads - m_dPitch;
	if (fabs(dPitchDiff) > 1E-8)
	{
		double dMaxPitchDiff = MAX_PITCHRATE_DEG * DEG_TO_RAD * dT;
		if (fabs(dPitchDiff) > dMaxPitchDiff)
			dPitchDiff *= (dMaxPitchDiff / fabs(dPitchDiff));
	}
	m_dPitch += dPitchDiff;

	//Update velocity vector
	m_dVelN = cos(m_dHdg) * m_dGndSpeed;
	m_dVelE = sin(m_dHdg) * m_dGndSpeed;
	m_dVelU = dDeltaAlt / dT;

	//Update position (altitude already updated)
	m_Pos.dN += (m_dVelN * dT);
	m_Pos.dE += (m_dVelE * dT);

	m_dLastUpdateTime += dT;

	return;
}

//Return north/east/up coordinates in meters given a lat/lon 
void CTestAircraft::LLToNEU(double dLatDegN, double dLonDegE, double dAltFt, double* pdNm, double* pdEm, double* pdUm)
{	
    *pdNm = (dLatDegN - m_dReferenceLatDegN) * EARTH_RADIUS_M * DEG_TO_RAD; 
    *pdEm = (dLonDegE - m_dReferenceLonDegE) * METERS_PER_DEG * cos(m_dReferenceLatDegN * DEG_TO_RAD); 
    *pdUm= dAltFt * FT_TO_M;

	return;
}

//Return lat/lon from our north/east/up coordinates in meters
void CTestAircraft::NEUToLL(double dN, double dE, double dU, double* pdLatDegN, double *pdLonDegE, double *pfAltFt)
{
     
   *pdLatDegN = dN / (DEG_TO_RAD * EARTH_RADIUS_M) + m_dReferenceLatDegN; 
   *pdLonDegE = dE / (METERS_PER_DEG * cos(m_dReferenceLatDegN * DEG_TO_RAD)) + m_dReferenceLonDegE; 
   *pfAltFt = dU * M_TO_FT;

   return; 
} 

//Normalize given angle in radians into -PI to PI
double CTestAircraft::NormalizeRads(double r)
{
	return r - TWOPI * floor(0.5 + r / TWOPI);
}

//Normalize given angle -180 to +180 into 0-360. 
double CTestAircraft::NormalizeDegs(double d)
{
	if (d < 0.0)
		d += 360.0;
	return d;
}

//Return the heading in radians (-PI to PI) for the given north/east vector
double CTestAircraft::GetHeadingFromVector(double N, double E)
{
	if (N != 0.0)
		return NormalizeRads(atan2(E, N));
	else if (E > 0.0f)
		return PI / 2.0;
	return -PI / 2.0;
}
