#ifndef _protection_
#define _protection_

#include "main.h"
#include "std.h"

namespace protection
{
	void			Init ();
	void			Shutdown ();

	bool			CheckKey ();
	bool 			CheckDate ();
	bool			CheckRus ();

	extern int		interval;
	extern bool		rlang;
	extern bool		nag_shown;
	extern int		days_left;
	extern bool		registered;
	extern bool		expired;
	extern Str_c	key;
}

#endif