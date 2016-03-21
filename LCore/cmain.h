#ifndef _cmain_
#define _cmain_

#pragma warning( disable : 4291 )		// we don't use exceptions, so who cares
#pragma warning( disable : 4018 )		// signed/unsigned mismatch

#define SafeDelete(p) { if(p) { delete (p); (p)=NULL; } }
#define SafeDeleteArray(p) { if(p) { delete[] (p); (p)=NULL; } }
#define GET_X_LPARAM(lParam)	((int)(short)LOWORD(lParam))
#define GET_Y_LPARAM(lParam)	((int)(short)HIWORD(lParam))

#include "pch.h"

// the proper for
#undef for
#define for if (0); else for

#include "LCore/cdefines.h"
#include "LCore/cmemory.h"

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


///////////////////////////////////////////////////////////////////////////////////////////
// min/max
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


inline bool IsInRect ( int iX, int iY, const RECT & tRect )
{
	return iX >= tRect.left && iX <= tRect.right && iY >= tRect.top && iY <= tRect.bottom;
}


inline void CollapseRect ( RECT & tRect, int iCollapse )
{
	tRect.top += iCollapse;
	tRect.bottom -= iCollapse;
	tRect.left += iCollapse;
	tRect.right -= iCollapse;
}

#endif