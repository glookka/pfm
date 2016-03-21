#ifndef _fclipboard_
#define _fclipboard_

#include "LCore/cmain.h"
#include "LPanel/pbase.h"

namespace clipboard
{
	void 	Copy ( Panel_c * pPanel );
	void	Cut ( Panel_c * pPanel );
	void	Paste ( const wchar_t * szDestDir );
	bool	IsEmpty ();
	bool	IsMoveMode ();
}

#endif
