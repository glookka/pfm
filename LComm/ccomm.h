#ifndef _ccomm_
#define _ccomm_

#include "LCore/cmain.h"

#include "winsock2.h"

class WindowNotifier_c
{
public:
					WindowNotifier_c ();

	void			SetWindow ( HWND hWnd );

protected:
	HWND			m_hWnd;
};


void CheckBTStacks ();

bool IsWidcommBTPresent ();
bool IsMSBTPresent ();

bool Init_WidcommBT ();
void Shutdown_WidcommBT ();

bool Init_MsBT ();
void Shutdown_MsBT ();

bool Init_Sockets ();
void Shutdown_Sockets ();

#endif