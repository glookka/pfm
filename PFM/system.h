#ifndef _cos_
#define _cos_

#include "main.h"
#include "std.h"

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

Str_c 			GetExecutablePath ();
Str_c			GetWinDir ( HWND hWnd );
Str_c 			GetWinErrorText ( int iErrorCode );

// buttons
namespace btns
{
	enum Event_e
	{
		 EVENT_NONE
		,EVENT_PRESS
		,EVENT_LONGPRESS
	};

	Event_e	Event_Hotkey ( int iKey, DWORD uModifiers, DWORD uVirtKey );
	Event_e	Event_Keydown ( int iKey );
	Event_e Event_Keyup ( int iKey );
	Event_e Event_Timer ( int & iKey );

	void Reset ();
}

#endif
