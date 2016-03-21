#ifndef _dialogs_bookmarks_
#define _dialogs_bookmarks_

#include "pfm/main.h"
#include "pfm/std.h"

bool ShowAddBMDialog ( const Str_c & sDir );
void ShowEditBMDialog ( HWND hParent );
void SetupInitialBookmarks ();

#endif