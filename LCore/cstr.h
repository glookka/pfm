#ifndef _cstr_
#define _cstr_

#include "LCore/cmain.h"

// dynamic wchar_t string
class StringDynamic_c
{
public:
						StringDynamic_c ();
						StringDynamic_c ( const StringDynamic_c & sStr );
						StringDynamic_c ( const wchar_t * szTxt );
						StringDynamic_c ( float fValue, const wchar_t * szFormat = NULL );
						StringDynamic_c ( int iValue );
						StringDynamic_c ( wchar_t cChar );
						~StringDynamic_c ();

	const wchar_t *		c_str () const;

	wchar_t &			operator [] ( int iIndex );
	StringDynamic_c &	operator = ( const wchar_t * szTxt );
	StringDynamic_c &	operator = ( const StringDynamic_c & sStr );
	bool				operator < ( const StringDynamic_c & sStr ) const;
	StringDynamic_c &	operator += ( const wchar_t * szTxt );
	StringDynamic_c		operator + ( const wchar_t * szTxt ) const;
	bool				operator == ( const wchar_t * szTxt ) const;
	bool				operator != ( const wchar_t * szTxt ) const;
						operator const wchar_t * () const;

	int					Length () const;
	bool				Empty () const;
	StringDynamic_c		SubStr ( int iStart ) const;
	StringDynamic_c		SubStr ( int iStart, int iLength ) const;
	StringDynamic_c &	ToUpper ();
	StringDynamic_c &	ToLower();

	int					Find ( const wchar_t * szPattern, int iOffset = -1 ) const;
	int					Find ( wchar_t cPattern, int iOffset = -1 ) const;
	int					RFind ( const wchar_t * szPattern, int iOffset = -1 ) const;
	int					RFind ( wchar_t cPattern, int iOffset = -1 ) const;
	bool				Begins ( wchar_t cPrefix ) const;
	bool				Ends ( wchar_t cSuffix ) const;
	bool				Begins ( const wchar_t * szPrefix ) const;
	bool				Ends ( const wchar_t * szSuffix ) const;

	void				LTrim ();
	void				RTrim ();
	void				Trim ();
	void				Insert ( const wchar_t * szText, int iPos );
	void				Erase ( int iPos, int iLen );
	void				Replace ( wchar_t cFind, wchar_t cReplace );
	StringDynamic_c &	Chop ( int iCount );
	void				Strip ( wchar_t cChar );

private:
	static const int	MAX_FLOAT_LENGTH = 32;
	static const int	MAX_INT_LENGTH = 32;

	wchar_t *			m_szData;
	int					m_iDataSize;

	static wchar_t		m_cEmpty;

	void				Grow ( int nChars, bool bDiscard = true );
};

typedef StringDynamic_c Str_c;

// string construction helper
Str_c NewString ( const wchar_t * szFormat, ...  );

// unicode to ansi conversion
void UnicodeToAnsi ( const wchar_t * szStr, char * szResult );

// ansi to unicode conversion
void AnsiToUnicode ( const char * szStr, wchar_t * szResult );

// string guid to GUID
GUID StrToGUID ( const wchar_t * szGUID );

#endif