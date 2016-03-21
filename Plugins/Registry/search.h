#ifndef _registry_search_
#define _registry_search_

#include <windows.h>
#include <commctrl.h>
#include <aygshell.h>

#include "../../pfm/pluginapi/pluginapi.h"

class RegPlugin_c;
bool FindRequestDlg ( PanelItem_t ** dItems, int nItems );
bool FindProgressDlg ( 	RegPlugin_c * pPlugin, const wchar_t * szPath, PanelItem_t ** dItems, int nItems );

#endif