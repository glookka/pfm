#include "pch.h"

#include "LSettings/sconfig.h"

Config_c g_tConfig;

// define static vars
#ifdef VAR
#undef VAR
#endif

#ifdef COMMENT
#undef COMMENT
#endif

#define COMMENT(val)
#define VAR(type,var_name,value) type Config_c::var_name = value;
#include "sconfig_vars.inc"

///////////////////////////////////////////////////////////////
// config
Config_c::Config_c ()
{
// register vars
#ifdef VAR
#undef VAR
#endif

#ifdef COMMENT
#undef COMMENT
#endif

#define VAR(type,var_name,value) RegisterVar ( L#var_name, &var_name );
#define COMMENT(value) AddComment (value);
#include "sconfig_vars.inc"
}


void Config_c::Shutdown ()
{
	ResetRegistered ();
}