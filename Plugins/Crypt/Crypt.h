#ifndef _crypt_crypt_
#define _crypt_crypt_

#include "resource.h"
#include "../../pfm/pluginapi/pluginstd.h"

#include "cryptlib/tomcrypt.h"

const int MAX_PWD_CHARS = 128;

enum LocString_e
{
#include "loc/locale.inc"
	TOTAL
};

struct AlgInfo_t
{
	ltc_cipher_descriptor	m_tDesc;
	int						m_iKeyLength;
	int						m_iId;
};

struct Config_t
{
	int crypt_cipher;
	int crypt_delete;
	int crypt_overwrite;

	Config_t ();
};

enum
{
	 EC_ALREADY_ENCRYPTED = EC_LAST + 1
	,EC_NOT_ENCRYPTED
	,EC_CORRUPT_HEADER
	,EC_INVPASS
	,EC_READ
	,EC_WRITE
	,EC_ENCRYPT
	,EC_DECRYPT
};

typedef void (*MarkCallback_t) ( const wchar_t *, bool );

extern Config_t g_Cfg;

const AlgInfo_t *	GetAlgList ();
int					GetNumAlgs ();
void				Help ( const wchar_t * szSection );

#endif
