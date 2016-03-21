#ifndef _registry_registry_
#define _registry_registry_

#include "resource.h"
#include "../../pfm/pluginapi/pluginstd.h"

const int MAX_REG_PATH = 1024;
const int MAX_RECENT_TEXTS = 10;
const int MAX_SEARCH = 256;
const int MAX_DATA_LEN = 16384;

enum LocString_e
{
#include "loc/locale.inc"
	TOTAL
};

struct RegRoot_t
{
	const wchar_t * m_szName;
	const wchar_t * m_szShortName;
	HKEY			m_hKey;
};

const int REG_ROOT_ITEMS = 4;
extern RegRoot_t g_RegRoot [REG_ROOT_ITEMS];


//////////////////////////////////////////////////////////////////////////
// registry plugin interface
class RegPlugin_c
{
public:
	static HIMAGELIST			m_hImageList;
	static HANDLE				m_hCfg;
	static PanelView_t			m_BriefView;
	static PanelView_t			m_MediumView;
	static PanelView_t			m_FullView;
	static ColumnType_e			m_dFileInfoCols [1];


						RegPlugin_c ();
						~RegPlugin_c ();

	static void			Init ();
	static void			Shutdown ();

	bool				PSetDirectory ( const wchar_t * szDir );
	bool				PGetFindData ( PanelItem_t * & dItems, int & nItems );
	void				PFreeFindData ( PanelItem_t * dItems, int nItems );
	bool				PDeleteFiles ( PanelItem_t ** dItems, int nItems );
	bool				PFindFiles ( PanelItem_t ** dItems, int nItems );

	const wchar_t *		GetPath () const;

	static void			LoadImageList ();
	static void			DestroyImageList ();

	static const wchar_t *	LocalStr ( int iString );

private:
	static bool			m_bInitialized;
	static wchar_t *	m_pFolderColumns [2];

	wchar_t				m_szPath [MAX_REG_PATH];

	void				SetRootItem ( PanelItem_t * dItems, int nItem, const wchar_t * szName );
	wchar_t **			CreateColumnData ( DWORD uType, BYTE * pData, DWORD uDataLen );
};


struct Config_t
{
	int find_area;
	int find_case;
	int find_key_names;
	int find_value_names;
	int find_data_sz;
	int find_data_multisz;
	int find_data_dword;
	int find_data_binary;
	int find_bin_mode;

	Config_t ();
};

extern Config_t g_Cfg;

void			FindTextAdd ( const wchar_t * szText );
const wchar_t * FindTextGet ( int iId );

void	Help ( const wchar_t * szSection );
HKEY	GetRootKey ( const wchar_t * szPath );
void	ToShortPath ( wchar_t * szShort, const wchar_t * szFull );
int		GetIconByType ( DWORD uType, bool bKey );

#endif