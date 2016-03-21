#ifndef _dbookmarks_
#define _dbookmarks_

#include "LCore/cmain.h"
#include "LCore/cstr.h"

bool ShowAddBMDialog ( const Str_c & sDir );
void ShowEditBMDialog ( HWND hParent );
void SetupInitialBookmarks ();

#endif