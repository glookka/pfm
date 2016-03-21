#ifndef _pluginstd_
#define _pluginstd_

#pragma warning ( disable : 4201 )
#pragma warning ( disable : 4214 )

#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <commctrl.h>

#pragma warning ( default : 4214 )
#pragma warning ( default : 4201 )

#include "aygshell.h"
#include "pluginapi.h"

#ifdef _DEBUG
	void Warning ( const wchar_t * szFormat, ... );

	#define Assert(_expr) \
		{ \
		if ( !(_expr) ) \
			{ \
			wchar_t szFile [256];\
			wchar_t szExpr [256];\
			MultiByteToWideChar ( CP_ACP, 0, __FILE__, -1, szFile, 512 );\
			MultiByteToWideChar ( CP_ACP, 0, #_expr, -1, szExpr, 512 );\
			Warning ( L"Assert failed: %s\n%s, %i\n", szExpr, szFile, __LINE__ ); \
			DebugBreak (); \
			} \
		}\

#else

	#define Assert(_expr) (__assume(_expr))
	#define Warning __noop

#endif

#define SafeDelete(p) { if(p) { delete (p); (p)=NULL; } }
#define SafeDeleteArray(p) { if(p) { delete[] (p); (p)=NULL; } }

//////////////////////////////////////////////////////////////////////////////////////////
// common stuff
template < typename T >
inline T Clamp ( T value, T minValue, T maxValue )
{
	if ( value < minValue )
		return minValue;

	if ( value > maxValue )
		return maxValue;

	return value;
}


template < typename T, typename U >
inline void Swap ( T& v1, U& v2 )
{
	T temp;
	temp = v1;
	v1 = T (v2);
	v2 = U (temp);
}


template < typename T >
inline bool Odd ( T value )
{
	return value & 1 ? true : false;
}


template < typename T1, typename T2 >
inline T2 Round ( T1 value )
{
	return value > T1 ( 0.0 ) ? T2 ( value + static_cast < T1 >( 0.5 ) ) : T2 ( value - static_cast < T1 > ( 0.5 ) );
}


template < typename T >
inline T Sqr ( T value )
{
	return value * value;
}


template < typename T1, typename T2 >
inline T1 Min ( const T1 & v1, const T2 & v2 )
{
	return ( v1 < v2 ) ? v1 : v2;
}


template < typename T1, typename T2, typename T3 >
inline T1 Min ( const T1 & v1, const T2 & v2, const T3 & v3 )
{
	return Min ( Min ( v1, v2 ), v3 );
}


template < typename T1, typename T2, typename T3, typename T4 >
inline T1 Min ( const T1 & v1, const T2 & v2, const T3 & v3, const T4 & v4 )
{
	return Min ( Min ( v1, v2 ), Min ( v3, v4 ) );
}


template < typename T1, typename T2, typename T3, typename T4, typename T5 >
inline T1 Min ( const T1 & v1, const T2 & v2, const T3 & v3, const T4 & v4, const T5 & v5 )
{
	return Min ( Min ( v1, v2 ), Min ( v3, v4 ), v5 );
}


template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 >
inline T2 Min ( const T1 & v1, const T2 & v2, const T3 & v3, const T4 & v4, const T5 & v5, const T6 & v6 )
{
	return Min ( Min ( v1, v2 ), Min ( v3, v4 ), Min ( v5, v6 ) );
}


template < typename T1, typename T2 >
inline T1 Max ( const T1 & v1, const T2 & v2 )
{
	return ( v1 > v2 ) ? v1 : v2;
}


template < typename T1, typename T2, typename T3 >
inline T1 Max ( const T1 & v1, const T2 & v2, const T3 & v3 )
{
	return Max ( Max (v1, v2), v3 );
}


template < typename T1, typename T2, typename T3, typename T4 >
inline T1 Max ( const T1 & v1, const T2 & v2, const T3 & v3, const T4 & v4 )
{
	return Max ( Max ( v1, v2 ), Max ( v3, v4 ) );
}


template < typename T1, typename T2, typename T3, typename T4, typename T5 >
inline T1 Max ( const T1 & v1, const T2 & v2, const T3 & v3, const T4 & v4, const T5 & v5 )
{/
return Max ( Max ( v1, v2 ), Max ( v3, v4 ), v5 );
}


template < typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 >
inline T1 Max ( const T1 & v1, const T2 & v2, const T3 & v3, const T4 & v4, const T5 & v5, const T6 & v6 )
{
	return Max ( Max ( v1, v2 ), Max ( v3, v4 ), Max ( v5, v6 ) );
}

const wchar_t *	LocalStr ( int iString );
const wchar_t * Txt ( int iString );
void			DlgTxt ( HWND hDlg, int iControl, int iString );
void			SetToolbarText ( HWND hToolbar, int iId, const wchar_t * szText );
int				GetColumnWidthRelative ( float fWidth );
void			AnsiToUnicode ( const char * szStr, wchar_t * szResult );

#undef TRY_FUNC
#define TRY_FUNC(var,func,val,file1,file2) \
{\
	int iErrResponse = ER_SKIP;\
	do\
	{\
		var = func;\
		if ( var == val )\
			iErrResponse = g_PSI.m_fnErrShowErrorDlg ( m_hWnd, GetLastError (), true, file1, file2 );\
	}\
	while ( var == val && iErrResponse == ER_RETRY );\
	if ( var == val )\
		switch ( iErrResponse )\
		{\
			case ER_CANCEL:	return RES_CANCEL;\
			case ER_SKIP:	return RES_SKIP;\
			default:		return RES_ERROR;\
		}\
	return iErrResponse == ER_CANCEL ? RES_CANCEL : RES_ERROR;\
}\

#endif
