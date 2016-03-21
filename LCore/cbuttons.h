#ifndef _cbuttons_
#define _cbuttons_

#include "LCore/cmain.h"

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