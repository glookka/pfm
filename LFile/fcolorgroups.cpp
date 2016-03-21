#include "pch.h"

#include "LFile/fcolorgroups.h"
#include "LSettings/sconfig.h"
#include "LSettings/scolors.h"
#include "LParser/pcommander.h"

ColorGroup_t g_tCurrentColorGroup;
Array_T <ColorGroup_t>  g_dColorGroups;

// group start command
CMD_BEGIN ( cg_start, 0 )
{
	g_tCurrentColorGroup = ColorGroup_t ();
	g_tCurrentColorGroup.m_id = g_dColorGroups.Length ();
}
CMD_END

// device flag
CMD_BEGIN ( cg_device, 0 )
{
	g_tCurrentColorGroup.m_bDevice = true;
}
CMD_END


// included/excluded attributes
CMD_BEGIN ( cg_attributes, 2 )
{
	g_tCurrentColorGroup.m_dwIncludeAttributes = tArgs.GetDWORD ( 0 );
	g_tCurrentColorGroup.m_dwExcludeAttributes = tArgs.GetDWORD ( 1 );
}
CMD_END

// include mask
CMD_BEGIN ( cg_ext_mask, 1 )
{
	g_tCurrentColorGroup.m_tFilter.Set ( tArgs.GetString ( 0 ) );
}
CMD_END

// selected/normal colors
CMD_BEGIN ( cg_color, 2 )
{
	g_tCurrentColorGroup.m_uColor 			= tArgs.GetDWORD ( 0 );
	g_tCurrentColorGroup.m_uSelectedColor 	= tArgs.GetDWORD ( 1 );
}
CMD_END

// end group
CMD_BEGIN ( cg_end, 0 )
{
	g_dColorGroups.Add ( g_tCurrentColorGroup );
}
CMD_END


void LoadColorGroups ( const wchar_t * szFileName )
{
	g_dColorGroups.Clear ();
	Commander_c::Get ()->ExecuteFile ( szFileName );
}


void RegisterColorGroupCommands ()
{
	CMD_REG ( cg_start );
	CMD_REG ( cg_device );
	CMD_REG ( cg_attributes );
	CMD_REG ( cg_ext_mask );
	CMD_REG ( cg_color );
	CMD_REG ( cg_end );
}


const ColorGroup_t * GetColorGroup ( const FileInfo_t & tInfo )
{
	for ( int i = 0; i < g_dColorGroups.Length (); ++i )
	{
		const ColorGroup_t * pGroup = & ( g_dColorGroups [i] );
		Assert ( pGroup );

		bool bDevice = !! ( tInfo.m_uFlags & FLAG_DEVICE ) == pGroup->m_bDevice;
		bool bAttrib = ! pGroup->m_dwIncludeAttributes || ( tInfo.m_tData.dwFileAttributes & pGroup->m_dwIncludeAttributes );
		bool bExAttrib = ! pGroup->m_dwExcludeAttributes || ( tInfo.m_tData.dwFileAttributes & ~(pGroup->m_dwExcludeAttributes) );

		if ( bDevice && bAttrib && bExAttrib && pGroup->m_tFilter.Fits ( tInfo.m_tData.cFileName ) )
			return pGroup;
	}

	return NULL;
}


DWORD CGGetFileColor ( const FileInfo_t & tInfo, bool bSelected )
{
	if ( tInfo.m_uFlags & FLAG_MARKED )
		return g_tColors.pane_font_marked;

	if ( bSelected && g_tConfig.pane_cursor_bar )
		return tInfo.m_pCG ? tInfo.m_pCG->m_uSelectedColor : g_tColors.pane_font_selected;

	return tInfo.m_pCG ? tInfo.m_pCG->m_uColor : g_tColors.pane_font;
}

int CompareColorGroups ( const ColorGroup_t * pCG1, const ColorGroup_t * pCG2 )
{
	if ( pCG1 )
	{
		if ( ! pCG2 )
			return 1;
	}
	else
	{
		if ( pCG2 )
			return -1;
		else
			return 0;
	}

	Assert ( pCG1 && pCG2 );
	return pCG1->m_id < pCG2->m_id ? -1 : ( pCG1->m_id > pCG2->m_id ? 1 : 0 );
}