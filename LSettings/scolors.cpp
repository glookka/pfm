#include "pch.h"

#include "LSettings/scolors.h"

Colors_c g_tColors;

// define static vars
#ifdef VAR
#undef VAR
#endif

#ifdef COMMENT
#undef COMMENT
#endif

#define COMMENT(val)
#define VAR(type,var_name,value) type Colors_c::var_name = value;
#include "scolors.inc"

///////////////////////////////////////////////////////////////
// colors
Colors_c::Colors_c ()
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
#include "scolors.inc"
}


void Colors_c::Shutdown ()
{
	ResetRegistered ();
}