#ifndef _sconfig_
#define _sconfig_

#include "LCore/cmain.h"
#include "LParser/pvar_loader.h"

class Config_c : public VarLoader_c
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
#include "sconfig_vars.inc"

					Config_c ();

	void			Shutdown ();
};

extern Config_c g_tConfig;

#endif