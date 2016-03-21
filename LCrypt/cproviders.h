#ifndef _cproviders_
#define _cproviders_

#include "LCore/cmain.h"
#include "LCore/carray.h"
#include "Utils/UCryptDll/tomcrypt.h"

namespace crypt
{
	// dll funcs
	typedef ltc_cipher_descriptor &		(*fn_get_cipher_descriptor) (int); 
	typedef int							(*fn_cipher_is_valid)		(int); 
	typedef int							(*fn_get_forced_key_len)	(int);
	typedef int							(*fn_ctr_start)				(int, const unsigned char *, const unsigned char *, int, int, int, symmetric_CTR *);
	typedef int							(*fn_ctr_encrypt)			(const unsigned char *, unsigned char *, unsigned long, symmetric_CTR *);
	typedef int							(*fn_ctr_decrypt)			(const unsigned char *, unsigned char *, unsigned long, symmetric_CTR *);
	typedef int							(*fn_ctr_done)				(symmetric_CTR *);

	extern fn_ctr_start			dll_ctr_start;
	extern fn_ctr_encrypt		dll_ctr_encrypt;
	extern fn_ctr_decrypt		dll_ctr_decrypt;
	extern fn_ctr_done			dll_ctr_done;


	struct AlgInfo_t
	{
		ltc_cipher_descriptor	m_tDesc;
		int						m_iKeyLength;
		int						m_iId;
	};

	typedef Array_T < AlgInfo_t > AlgList_t;

	bool LoadLibrary ();
	void FreeLibrary ();

	const AlgList_t & GetAlgList ();
	AlgInfo_t * GetAlg ( int iLTid );
}

#endif