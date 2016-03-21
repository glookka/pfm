#include <windows.h>
#include "tomcrypt.h"

int get_forced_key_len ( int idx )
{
	switch ( idx )
	{
	case 0: return 32;	//aes
	case 1:	return 32;	//blowfish
	case 2:	return 32;	//twofish
	case 3:	return 8;	//des
	case 4:	return 24;	//des3
	case 5:	return 16;	//cast5
	case 6:	return 16;	//rc2
	case 7:	return 16;	//rc5
	case 8:	return 16;	//rc6
	}

	return 0;
}

ltc_cipher_descriptor & get_cipher_descriptor ( int idx )
{
	return cipher_descriptor [idx];
}

BOOL WINAPI DllMain ( HANDLE hinstDLL, DWORD dwReason, LPVOID lpvReserved )
{
	// register all available ciphers
	register_cipher (&aes_desc);
	register_cipher (&blowfish_desc);
	register_cipher (&twofish_desc);
	register_cipher (&des_desc);
	register_cipher (&des3_desc);
	register_cipher (&cast5_desc);
	register_cipher (&rc2_desc);
	register_cipher (&rc5_desc);
	register_cipher (&rc6_desc);

	return TRUE;
}
