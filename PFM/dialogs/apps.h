#ifndef _dialogs_apps_
#define _dialogs_apps_

#include "pfm/main.h"
#include "pfm/std.h"

void	OpenWithDlg ( const Str_c & sFileName );
int		SelectAppDlg ( HWND hParent );
bool	RunParamsDlg ( Str_c & sPath, Str_c & sParams );

#endif