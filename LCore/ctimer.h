#ifndef _ctimer_
#define _ctimer_

#include "LCore/cmain.h"

class Timer_c
{
public:
					Timer_c ();

	double			GetTimeSec ();

private:
	__int64			m_iFreq;
	__int64			m_iStartTime;

	__int64			GetTime();
};

extern Timer_c g_Timer;

#endif