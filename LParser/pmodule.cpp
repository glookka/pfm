#include "pch.h"

#include "LParser/pmodule.h"
#include "LParser/pparser.h"
#include "LParser/pcommander.h"

void Init_Parser ()
{
	Parser_c::Init ();
	Commander_c::Init ();
}


void Shutdown_Parser ()
{
	Parser_c::Shutdown ();
	Commander_c::Shutdown ();
}