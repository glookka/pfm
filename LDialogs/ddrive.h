#ifndef _ddrive_
#define _ddrive_

#include "LCore/cmain.h"
#include "LCore/cstr.h"

bool GetDriveMenuItemSize ( int iItem, SIZE & tSize );
bool DriveMenuDrawItem ( const DRAWITEMSTRUCT * pDrawItem );

// shows drive menu. returns click result
bool ShowDriveMenu ( int iX, int iY, HWND hWnd, Str_c & sPath );

#endif