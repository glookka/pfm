#include "pch.h"

#include "LCore/cmodule.h"
#include "LCore/clog.h"


void Init_Core ()
{
	// randomization
	LARGE_INTEGER uLarge;
	QueryPerformanceCounter ( &uLarge);
	DWORD uSeed = uLarge.LowPart ^ uLarge.HighPart;
	srand ( uSeed );
}


void Shutdown_Core ()
{
	Shutdown_Memory ();

	LogMemLeaks ( false );
}