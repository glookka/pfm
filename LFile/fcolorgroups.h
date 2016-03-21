#ifndef _fcolorgroups_
#define _fcolorgroups_

#include "LCore/cmain.h"
#include "LFile/ffilter.h"
#include "LFile/fsorter.h"

// a file color group
struct ColorGroup_t
{
	ColorGroup_t ()
		: m_dwIncludeAttributes ( 0 )
		, m_dwExcludeAttributes	( 0 )
		, m_uColor				( 0 )
		, m_uSelectedColor		( 0 )
		, m_bDevice				( false )
		, m_id					( 0 )
	{
	}

	DWORD			m_dwIncludeAttributes;
	DWORD			m_dwExcludeAttributes;
	ExtFilter_c		m_tFilter;
	DWORD			m_uColor;
	DWORD			m_uSelectedColor;
	bool			m_bDevice;
	int				m_id;
};


// return this file's color
DWORD CGGetFileColor ( const FileInfo_t & tInfo, bool bSelected );

// return a color group for this file
const ColorGroup_t * GetColorGroup ( const FileInfo_t & tInfo );

// registers color group commands
void RegisterColorGroupCommands ();

// load color groups
void LoadColorGroups ( const wchar_t * szFileName );

// compare two color groups
int CompareColorGroups ( const ColorGroup_t * pCG1, const ColorGroup_t * pCG2 );

#endif