#include "pch.h"

#include "LCore/cbuttons.h"
#include "LCore/ctimer.h"
#include "LCore/clog.h"

#include "winuserm.h"

namespace btns
{
	struct Button_t
	{
		float	m_fLastEventTime;
		bool	m_bPressed;
	};

	const float LONG_PRESS_TIME = 0.4f;
	const int NUM_BUTTONS = 256;

	Button_t g_dButtons [NUM_BUTTONS];
	double g_fStartTime = 0.0;
	int g_iLastKeyDown = -1;
	
	Event_e	Event_Hotkey ( int iKey, DWORD uModifiers, DWORD uVirtKey )
	{
		if ( uModifiers & MOD_KEYUP )
			return Event_Keyup ( iKey );
		else
			return Event_Keydown ( iKey );

		return EVENT_NONE;
	}

	float GetEventTime ()
	{
		if ( g_fStartTime == 0.0 )
			g_fStartTime = g_Timer.GetTimeSec ();
		
		return (float) ( g_Timer.GetTimeSec () - g_fStartTime );
	}

	Event_e	Event_Keydown ( int iKey )
	{
		if ( g_iLastKeyDown == VK_ACTION && iKey == VK_RETURN )
			return EVENT_NONE;

		if ( iKey < 0 || iKey >= NUM_BUTTONS )
			return EVENT_NONE;

		if ( iKey == 91 || iKey == 132 )
			return EVENT_NONE;

		Button_t & tButton = g_dButtons [iKey];

		g_iLastKeyDown = iKey;

		bool bWasDown = tButton.m_bPressed;

		tButton.m_bPressed = true;
		tButton.m_fLastEventTime = GetEventTime ();

		if ( bWasDown )
			return EVENT_PRESS;

		return EVENT_NONE;
	}

	Event_e	Event_Keyup ( int iKey )
	{
		g_iLastKeyDown = -1;

		if ( iKey < 0 || iKey >= NUM_BUTTONS )
			return EVENT_NONE;

		if ( iKey == 91 || iKey == 132 )
			return EVENT_NONE;

		Button_t & tButton = g_dButtons [iKey];

		if ( tButton.m_bPressed )
		{
			tButton.m_bPressed = false;
			if ( GetEventTime () - tButton.m_fLastEventTime >= LONG_PRESS_TIME )
				return EVENT_LONGPRESS;
			else
				return EVENT_PRESS;
		}
		else
		{
			// TODO: this may be not the right way
			if ( iKey == VK_TSOFT1 || iKey == VK_TSOFT2 )
				return EVENT_PRESS;
			else
				return EVENT_NONE;
		}

		return EVENT_NONE;
	}

	Event_e Event_Timer (  int & iKey )
	{
		for ( int i = 0; i < NUM_BUTTONS; ++i )
		{
			Button_t & tButton = g_dButtons [i];
			if ( GetEventTime () - tButton.m_fLastEventTime >= LONG_PRESS_TIME && tButton.m_bPressed )
			{
				iKey = i;
				tButton.m_bPressed = false;
				return EVENT_LONGPRESS;
			}
		}

		return EVENT_NONE;
	}

	void Reset ()
	{
		g_iLastKeyDown = -1;
		memset ( g_dButtons, 0, sizeof ( g_dButtons ) );
	}
}

