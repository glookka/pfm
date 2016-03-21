#ifndef _dialogs_bmmenu_
#define _dialogs_bmmenu_

#include "pfm/main.h"
#include "pfm/std.h"

bool GetDriveMenuItemSize ( int iItem, SIZE & tSize );
bool DriveMenuDrawItem ( const DRAWITEMSTRUCT * pDrawItem );
bool IsDriveMenuItem ( int iItem );

// shows drive menu. returns click result
bool ShowDriveMenu ( int iX, int iY, HWND hWnd, Str_c & sPath );

#endif