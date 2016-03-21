#include "pch.h"

#include "LDialogs/dmodule.h"
#include "LDialogs/dbookmarks.h"
#include "LDialogs/dshortcut.h"
#include "LDialogs/dfind.h"

void Init_Dialogs ()
{
}


void Shutdown_Dialogs ()
{
	//
}


void SetupInitialDlgData ()
{
	SetupInitialBookmarks ();
	SetupInitialShortcuts ();
	SetupInitialFindTargets ();
}