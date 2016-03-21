#ifndef _ckey_
#define _ckey_

#include "LCore/cmain.h"
#include "LCore/cstr.h"

namespace reg
{
	const int RAW_KEY_LEN = 8;
	const int HASH_SIZE_BYTES = 16;			// hash result size ( in bytes )
	const int BIT_TABLE_SIZE = 131072;		// bit table size ( in bits )

	int BitsToKey ( const unsigned char * dBits, int nBits, char * dKey );
	int KeyToBits ( const char * dKey, int iLen, unsigned char * dBits );

	char ToLetter ( unsigned char iCode );
	unsigned char ToBits ( char iChar );

//	void FormatKey ( StrAnsi_c & sKey, int nGroupLetters );
	void UnformatKey ( Str_c & sKey );

	unsigned int HashToIndex ( unsigned char * dHash, int nBytes );
	void InvertBits ( unsigned char & uByte );
}

#endif