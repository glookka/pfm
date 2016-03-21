#ifndef _filemanager_
#define _filemanager_

#include "LCore/cmain.h"
#include "LCore/cstr.h"

class Panel_c;

const float FM_TIMER_PERIOD_MS = 200;

void FMActivatePanel ( Panel_c * pPanel );			// activate a given panel, deactivate second
void FMMaximizePanel ( Panel_c * pPanel );			// maximize a panel
void FMNormalPanels ();								// all panels have equal size
void FMUpdateSplitterThickness ();					// update thickness
void FMSplitterMoved ();							// slider signals a move event
void FMUpdatePanelRects ();							// update panel rects
void FMRepositionPanels ( bool bInitial = false );	// reposition panels
bool FMMarkMode ();									// is selection mode on?
void FMSetFastNavMode ( bool bFastNav );
void FMSetMarkMode ( bool bMark );					// set selection mode. update toolbar
void FMSetFullscreen ( bool bFullscreen );
void FMToggleFullscreen ();

void FMFolderChange_Event ();
void FMSelectionChange_Event ();
void FMCursorChange_Event ();
void FMExecute_Event ();

Str_c FMGetVersion ();

bool FMCalcWindowRect ( RECT & tFinalRect, bool bDialog, bool bTabbed = false );

Panel_c * FMGetPanel1 ();
Panel_c * FMGetPanel2 ();

Panel_c * FMGetActivePanel ();
Panel_c * FMGetPassivePanel ();

void FMLoadColorScheme ( const Str_c & sScheme );

#endif