#ifndef _fclipboard_
#define _fclipboard_

#include "pfm/main.h"
#include "pfm/panel.h"

namespace clipboard
{
	void 	Copy ( Panel_c * pPanel );
	void	Cut ( Panel_c * pPanel );
	void	Paste ( const wchar_t * szDestDir );
	bool	IsEmpty ();
	bool	IsMoveMode ();
}

#endif
