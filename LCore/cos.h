#ifndef _cos_
#define _cos_

#include "LCore/cmain.h"
#include "LCore/cstr.h"

enum WinCeVersion_e
{
	 WINCE_2003
	,WINCE_2003SE
	,WINCE_50
	,WINCE_UNKNOWN
};

WinCeVersion_e	GetOSVersion ();
bool 			IsVGAScreen ();
int				GetVGAScale ();
void 			GetScreenResolution ( int & iWidth, int & iHeight );

typedef BOOL (__stdcall *UnregisterFunc1Proc)( UINT, UINT );
extern UnregisterFunc1Proc UndergisterFunc;

void			LoadExtraOSFuncs ();
void			UnloadExtraOSFuncs ();

bool			CanChangeOrientation ();
int				GetDisplayOrientation ();
void			ChangeDisplayOrientation ( int iAngle );

Str_c	GetWinDir ( HWND hWnd );

Str_c 	GetWinErrorText ( int iErrorCode );

#endif