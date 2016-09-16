//THIS IS A SHARED FILE
//BE CAREFUL ABOUT CHANGING!

//This class is a simple timer that reports high-precison time (in seconds) since 
//it was instantiated. There's no CPP file because it's pretty simple.

#pragma once

typedef class CTime
{
	public:
	CTime::CTime()
	{
		//In Windows there is almost certainly a high resolution multimedia timer, but 
		//according to documentation it's (slightly) possible there isn't.
		//If there is one we use it (m_lHaveTimer > 0), or if not we default to the low-res GetTickCount().
		LARGE_INTEGER Freq;
		m_lHaveTimer = QueryPerformanceFrequency(&Freq);
		if (m_lHaveTimer && Freq.QuadPart == 0) 
			m_lHaveTimer = 0;
		if (m_lHaveTimer)
		{
			LARGE_INTEGER Ticks;
			m_dHighPerfFreq = 1.0 / (double)Freq.QuadPart;  
			QueryPerformanceCounter(&Ticks);
			m_dSysCreationTime = (double)(Ticks.QuadPart) * m_dHighPerfFreq;   
		}
		else
			m_dSysCreationTime = GetTickCount64() / 1000.0;
	}
	
	//Return the current "time" (number of seconds since instantiation).
	double GetTimeSeconds()
	{
		if (m_lHaveTimer)
		{
			LARGE_INTEGER Ticks;
			QueryPerformanceCounter(&Ticks);
			return ((double)(Ticks.QuadPart) * m_dHighPerfFreq - m_dSysCreationTime);
		}
		else
		{
			return ((double)(GetTickCount64()) / 1000.0 - m_dSysCreationTime);
		}
	}
	
	protected:

	long    m_lHaveTimer;		 //1 if computer has multimedia timer, 0 if not
	double	m_dHighPerfFreq;     //timer frequency
	double  m_dSysCreationTime;  //System time when we were instantiated (time zero), so we can report time since then
} CTime;




