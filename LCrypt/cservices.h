#ifndef _cservices_
#define _cservices_

#include "LCore/cmain.h"
#include "LCore/cstr.h"

namespace reg
{
	void			Init ();

	bool			CheckKeyValidity ();
	bool			GetDeviceOwner ( Str_c & sOwner );

	void			StoreKey ( const Str_c & sCode );
	bool			StoreKeyToRegistry ();

	bool 			CheckDateValidity ();

	bool			WriteDate ();

	void			EncryptSymmetric ( unsigned char * dData, int iDataSize, unsigned char * dKey, int iKeySize );
	void			DecryptSymmetic ( unsigned char * dData, int iDataSize, unsigned char * dKey, int iKeySize );

	extern int		interval;

	extern bool		rlang;
	extern int		days_left;
	extern bool		registered;
	extern bool		expired;
}

#endif