#ifndef _dapps_
#define _dapps_

#include "LCore/cmain.h"
#include "LCore/cstr.h"

void ShowOpenWithDialog ( const Str_c & sFileName );
int ShowSelectAppDialog ( HWND hParent );

bool ShowRunParamsDialog ( Str_c & sPath, Str_c & sParams );

#endif