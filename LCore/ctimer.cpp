#include "pch.h"

#include "LCore/ctimer.h"

Timer_c g_Timer;


Timer_c::Timer_c ()
{
	LARGE_INTEGER iTmpTime;
	QueryPerformanceFrequency ( &iTmpTime );
	m_iFreq = iTmpTime.QuadPart;
}


__int64 Timer_c::GetTime ()
{
	LARGE_INTEGER iTmpTime;
	QueryPerformanceCounter ( &iTmpTime );
	return iTmpTime.QuadPart;
}


double Timer_c::GetTimeSec ()
{
	LARGE_INTEGER iTmpTime;
	QueryPerformanceCounter ( &iTmpTime );
	return double ( iTmpTime.QuadPart ) / double ( m_iFreq );
}