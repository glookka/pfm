#ifndef _plugins_
#define _plugins_

#include "main.h"
#include "parser.h"
#include "pluginapi/pluginapi.h"

enum
{
	 PLUGIN_ROOT_FS		= 1 << 0
	,PLUGIN_MENU		= 1 << 1
};


// todo: привести к единому стандарту
class Plugin_c
{
	friend class PluginManager_c;
	typedef HANDLE	(*OpenPlugin_fn)			();
	typedef void	(*ClosePlugin_fn)			( HANDLE );
	typedef void	(*ExitPFM_fn)				();
	typedef bool	(*GetFindData_fn)			( HANDLE, PanelItem_t * &, int & );
	typedef void	(*FreeFindData_fn)			( HANDLE, PanelItem_t *, int );
	typedef bool	(*SetDirectory_fn)			( HANDLE, const wchar_t * );
	typedef void	(*GetPluginInfo_fn)			( PluginInfo_t & Info );
	typedef void	(*SetStartupInfo_fn)		( const PluginStartupInfo_t & );
	typedef bool	(*DeleteFiles_fn)			( HANDLE hPlugin, PanelItem_t ** dItems, int nItems );
	typedef bool	(*FindFiles_fn)				( HANDLE hPlugin, PanelItem_t ** dItems, int nItems );
	typedef void	(*HandleMenu_fn)			( int iMenuItem, const wchar_t * szRoot, PanelItem_t ** dItems, int nItems );

public:
								Plugin_c ( const wchar_t * szPath, const wchar_t * szDll, DWORD uFlags );
								~Plugin_c ();

	HANDLE						OpenFilePlugin ( const wchar_t * szPath );
	const PluginInfo_t *		GetPluginInfo ();
	void						ClosePlugin ( HANDLE hPlugin );
	bool						GetFindData ( HANDLE hPlugin, PanelItem_t * & dItems, int & nItems );
	void						FreeFindData ( HANDLE hPlugin, PanelItem_t * dItems, int nItems);
	bool						SetDirectory ( HANDLE hPlugin, const wchar_t * szDir );
	ItemCompare_t				GetCompareFunc () const;
	bool						DeleteFiles ( HANDLE hPlugin, PanelItem_t ** dItems, int nItems );
	bool						FindFiles ( HANDLE hPlugin, PanelItem_t ** dItems, int nItems );

	void						HandleMenu ( int iSubmenuId, const wchar_t * szRoot, PanelItem_t ** dItems, int nItems );

	DWORD						GetFlags () const;
	const wchar_t *				GetDLLName () const;

	const wchar_t *				GetRootFSName () const;
	const wchar_t *				GetRootFSShortcut () const;

	const wchar_t *				GetMenuName () const;
	const wchar_t *				GetSubmenuName ( int iSubmenu ) const;
	int							GetNumSubmenuItems () const;

private:
	static PluginStartupInfo_t	m_PluginStartupInfo;
	static PluginInfo_t			m_PluginInfo;

	Str_c						m_sPath;
	Str_c						m_sDll;
	DWORD						m_uFlags;

	// RootFS-specific
	Str_c						m_sRootFSShortcut;
	Str_c						m_sRootFSName;

	// menu-specific
	Str_c						m_sSubmenu;
	Array_T <Str_c>				m_dSubmenu;

	HINSTANCE					m_hLibrary;

	Array_T	<const wchar_t *>	m_dStrings;

	OpenPlugin_fn				m_fnOpenPlugin;
	GetPluginInfo_fn			m_fnGetPluginInfo;
	ClosePlugin_fn				m_fnClosePlugin;
	ExitPFM_fn					m_fnExitPFM;
	GetFindData_fn				m_fnGetFindData;
	FreeFindData_fn				m_fnFreeFindData;
	SetDirectory_fn				m_fnSetDirectory;
	ItemCompare_t 				m_fnCompare;
	SetStartupInfo_fn			m_fnSetStartupInfo;
	DeleteFiles_fn				m_fnDeleteFiles;
	FindFiles_fn				m_fnFindFiles;
	HandleMenu_fn				m_fnHandleMenu;

	void						LoadPluginLibrary ();
};


class PluginManager_c
{
public:
								PluginManager_c ();

	void						Init ();
	void						Shutdown ();

	HANDLE						OpenFilePlugin ( const wchar_t * szPath );
	void						ClosePlugin ( HANDLE hInstance );
	bool						GetFindData ( HANDLE hPlugin, PanelItem_t * & dItems, int & nItems );
	void						FreeFindData ( HANDLE hPlugin, PanelItem_t * dItems, int nItems);
	bool						SetDirectory ( HANDLE hPlugin, const wchar_t * szDir );
	ItemCompare_t 				GetCompareFunc ( HANDLE hPlugin );
	const PluginInfo_t *		GetPluginInfo ( HANDLE hPlugin ) const;

	bool						DeleteFiles ( HANDLE hPlugin, PanelItem_t ** dItems, int nItems );
	bool						HandlesDelete ( HANDLE hPlugin ) const;
	bool						FindFiles ( HANDLE hPlugin, PanelItem_t ** dItems, int nItems );
	bool						HandlesFind ( HANDLE hPlugin ) const;

	bool						HandlesRealNames ( HANDLE hPlugin ) const;

	int							GetNumPlugins () const;
	Plugin_c *					GetPlugin ( int iPlugin ) const;

private:
	struct PluginInstance_t
	{
		Plugin_c *		m_pOwner;
		HANDLE			m_hInstance;
	};

	VarLoader_c	*				m_pLoader;
	Array_T	<Plugin_c *>		m_dPlugins;
	Array_T	<PluginInstance_t>	m_dInstances;

	Plugin_c *					CreatePlugin ( const Str_c & sPath );
	Plugin_c *					FindPlugin ( HANDLE hPlugin ) const;
};


extern PluginManager_c g_PluginManager;

#endif