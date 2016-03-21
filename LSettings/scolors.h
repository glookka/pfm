#ifndef _LSettings_colors_
#define _LSettings_colors_

#include "LCore/cmain.h"
#include "LParser/pvar_loader.h"

class Colors_c : public VarLoader_c
{
public:
#ifdef VAR
#undef VAR
#endif

#ifdef COMMENT
#undef COMMENT
#endif

#define VAR(type,var_name,value) static type var_name;
#define COMMENT(value)
#include "scolors.inc"

					Colors_c ();

	void			Shutdown ();
};

extern Colors_c g_tColors;

#endif