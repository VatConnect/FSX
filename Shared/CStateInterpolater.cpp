#include "stdafx.h"
#include "CStateInterpolater.h"

//Constants. Earth radius is the closest sphere to WGS-84 model. Meters-per-deg are per degree longtitude at equator
const double DEG_TO_RAD = 4.0 * atan(1.0) / 180.0; 
const double RAD_TO_DEG = 180.0 / (4.0 * atan(1.0));
const double PI = 4.0 * atan(1.0);
const double TWOPI = 8.0 * atan(1.0);
#define METERS_PER_DEG 111120.0 
#define EARTH_RADIUS_M 6367445.0 
#define M_TO_FT	   3.28084
#define FT_TO_M	   0.3048
#define KTS_TO_MS  0.5144444444 

//This is pretty important to the user experience. It's arbitrary, but don't set to 0. It's how long it takes to
//"slide" to the correct position, in seconds, when updates are sparse and we have been heading off blindly on our
//predicted positions. The lower it is, the faster the warping to the correct position, but also the more accurate the 
//position is. The higher it is, the smoother it looks to the user, but the more inaccurate the position.  
#define POS_CORRECT_TIME 1.0   
#define ORIENT_CORRECT_TIME 3.0

CStateInterpolater::CStateInterpolater() : m_pLatestUpdate(&m_State[0]), m_pPreviousUpdate(&m_State[1]), m_bStateDataValid(false)
{
}

CStateInterpolater::~CStateInterpolater()
{
}

//Update the state as of the object's reported time (i.e. I was at this state according to this time on my clock). returns 1 success, 0 fail.
//If object's time is unknown, set to -9999.0 or less and it will be derived. Roll is positive to the right, looking forward
//out of the object. The groundspeed should be the one reported by the object. 
int CStateInterpolater::UpdateState(double dTimeSeconds, double dLatDegN, double dLonDegE, double dAltMSLFt, double dHeadingDegTrue, 
	double dPitchDegPosUp, double dRollDegPosRight, double dGroundSpeedKts)
{
	//First update?
	if (!m_bStateDataValid)
		return Initialize(dTimeSeconds, dLatDegN, dLonDegE, dAltMSLFt, dHeadingDegTrue, dPitchDegPosUp, dRollDegPosRight, dGroundSpeedKts);

	//Update (swap) the latest and previous update pointers, so latest state before this becomes the "previous"
	StateStruct *p = m_pLatestUpdate;
	m_pLatestUpdate = m_pPreviousUpdate;
	m_pPreviousUpdate = p;

	//Fill in new position and orientation 
	m_pLatestUpdate->dOurTimeSecs = m_OurTime.GetTimeSeconds();
	m_pLatestUpdate->vecPos.DegN = dLatDegN;
	m_pLatestUpdate->vecPos.DegE = dLonDegE;
	m_pLatestUpdate->vecPos.AltFtMSL = dAltMSLFt;
	m_pLatestUpdate->vecOrient.H = NormalizeRads(dHeadingDegTrue * DEG_TO_RAD);
	m_pLatestUpdate->vecOrient.P = NormalizeRads(dPitchDegPosUp * DEG_TO_RAD);
	m_pLatestUpdate->vecOrient.R = NormalizeRads(dRollDegPosRight * DEG_TO_RAD);
	m_pLatestUpdate->dRepGroundspeedMS = dGroundSpeedKts * KTS_TO_MS;

	//Save out the difference in updated positions
	PosVector vecDeltaPos;
	CalcLLDelta(m_pPreviousUpdate->vecPos, m_pLatestUpdate->vecPos, vecDeltaPos);
	double dMagDeltaPos = VecMag2D(vecDeltaPos);

	//Determine object's time of this state. If object not reporting a time, use our time of receipt and hope that lag is
	//pretty constant!
	if (!m_bDerivingTimes)
		m_pLatestUpdate->dStateTimeSecs = dTimeSeconds; 
	else
		m_pLatestUpdate->dStateTimeSecs = m_pPreviousUpdate->dStateTimeSecs + (m_pLatestUpdate->dOurTimeSecs - m_pPreviousUpdate->dOurTimeSecs);

	//Determine velocity, acceleration and angular velocity vectors  
	double dDeltaTime = m_pLatestUpdate->dStateTimeSecs - m_pPreviousUpdate->dStateTimeSecs;
	double dMagRepVel = dGroundSpeedKts * KTS_TO_MS;
	
	 if (dDeltaTime < 1E-8)
	{
		//If this happens, something is wrong... lag is out of control? updates came out of order? 
		//Copy from previous velocity 
		m_pLatestUpdate->vecVel.N = m_pPreviousUpdate->vecVel.N;
		m_pLatestUpdate->vecVel.E = m_pPreviousUpdate->vecVel.E;
		m_pLatestUpdate->vecVel.U = m_pPreviousUpdate->vecVel.U;
		m_pLatestUpdate->vecAccel.N = 0.0;
		m_pLatestUpdate->vecAccel.E = 0.0;
		m_pLatestUpdate->vecAccel.U = 0.0;
		m_pLatestUpdate->vecAngVel.H = 0.0;
		m_pLatestUpdate->vecAngVel.P = 0.0;
		m_pLatestUpdate->vecAngVel.R = 0.0;
	}
	else
	{
		//If we're deriving times, scale the N/E velocity to match the reported groundspeed. This should be more accurate than deriving,
		//because of the uncertainty in network and server lag
		if (m_bDerivingTimes)
		{
			if (dMagDeltaPos > 1E-8)
			{
				m_pLatestUpdate->vecVel.N = (vecDeltaPos.N / dMagDeltaPos * dMagRepVel); 
				m_pLatestUpdate->vecVel.E = (vecDeltaPos.E / dMagDeltaPos * dMagRepVel); 
			}
			else
			{
				//Weird case -- we have a reported groundspeed yet we also haven't moved. Could happen if object doesn't 
				//accelerate, just instantly zaps to its velocity and it happened just as it sent its update. 
				//Ignore it for now otherwise we'll calculate a huge acceleration and be way off (we will smooth to it 
				//in the next object update).
				m_pLatestUpdate->vecVel.N = 0.0;
				m_pLatestUpdate->vecVel.E = 0.0;
			}
		}
		//We have object-reported times, so this should be more accurate than reported groundspeed (which is probably only integer precision)
		else
		{
			m_pLatestUpdate->vecVel.N = vecDeltaPos.N / dDeltaTime; 
			m_pLatestUpdate->vecVel.E = vecDeltaPos.E / dDeltaTime;
		}
		m_pLatestUpdate->vecVel.U = vecDeltaPos.U / dDeltaTime;

		//Calculate acceleration
		m_pLatestUpdate->vecAccel.N = (m_pLatestUpdate->vecVel.N - m_pPreviousUpdate->vecVel.N) / dDeltaTime;
		m_pLatestUpdate->vecAccel.E = (m_pLatestUpdate->vecVel.E - m_pPreviousUpdate->vecVel.E) / dDeltaTime;
		m_pLatestUpdate->vecAccel.U = (m_pLatestUpdate->vecVel.U - m_pPreviousUpdate->vecVel.U) / dDeltaTime;

		//Calculate angular velocity
		m_pLatestUpdate->vecAngVel.H = CalcAngDiff(m_pPreviousUpdate->vecOrient.H, m_pLatestUpdate->vecOrient.H) / dDeltaTime;
		m_pLatestUpdate->vecAngVel.P = CalcAngDiff(m_pPreviousUpdate->vecOrient.P, m_pLatestUpdate->vecOrient.P) / dDeltaTime;
		m_pLatestUpdate->vecAngVel.R = CalcAngDiff(m_pPreviousUpdate->vecOrient.R, m_pLatestUpdate->vecOrient.R) / dDeltaTime;
	}
	
	//Determine position and orientation errors (difference between where we've been showing our extrapolated selves until now based
	//on the previous update, and where we should be showing the object now based on this latest update)
	
	PosOrientStruct LastExtrap;   
	ExtrapPosOrientFromState(m_OurTime.GetTimeSeconds(), m_pPreviousUpdate, &LastExtrap);

	//Calculate the position error (difference between this update and where we were showing us
	CalcLLDelta(m_pLatestUpdate->vecPos, LastExtrap.vecPos, m_pLatestUpdate->vecPosErr);
	
	//Calculate the orientation error 
	m_pLatestUpdate->vecOrientErr.H = CalcAngDiff(m_pLatestUpdate->vecOrient.H, LastExtrap.vecOrient.H);
	m_pLatestUpdate->vecOrientErr.P = CalcAngDiff(m_pLatestUpdate->vecOrient.P, LastExtrap.vecOrient.P);
	m_pLatestUpdate->vecOrientErr.R = CalcAngDiff(m_pLatestUpdate->vecOrient.R, LastExtrap.vecOrient.R);
	
	return 1;
}

//Get the state as of now, which is a best guess of where the object is based on interpolating from previous updates. 1 success, 0 fail. 
int CStateInterpolater::GetStateNow(double *pdLatDegN, double *pdLonDegE, double *pdAltMSLFt, double *pdHeadingDegTrue, double *pdPitchDegPosUp, 
	double *pdRollDegPosRight)
{
	if (!m_bStateDataValid)
	{
		//Just for completeness, in case caller tries to GetStateNow before we get our first update.
		*pdLatDegN = *pdLonDegE = *pdAltMSLFt = *pdHeadingDegTrue = *pdPitchDegPosUp = *pdRollDegPosRight = 0.0; 
		return 0;
	}
	
	static PosOrientStruct P;  //static because this is called a lot, like 60HZ
	double t = m_OurTime.GetTimeSeconds();

	ExtrapPosOrientFromState(t, m_pLatestUpdate, &P);

	*pdLatDegN = P.vecPos.DegN;
	*pdLonDegE = P.vecPos.DegE;
	*pdAltMSLFt = P.vecPos.AltFtMSL;
	*pdHeadingDegTrue = P.vecOrient.H * RAD_TO_DEG;
	if (*pdHeadingDegTrue < 0.0)
		*pdHeadingDegTrue += 360.0;
	*pdPitchDegPosUp = NormalizePitch(P.vecOrient.P * RAD_TO_DEG);
	*pdRollDegPosRight = P.vecOrient.R * RAD_TO_DEG;

	return 1;
}

//Calculate the difference (north/east/up in meters) between two lat/long/alt points (pt2 minus pt1). It uses a sphere approximation 
//instead of the full WGS84 model which the lat/lon units are in, but is "accurate enough" for differences within about a couple hundred miles.
void CStateInterpolater::CalcLLDelta(LLVector &Pt1, LLVector &Pt2, PosVector &D)
{
	D.N = (Pt2.DegN - Pt1.DegN) * DEG_TO_RAD * EARTH_RADIUS_M;
	D.E = (Pt2.DegE - Pt1.DegE) * METERS_PER_DEG * cos(Pt1.DegN * DEG_TO_RAD); 
    D.U = (Pt2.AltFtMSL - Pt1.AltFtMSL) * FT_TO_M;
	return;
}

//Return the lat/lon from a given lat/lon point plus a north/east/up offset vector (Pt1 + V, result in Offset)
void CStateInterpolater::CalcLLOffset(LLVector &Pt1, PosVector &V, LLVector &Offset)
{
    Offset.DegN = Pt1.DegN + V.N / EARTH_RADIUS_M * RAD_TO_DEG;
	Offset.DegE = Pt1.DegE + V.E / (METERS_PER_DEG * cos(Pt1.DegN * DEG_TO_RAD));
	Offset.AltFtMSL = Pt1.AltFtMSL + V.U * M_TO_FT;
	return;
}

//Return the 2D magnitude (north and east only) for the given vector
double CStateInterpolater::VecMag2D(PosVector &V)
{
	return sqrt(V.N * V.N + V.E * V.E);
}

//Normalize given angle in radians into -PI to PI
double CStateInterpolater::NormalizeRads(double r)
{
	return r - TWOPI * floor(0.5 + r / TWOPI);
}

//Normalize given degrees +/- 360 to +/- 90 (meant for normalizing pitch, so e.g. +120 becomes +60)
double CStateInterpolater::NormalizePitch(double Deg)
{
	if (Deg < -270.0)
		Deg += 360.0;
	if (Deg >= 270.0)
		Deg -= 360.0;
    if (Deg <= -90.0)
		Deg = -89.999;     //to avoid gimbal lock
	if (Deg >= 90.0)
		Deg = 89.999;
	return Deg;
}
//Return the difference in radians, max +/- PI (OldRads - NewRads)  
double CStateInterpolater::CalcAngDiff(double NewRads, double OldRads)
{
	NewRads = NormalizeRads(NewRads);
	OldRads = NormalizeRads(OldRads);

	double d = OldRads - NewRads;

	if (d > PI)
		d -= TWOPI;
	else if (d < -PI)
		d += TWOPI;
	
	return d;
}
//Given a state (with our timestamp), and knowing our current time in m_OurTime, extrapolate the position and
//orientation for "now" based on that state, using its velocity and acceleration vectors, plus the error amount
//we reduce to zero over ERR_CORRECT_TIME
void CStateInterpolater::ExtrapPosOrientFromState(double dOurTimeSecs, StateStruct *pState, PosOrientStruct *pExtrap)
{
	double dDeltaTime = dOurTimeSecs - pState->dOurTimeSecs;
	double dhalfDT2 = 0.5f * dDeltaTime * dDeltaTime;
	
	//Calculate the change in position during dDeltaTime
	PosVector DeltaPos;
	DeltaPos.N = pState->vecVel.N * dDeltaTime + dhalfDT2 * pState->vecAccel.N;
	DeltaPos.E = pState->vecVel.E * dDeltaTime + dhalfDT2 * pState->vecAccel.E;
	DeltaPos.U = pState->vecVel.U * dDeltaTime + dhalfDT2 * pState->vecAccel.U;

	//Add error correction amount, reducing to zero over ERR_CORRECT_TIME 
	if (dDeltaTime < POS_CORRECT_TIME)
	{
		double ddT = 1.0 - (dDeltaTime / POS_CORRECT_TIME);
		DeltaPos.N += (ddT * pState->vecPosErr.N);
		DeltaPos.E += (ddT * pState->vecPosErr.E);
		DeltaPos.U += (ddT * pState->vecPosErr.U);
	}

	//Add to the original state and put it in pExtrap->vecPos
	CalcLLOffset(pState->vecPos, DeltaPos, pExtrap->vecPos);

	//Calculate the new orientation 
	pExtrap->vecOrient.H = pState->vecOrient.H + dDeltaTime * pState->vecAngVel.H;
	pExtrap->vecOrient.P = pState->vecOrient.P + dDeltaTime * pState->vecAngVel.P;
	pExtrap->vecOrient.R = pState->vecOrient.R + dDeltaTime * pState->vecAngVel.R;

	//Add error correction amount
	if (dDeltaTime < ORIENT_CORRECT_TIME)
	{
		double ddT = 1.0 - (dDeltaTime / ORIENT_CORRECT_TIME);
		pExtrap->vecOrient.H += (ddT * pState->vecOrientErr.H);
		pExtrap->vecOrient.P += (ddT * pState->vecOrientErr.P);
		pExtrap->vecOrient.R += (ddT * pState->vecOrientErr.R);
	}

	pExtrap->vecOrient.H = NormalizeRads(pExtrap->vecOrient.H);
	pExtrap->vecOrient.P = NormalizeRads(pExtrap->vecOrient.P);
	pExtrap->vecOrient.R = NormalizeRads(pExtrap->vecOrient.R);

	//If updates are sparse and we're moving, override smoothed heading and lock heading to movement vector
	if (dDeltaTime > 1.5 && (pState->vecVel.N > 0.1 || pState->vecVel.E > 0.1))
	{
		if (DeltaPos.N != 0.0)
			pExtrap->vecOrient.H = NormalizeRads(atan2(DeltaPos.E, DeltaPos.N));
		else if (DeltaPos.E > 0.0f)
			pExtrap->vecOrient.H = PI / 2.0;
		else
			pExtrap->vecOrient.H = -PI / 2.0;
	}

}


//Set the initial state (i.e. object was at this position and orientation at the given object's clock time).
//-9999.0 if there is not an object reported time. Pitch positive upward, roll positive going to the right 
//(looking out from the object). Return 1 if success, 0 if fail. 
int CStateInterpolater::Initialize(double dTimeSeconds, double dLatDegN, double dLonDegE, double dAltMSLFt, double dHeadingDegTrue, 
	double dPitchDegPosUp, double dRollDegPosRight, double dGroundSpeedKts)
{
	//Set the initial state
    m_State[0].dOurTimeSecs = m_OurTime.GetTimeSeconds();
	if (dTimeSeconds <= -9999.0)
	{
		m_bDerivingTimes = true;
		m_State[0].dStateTimeSecs = m_State[0].dOurTimeSecs;
	}
	else
	{
		m_bDerivingTimes = false;
		m_State[0].dStateTimeSecs = dTimeSeconds;
	}
	m_State[0].vecPos.DegN = dLatDegN;
	m_State[0].vecPos.DegE = dLonDegE;
	m_State[0].vecPos.AltFtMSL = dAltMSLFt;
	m_State[0].vecOrient.H = dHeadingDegTrue * DEG_TO_RAD;
	m_State[0].vecOrient.P = dPitchDegPosUp * DEG_TO_RAD;
	m_State[0].vecOrient.R = dRollDegPosRight * DEG_TO_RAD;
	m_State[0].vecAngVel.H = 0.0;
	m_State[0].vecAngVel.P = 0.0;
	m_State[0].vecAngVel.R = 0.0;
	m_State[0].vecAccel.N = 0.0;
	m_State[0].vecAccel.E = 0.0;
	m_State[0].vecAccel.U = 0.0;
	m_State[0].vecPosErr.N = 0.0;
	m_State[0].vecPosErr.E = 0.0;
	m_State[0].vecPosErr.U = 0.0;
	m_State[0].vecOrientErr.H = 0.0;
	m_State[0].vecOrientErr.P = 0.0;
	m_State[0].vecOrientErr.R = 0.0;
	m_State[0].dRepGroundspeedMS = dGroundSpeedKts * KTS_TO_MS;

	//Create initial velocity vector from reported groundspeed
	m_State[0].vecVel.N = cos(m_State[0].vecOrient.H) * m_State[0].dRepGroundspeedMS;
	m_State[0].vecVel.E = sin(m_State[0].vecOrient.H) * m_State[0].dRepGroundspeedMS;
	m_State[0].vecVel.U = 0.0;

	//Copy everything over to the "previous" state too
	m_pLatestUpdate = &m_State[0];
	m_pPreviousUpdate = &m_State[1];
	memcpy_s(m_pPreviousUpdate, sizeof(StateStruct), m_pLatestUpdate, sizeof(StateStruct)); 

	m_bStateDataValid = true;

	return 1;
}

