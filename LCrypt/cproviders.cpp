#include "pch.h"

#include "LCrypt/cproviders.h"
#include "LCore/clog.h"
#include "LCore/cfile.h"


namespace crypt
{
HINSTANCE	g_hCryptLib			= NULL;
bool		g_bInitializedOk	= false;

fn_get_cipher_descriptor	dll_get_cipher_descriptor = NULL;
fn_cipher_is_valid			dll_cipher_is_valid = NULL;
fn_get_forced_key_len		dll_get_forced_key_len = NULL;
fn_ctr_start				dll_ctr_start = NULL;
fn_ctr_encrypt				dll_ctr_encrypt = NULL;
fn_ctr_decrypt				dll_ctr_decrypt = NULL;
fn_ctr_done					dll_ctr_done = NULL;

AlgList_t g_dAlgs;

// pre-s
void EnumerateAlgs ();
 
bool LoadLibrary ()
{
	if ( ! g_bInitializedOk )
	{
		if ( ! g_hCryptLib )
		{
			g_hCryptLib = ::LoadLibrary ( L"Crypt.dll" );
			if ( ! g_hCryptLib )
				return false;
		}

		dll_get_cipher_descriptor	= (fn_get_cipher_descriptor)	GetProcAddress ( g_hCryptLib, L"get_cipher_descriptor" );
		dll_cipher_is_valid			= (fn_cipher_is_valid)			GetProcAddress ( g_hCryptLib, L"cipher_is_valid" );
		dll_get_forced_key_len		= (fn_get_forced_key_len)		GetProcAddress ( g_hCryptLib, L"get_forced_key_len" );
		dll_ctr_start				= (fn_ctr_start)				GetProcAddress ( g_hCryptLib, L"ctr_start" );
		dll_ctr_encrypt				= (fn_ctr_encrypt)				GetProcAddress ( g_hCryptLib, L"ctr_encrypt" );
		dll_ctr_decrypt				= (fn_ctr_decrypt)				GetProcAddress ( g_hCryptLib, L"ctr_decrypt" );
		dll_ctr_done				= (fn_ctr_done)					GetProcAddress ( g_hCryptLib, L"ctr_done" );

		if ( ! dll_get_cipher_descriptor || ! dll_cipher_is_valid || ! dll_get_forced_key_len || ! dll_ctr_start || ! dll_ctr_encrypt || ! dll_ctr_decrypt || ! dll_ctr_done )
			return false;

		EnumerateAlgs ();

		g_bInitializedOk = true;
	}

	return true;
}

void FreeLibrary ()
{
	if ( g_hCryptLib )
		::FreeLibrary ( g_hCryptLib );
}

const AlgList_t & GetAlgList ()
{
	return g_dAlgs;
}

AlgInfo_t * GetAlg ( int iId )
{
	if ( iId < 0 || iId >= g_dAlgs.Length () )
		return NULL;

	return &g_dAlgs [iId];
}

void EnumerateAlgs ()
{
	Assert ( dll_get_cipher_descriptor && dll_cipher_is_valid );

	int nCipher = 0;
	for ( int i = 0; i < TAB_SIZE; ++i )
		if ( dll_cipher_is_valid ( i ) == CRYPT_OK )
		{
			AlgInfo_t tInfo;
			ltc_cipher_descriptor & tDesc = dll_get_cipher_descriptor ( i );
			tInfo.m_tDesc = tDesc;
			tInfo.m_iKeyLength = dll_get_forced_key_len ( nCipher );
			tInfo.m_iId = i;

			g_dAlgs.Add ( tInfo );
			++nCipher;
		}
}


} // namespace crypt