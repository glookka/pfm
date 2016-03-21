#include "search.h"

#include "Registry.h"
#include "resource.h"

static wchar_t g_szSearchText [MAX_SEARCH];
extern HINSTANCE g_hInstance;
extern PluginStartupInfo_t g_PSI;

static const int NUM_BINARY_MODES = 3;
static wchar_t * g_szBinaryModes [NUM_BINARY_MODES] =
{
	 L"Unicode"
	,L"ASCII"
	,L"Hex"
};

enum RegSearchFlags_e
{
	 RS_CASE_SENSITIVE	=	1<<0
	,RS_NAMES_KEYS		=	1<<1
	,RS_NAMES_VALUES	=	1<<2
	,RS_DATA_SZ			=	1<<3
	,RS_DATA_MULTI_SZ	=	1<<4
	,RS_DATA_DWORD		=	1<<5
	,RS_DATA_BINARY		=	1<<6
};

enum RegSearchBinary_e
{
	 REG_BIN_UNICODE
	,REG_BIN_ASCII
	,REG_BIN_HEX
};


static BOOL CALLBACK FindRequestDlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{
	switch ( Msg )
	{
	case WM_INITDIALOG:
		{
			// fixme: localize, add to config

			HWND hList = GetDlgItem ( hDlg, IDC_SEARCH_TEXT );
			for ( int i = 0; i < MAX_RECENT_TEXTS && FindTextGet (i); i++ )
				SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM) FindTextGet (i) );

			SendMessage ( hList, CB_SETCURSEL, 0, 0 );

			CheckDlgButton ( hDlg, IDC_CASE_CHECK,			g_Cfg.find_case			? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_NAMES_KEY_CHECK,		g_Cfg.find_key_names	? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_NAMES_VALUE_CHECK,	g_Cfg.find_value_names	? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_DATA_SZ_CHECK,		g_Cfg.find_data_sz		? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_DATA_MULTISZ_CHECK,	g_Cfg.find_data_multisz	? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_DATA_DWORD_CHECK,	g_Cfg.find_data_dword	? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_DATA_BINARY_CHECK,	g_Cfg.find_data_binary	? BST_CHECKED : BST_UNCHECKED );

			hList = GetDlgItem ( hDlg, IDC_BINARY_MODE );
			for ( int i = 0; i < NUM_BINARY_MODES; i++ )
				SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM) g_szBinaryModes [i] );

			SendMessage ( hList, CB_SETCURSEL, 0, g_Cfg.find_bin_mode );

			CheckRadioButton ( hDlg, IDC_EVERYWHERE, IDC_SELKEYS, g_Cfg.find_area + IDC_EVERYWHERE );

			g_PSI.m_fnDlgInitFullscreen ( hDlg, false );
			g_PSI.m_fnCreateToolbar ( hDlg, TOOLBAR_OK_CANCEL, 0 );
		}
		break;

	case WM_COMMAND:
		{
			int iCommand = LOWORD(wParam);

			switch ( iCommand )
			{
			case IDOK:
				SipShowIM ( SIPF_OFF );

				GetDlgItemText ( hDlg, IDC_SEARCH_TEXT, g_szSearchText, MAX_SEARCH );
				FindTextAdd ( g_szSearchText );

				g_Cfg.find_case			= IsDlgButtonChecked ( hDlg, IDC_CASE_CHECK )			== BST_CHECKED ? 1 : 0;
				g_Cfg.find_key_names	= IsDlgButtonChecked ( hDlg, IDC_NAMES_KEY_CHECK )		== BST_CHECKED ? 1 : 0;
				g_Cfg.find_value_names	= IsDlgButtonChecked ( hDlg, IDC_NAMES_VALUE_CHECK )	== BST_CHECKED ? 1 : 0;
				g_Cfg.find_data_sz		= IsDlgButtonChecked ( hDlg, IDC_DATA_SZ_CHECK )		== BST_CHECKED ? 1 : 0;
				g_Cfg.find_data_multisz	= IsDlgButtonChecked ( hDlg, IDC_DATA_MULTISZ_CHECK )	== BST_CHECKED ? 1 : 0;
				g_Cfg.find_data_dword	= IsDlgButtonChecked ( hDlg, IDC_DATA_DWORD_CHECK )		== BST_CHECKED ? 1 : 0;
				g_Cfg.find_data_binary	= IsDlgButtonChecked ( hDlg, IDC_DATA_BINARY_CHECK )	== BST_CHECKED ? 1 : 0;

				g_Cfg.find_bin_mode = SendMessage ( GetDlgItem ( hDlg, IDC_BINARY_MODE ), CB_GETCURSEL, 0, 0 );		

				for ( int i = IDC_EVERYWHERE; i <= IDC_SELKEYS; i++ )
					if ( IsDlgButtonChecked ( hDlg, i ) == BST_CHECKED )
					{
						g_Cfg.find_area = i - IDC_EVERYWHERE;
						break;
					}

			case IDCANCEL:
				g_PSI.m_fnDlgCloseFullscreen ();
				EndDialog ( hDlg, iCommand );
				break;
			}
		}
		break;

	case WM_HELP:
		Help ( L"Main_Contents" );
		return TRUE;
	}

	g_PSI.m_fnDlgHandleActivate ( hDlg, Msg, wParam, lParam );

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
// registry search
class RegIterator_c
{
public:
	RegIterator_c ()
		: m_pRoot		( NULL )
		, m_uType		( REG_NONE )
		, m_uDataLen	( 0 )
		, m_bKey		( false )
		, m_uFlags		( 0 )
		, m_eBinMode	( REG_BIN_UNICODE )
	{
		m_szNameBuffer [0] = L'\0';
		m_szPathBuffer [0] = L'\0';
	}

	~RegIterator_c ()
	{
		while ( !Empty () )
			Pop ();
	}

	bool FindFirst ( const wchar_t * szRoot, DWORD uFlags, RegSearchBinary_e eBinMode )
	{
		m_uFlags = uFlags;
		m_eBinMode = eBinMode;

		HKEY hResult = NULL;
		HKEY hRootKey = GetRootKey ( szRoot );
		if ( !hRootKey )
		{
			if ( !wcscmp ( szRoot, L"\\" ) )
			{
				for ( int i = REG_ROOT_ITEMS - 1; i >= 0; i-- )
				{
					RegRoot_t & RegRoot = g_RegRoot [i];

					int iRootLen = wcslen ( RegRoot.m_szShortName );

					Node_t Node;
					Node.m_hKey	  = RegRoot.m_hKey;
					Node.m_szPath = new wchar_t [iRootLen + 1];
					wcscpy ( Node.m_szPath, RegRoot.m_szShortName );
					Push ( Node );
				}

				return true;
			}
			else
				return false;
		}

		const wchar_t * szSubkey = wcsstr ( szRoot, L"\\" );
		if ( !szSubkey )
			hResult = hRootKey;
		else
		{
			if ( RegOpenKeyEx ( hRootKey, szSubkey, 0, 0, &hResult ) != ERROR_SUCCESS )
				return false;

			if ( !hResult )
				return false;
		}

		Node_t Node;
		Node.m_hKey	  = hResult;
		int iRootLen = wcslen ( szRoot );
		Node.m_szPath = new wchar_t [iRootLen + 1];
		wcscpy ( Node.m_szPath, szRoot );
		Push ( Node );
		return true;
	}

	bool FindNext ()
	{	
		bool bNeedValues	= ( m_uFlags & ( RS_NAMES_VALUES | RS_DATA_SZ | RS_DATA_MULTI_SZ | RS_DATA_DWORD | RS_DATA_BINARY ) ) != 0;
		bool bNeedKeys		= ( m_uFlags & RS_NAMES_KEYS ) != 0;

		if ( Empty () || ( !bNeedValues && !bNeedKeys ) )
			return false;

		Node_t & Node = Root ();

		int iRootLen = wcslen ( Node.m_szPath );

		// reset data
		m_bKey = false;
		m_szNameBuffer [0] = L'\0';
		m_uType = REG_NONE;
		m_uDataLen = 0;

		DWORD uBufLen = MAX_PATH;
		if ( RegEnumKeyEx ( Node.m_hKey, Node.m_iSubKey, m_szNameBuffer, &uBufLen, NULL, NULL, NULL, NULL ) == ERROR_SUCCESS )
		{
			// push new one
			Node.m_iSubKey++;
			m_bKey	= true;

			Node_t NewNode;
			if ( RegOpenKeyEx ( Node.m_hKey, m_szNameBuffer, 0, 0, &NewNode.m_hKey ) == ERROR_SUCCESS )
			{
				NewNode.m_szPath = new wchar_t [iRootLen + wcslen (m_szNameBuffer) + 2];
				wcscpy ( NewNode.m_szPath, Node.m_szPath );
				wcscat ( NewNode.m_szPath, L"\\" );
				wcscat ( NewNode.m_szPath, m_szNameBuffer );
				Push ( NewNode );
			}

			if ( bNeedKeys )
			{
				wcsncpy ( m_szPathBuffer, Node.m_szPath, MAX_REG_PATH );
				return true;
			}
			else
				return FindNext ();
		}

		if ( bNeedValues )
		{
			uBufLen = MAX_PATH;
			m_uDataLen = MAX_DATA_LEN;
			LONG uRes = RegEnumValue ( Node.m_hKey, Node.m_iSubValue, m_szNameBuffer, &uBufLen, NULL, &m_uType, m_dData, &m_uDataLen );
			if ( uRes == ERROR_SUCCESS || uRes == ERROR_MORE_DATA )
			{
				if ( uRes == ERROR_MORE_DATA )
					m_uDataLen = 0;

				Node.m_iSubValue++;

				wcsncpy ( m_szPathBuffer, Node.m_szPath, MAX_REG_PATH );

				// zero-terminate data
				m_dData [m_uDataLen == MAX_DATA_LEN ? m_uDataLen - 1 : m_uDataLen] = 0;
				return true;
			}
		}

		Pop ();

		return FindNext ();
	}

	// data accessors
	bool			IsKey () const			{	return m_bKey;	}
	const wchar_t * GetName () const		{	return m_szNameBuffer; }
	const wchar_t * GetPath () const		{	return m_szPathBuffer; }
	DWORD			GetType () const		{	return m_uType;	}
	const BYTE *	GetData () const		{	return m_dData;	}
	DWORD			GetDataLen () const		{	return m_uDataLen; }

private:
	struct Node_t
	{
		HKEY		m_hKey;
		int			m_iSubKey;
		int			m_iSubValue;
		wchar_t *	m_szPath;
		Node_t *	m_pNext;

		Node_t ()
			: m_hKey		( NULL )
			, m_iSubKey		( 0 )
			, m_iSubValue	( 0 )
			, m_szPath		( NULL )
			, m_pNext		( NULL )
		{
		}
	};

	Node_t *	m_pRoot;

	wchar_t		m_szNameBuffer [MAX_REG_PATH];
	wchar_t		m_szPathBuffer [MAX_REG_PATH];
	DWORD		m_uType;
	DWORD		m_uDataLen;
	BYTE		m_dData [MAX_DATA_LEN];
	bool		m_bKey;

	DWORD		m_uFlags;
	RegSearchBinary_e m_eBinMode;

	void Push ( const Node_t & Node )
	{
		Node_t * pNewRoot = new Node_t;
		*pNewRoot = Node;
		pNewRoot->m_pNext = m_pRoot;
		m_pRoot = pNewRoot;
	}

	void Pop ()
	{
		if ( !Empty () )
		{
			Node_t * pOldRoot = m_pRoot;
			m_pRoot = m_pRoot->m_pNext;
			RegCloseKey ( pOldRoot->m_hKey );
			delete [] pOldRoot->m_szPath;
			delete pOldRoot;
		}
	}

	bool Empty () const
	{
		return m_pRoot == NULL;
	}

	Node_t & Root ()
	{
		return *m_pRoot;
	}
};


class RegSearch_c
{
public:
	RegSearch_c ( const wchar_t * szPath, PanelItem_t ** dItems, int nItems )
		: m_dItems	( dItems )
		, m_nItems	( nItems )
		, m_iSelKey	( 0 )
		, m_hFilter	( INVALID_HANDLE_VALUE )
		, m_dFound	( NULL )
		, m_nFound	( 0 )
	{
		switch ( g_Cfg.find_area + IDC_EVERYWHERE )
		{
		case IDC_EVERYWHERE:
			wcscpy ( m_szRoot, L"\\" );
			break;
		case IDC_CURKEY:
		case IDC_SELKEYS:
			ToShortPath ( m_szRoot, szPath );
			break;
		}

		m_hFilter = g_PSI.m_fnFilterCreate ( g_szSearchText, g_Cfg.find_case!=0 );

		// initial reserve
		m_iReserved	= 512;
		m_dFound = new Found_t [m_iReserved];
		m_uFlags = GetSearchFlags ();
		m_eBinMode = RegSearchBinary_e(g_Cfg.find_bin_mode);
	}

	~RegSearch_c ()
	{
		g_PSI.m_fnFilterDestroy ( m_hFilter );
		for ( int i = 0; i < m_nFound; i++ )
		{
			delete [] m_dFound [i].m_szName;
			delete [] m_dFound [i].m_szPath;
		}

		delete [] m_dFound;
	}

	int GetNumItems () const
	{
		return m_nFound;
	}

	const wchar_t * GetName ( int iItem ) const
	{
		return m_dFound [iItem].m_szName;
	}

	const wchar_t * GetPath ( int iItem ) const
	{
		return m_dFound [iItem].m_szPath;
	}

	bool IsKey ( int iItem ) const
	{
		return m_dFound [iItem].m_bKey;
	}

	DWORD GetType ( int iItem ) const
	{
		return m_dFound [iItem].m_uType;
	}

	bool FindFirst ()
	{
		if ( g_Cfg.find_area == IDC_SELKEYS )
			return StartAt ( 0 );
		else
			return m_RegIterator.FindFirst ( m_szRoot, m_uFlags, m_eBinMode );
	}

	bool FindNext ()
	{
		bool bFindResult = false;

		if ( g_Cfg.find_area + IDC_EVERYWHERE == IDC_SELKEYS )
		{
			if ( !m_RegIterator.FindNext () )
			{
				if ( StartAt ( m_iSelKey + 1 ) )
					bFindResult = m_RegIterator.FindNext ();
				else
					bFindResult = false;	
			}
		}
		else
			bFindResult = m_RegIterator.FindNext ();

		if ( !bFindResult )
			return false;

		if ( PassesFilter () )
		{
			if ( m_nFound >= m_iReserved )
			{
				int iNewLen = m_iReserved;
				if ( iNewLen < 1 )
					iNewLen = 1;
				while ( m_nFound + 1 > iNewLen )
					iNewLen *= 2;

				Found_t * pNewData = new Found_t [iNewLen];
				if ( m_dFound )
				{
					for ( int i = 0; i < m_nFound; i++ )
						pNewData [i] = m_dFound [i];

					delete [] m_dFound;
				}

				m_dFound = pNewData;
				m_iReserved = iNewLen;
			}

			Found_t & tNewFound = m_dFound [m_nFound];
			tNewFound.m_bKey	= m_RegIterator.IsKey ();
			tNewFound.m_uType	= m_RegIterator.GetType ();

			int iNameLen = wcslen ( m_RegIterator.GetName () );
			tNewFound.m_szName	= new wchar_t [iNameLen + 1];
			wcscpy ( tNewFound.m_szName, m_RegIterator.GetName () );

			int iPathLen = wcslen ( m_RegIterator.GetPath () );
			tNewFound.m_szPath	= new wchar_t [iPathLen + 1];
			wcscpy ( tNewFound.m_szPath, m_RegIterator.GetPath () );

			m_nFound++;
		}

		return true;
	}

private:
	PanelItem_t **	m_dItems;
	int				m_nItems;
	RegIterator_c	m_RegIterator;
	int				m_iSelKey;
	HANDLE			m_hFilter;

	struct Found_t
	{
		bool		m_bKey;
		wchar_t *	m_szName;
		wchar_t *	m_szPath;
		DWORD		m_uType;
	};

	Found_t *		m_dFound;
	int				m_iReserved;
	int				m_nFound;

	wchar_t			m_szRoot [MAX_REG_PATH];
	wchar_t			m_szPath [MAX_REG_PATH];

	DWORD			m_uFlags;
	RegSearchBinary_e m_eBinMode;

	DWORD GetSearchFlags () const
	{
		DWORD uFlags = 0;

		if ( g_Cfg.find_case )			uFlags |= RS_CASE_SENSITIVE;
		if ( g_Cfg.find_key_names )		uFlags |= RS_NAMES_KEYS;
		if ( g_Cfg.find_value_names )	uFlags |= RS_NAMES_VALUES;
		if ( g_Cfg.find_data_sz )		uFlags |= RS_DATA_SZ;
		if ( g_Cfg.find_data_multisz )	uFlags |= RS_DATA_MULTI_SZ;
		if ( g_Cfg.find_data_dword )	uFlags |= RS_DATA_DWORD;
		if ( g_Cfg.find_data_binary )	uFlags |= RS_DATA_BINARY;

		return uFlags;
	}

	bool StartAt ( int iItem )
	{
		if ( m_nItems == 0 || iItem >= m_nItems )
			return false;

		m_iSelKey = iItem;
		wcsncpy ( m_szPath, m_szRoot, MAX_PATH );
		wcscat ( m_szPath, L"\\"  );
		wcscat ( m_szPath, m_dItems [m_iSelKey]->m_FindData.cFileName );

		return m_RegIterator.FindFirst ( m_szPath, m_uFlags, m_eBinMode );
	}

	bool PassesFilter () const
	{
		bool bNeedCheckNames = ( m_uFlags & ( RS_NAMES_KEYS | RS_NAMES_VALUES ) ) != 0;
		bool bNeedCheckData = ( m_uFlags & ( RS_DATA_SZ | RS_DATA_MULTI_SZ | RS_DATA_DWORD | RS_DATA_BINARY ) ) != 0;

		bool bFoundInNames = false;
		bool bFoundInData = false;

		if ( bNeedCheckNames )
			bFoundInNames = g_PSI.m_fnFilterFits ( m_hFilter, m_RegIterator.GetName () );

		if ( bNeedCheckData )
			bFoundInData = CheckData ( m_RegIterator.GetType (), m_RegIterator.GetData (), m_RegIterator.GetDataLen () );

		return bFoundInNames || bFoundInData;
	}

	bool CheckData ( DWORD uType, const BYTE * pData, int iDataLen ) const
	{
		// FIXME: !special case when search string is empty!

		switch ( uType )
		{
		case REG_SZ:
			if ( !(m_uFlags & RS_DATA_SZ) )
				return false;
		case REG_MULTI_SZ:
			if ( !(m_uFlags & RS_DATA_MULTI_SZ) )
				return false;

			// FIXME: convert zeroes to something else for searching MULTI_SZ

			return wcsstr ( g_szSearchText, (const wchar_t*)pData ) != NULL;			
		
		case REG_BINARY:
			if ( !(m_uFlags & RS_DATA_BINARY) )
				return false;

			// FIXME: binary data handling

			break;

		case REG_DWORD:
			if ( !(m_uFlags & RS_DATA_DWORD) )
				return false;

			// FIXME: what to search for? part of number or whole number? see SKtools

			break;
		}

		return false;
	}
};


//////////////////////////////////////////////////////////////////////////
// find progress dialog
static const DWORD TIMER_FIND = 0xAA000001;

class FindProgressDlg_c
{
public:
	bool			m_bSearchFinished;
	RegSearch_c *	m_pRegSearch;


	FindProgressDlg_c ( RegPlugin_c * pPlugin, const wchar_t * szPath, PanelItem_t ** dItems, int nItems )
		: m_bSearchFinished	( false )
		, m_uTimer			( 0 )
		, m_hDlg			( NULL )
		, m_nLastItems		( 0 )
		, m_pPlugin			( pPlugin )
	{
		m_pRegSearch = new RegSearch_c ( szPath, dItems, nItems );
	}

	~FindProgressDlg_c ()
	{
		delete m_pRegSearch;
	}

	bool Init ( HWND hDlg )
	{
		m_hDlg = hDlg;

		m_bSearchFinished = false;

		DlgTxt ( m_hDlg, IDC_MESSAGE, DLG_SEARCHING );

		HWND hList = GetDlgItem ( m_hDlg, IDC_FILELIST );
		SendMessage ( hList, WM_SETFONT, (WPARAM)g_PSI.m_hSmallFont, TRUE );
		ListView_SetExtendedListViewStyle ( hList, LVS_EX_FULLROWSELECT );

		LVCOLUMN tColumn;
		ZeroMemory ( &tColumn, sizeof ( tColumn ) );
		tColumn.mask = LVCF_TEXT | LVCF_SUBITEM;
		tColumn.iSubItem = 0;
		tColumn.pszText = (wchar_t *)Txt ( DLG_COLUMN_NAME );

		ListView_InsertColumn ( hList, tColumn.iSubItem,  &tColumn );
		++tColumn.iSubItem;
		tColumn.pszText = (wchar_t *)Txt ( DLG_COLUMN_PATH );
		ListView_InsertColumn ( hList, tColumn.iSubItem,  &tColumn );

		ListView_SetImageList ( hList, RegPlugin_c::m_hImageList, LVSIL_SMALL );

		UpdateProgress ( true );

		g_PSI.m_fnDlgInitFullscreen ( m_hDlg, false );
		g_PSI.m_fnCreateToolbar ( hDlg, TOOLBAR_CANCEL, SHCMBF_HIDESIPBUTTON );

		m_uTimer = SetTimer ( m_hDlg, TIMER_FIND, 0, NULL );

		if ( g_PSI.m_fnDlgHandleSizeChange ( 0, 0, 0, 0, true ) )
			UpdateSearchColumns ();

		return m_pRegSearch->FindFirst ();
	}


	void UpdateProgress ( bool bForce = false )
	{
		wchar_t szBuffer [256];
		int nItems = m_pRegSearch->GetNumItems ();
		if ( nItems && ( m_nLastItems != nItems || bForce ) )
		{	
			HWND hList = GetDlgItem ( m_hDlg, IDC_FILELIST );

			LVITEM tItem;
			ZeroMemory ( &tItem, sizeof ( tItem ) );
			tItem.mask = LVIF_TEXT | LVIF_IMAGE;

			for ( int i = 0; i < nItems - m_nLastItems; ++i )
			{
				const wchar_t * szName = m_pRegSearch->GetName ( m_nLastItems + i );
				const wchar_t * szPath = m_pRegSearch->GetPath ( m_nLastItems + i );
				DWORD uType = m_pRegSearch->GetType ( m_nLastItems + i );

				tItem.pszText	= (wchar_t *) szName;
				tItem.iItem		= m_nLastItems + i;
				tItem.iImage	= GetIconByType ( uType, m_pRegSearch->IsKey ( m_nLastItems + i ) );

				int iItem = ListView_InsertItem ( hList, &tItem );
				ListView_SetItemText ( hList, iItem, 1, (wchar_t *)szPath );
			}

			m_nLastItems = nItems;

			if ( !m_bSearchFinished )
			{
				wsprintf ( szBuffer, Txt ( DLG_SEARCHING_N ), nItems );
				SetDlgItemText ( m_hDlg, IDC_MESSAGE, szBuffer );
			}
		}
	}

	void Close ()
	{
		ListView_SetImageList ( GetDlgItem ( m_hDlg, IDC_FILELIST ), NULL, LVSIL_SMALL );
		g_PSI.m_fnDlgCloseFullscreen ();
	}

	void SearchFinished ( bool bCancelled )
	{
		m_bSearchFinished = true;
		KillTimer ( m_hDlg, m_uTimer );

		HWND hTB = g_PSI.m_fnCreateToolbar ( m_hDlg, TOOLBAR_OK_CANCEL, SHCMBF_HIDESIPBUTTON );
		SetToolbarText ( hTB, IDOK, Txt ( TBAR_GOTO ) );

		wchar_t szBuffer [256];
		wsprintf ( szBuffer, Txt ( bCancelled ? DLG_SEARCH_CANCELLED : DLG_SEARCH_FINISHED ), m_pRegSearch->GetNumItems () );
		SetDlgItemText ( m_hDlg, IDC_MESSAGE, szBuffer );
	}

	bool MoveToSelected ()
	{
		int iSelected = ListView_GetSelectionMark ( GetDlgItem ( m_hDlg, IDC_FILELIST ) );
		if ( iSelected == -1 )
			return false;

		wchar_t szPath [MAX_REG_PATH];
		wcsncpy ( szPath, g_PSI.m_szRootFSShortcut, MAX_REG_PATH );
		wcsncat ( szPath, m_pRegSearch->GetPath ( iSelected ), MAX_REG_PATH - wcslen ( szPath ) );

		g_PSI.m_fnPanelSetDir ( m_pPlugin, szPath );
		g_PSI.m_fnFSPanelRefresh ( m_pPlugin );
		g_PSI.m_fnPanelSetCursor ( m_pPlugin, m_pRegSearch->GetName ( iSelected ) );

		return true;
	}

	void UpdateSearchColumns ()
	{
		HWND hList = GetDlgItem ( m_hDlg, IDC_FILELIST );
		ListView_SetColumnWidth ( hList, 0, GetColumnWidthRelative ( 0.4f ) );
		ListView_SetColumnWidth ( hList, 1, GetColumnWidthRelative ( 0.6f ) );
	}

private:
	DWORD			m_uTimer;
	HWND			m_hDlg;
	int				m_nLastItems;
	RegPlugin_c *	m_pPlugin;
};

FindProgressDlg_c * g_pFindProgressDlg = NULL;


static BOOL CALLBACK FindResultsDlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	switch ( Msg )
	{
	case WM_INITDIALOG:
		if ( !g_pFindProgressDlg->Init ( hDlg ) )
			g_pFindProgressDlg->SearchFinished ( false );
		break;

	case WM_TIMER:
		if ( wParam == TIMER_FIND && !g_pFindProgressDlg->m_bSearchFinished )
		{
			bool bFinished = false;
			const float SEARCH_CHUNK_TIME = 0.1f;
			float fStart = g_PSI.m_fnTimeSec ();

			while ( g_PSI.m_fnTimeSec () - fStart <= SEARCH_CHUNK_TIME )
				if ( !g_pFindProgressDlg->m_pRegSearch->FindNext () )
				{
					g_pFindProgressDlg->SearchFinished ( false );
					break;
				}

			g_pFindProgressDlg->UpdateProgress ();
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			if ( g_pFindProgressDlg->m_bSearchFinished )
			{
				SipShowIM ( SIPF_OFF );
				g_pFindProgressDlg->Close ();
				EndDialog ( hDlg, LOWORD (wParam) );
			}
			else
				g_pFindProgressDlg->SearchFinished ( true );
			break;

		case IDOK:
			if ( !g_pFindProgressDlg->MoveToSelected () )
				break;

			SipShowIM ( SIPF_OFF );
			g_pFindProgressDlg->Close ();
			EndDialog ( hDlg, LOWORD (wParam) );
			break;
		}
		break;

	case WM_HELP:
		Help ( L"FileSearch" );
		return TRUE;
	}

	if ( g_PSI.m_fnDlgHandleSizeChange ( hDlg, Msg, wParam, lParam, false ) )
	{
		g_PSI.m_fnDlgDoResize ( hDlg, GetDlgItem ( hDlg, IDC_FILELIST ), NULL, 0 );
		g_pFindProgressDlg->UpdateSearchColumns ();
	}

	g_PSI.m_fnDlgHandleActivate ( hDlg, Msg, wParam, lParam );

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
bool FindRequestDlg ( PanelItem_t ** dItems, int nItems )
{
	return DialogBox ( g_hInstance, MAKEINTRESOURCE ( IDD_FIND_REQUEST ), g_PSI.m_hMainWindow, FindRequestDlgProc ) == IDOK;
}


bool FindProgressDlg ( RegPlugin_c * pPlugin, const wchar_t * szPath, PanelItem_t ** dItems, int nItems )
{
	g_pFindProgressDlg = new FindProgressDlg_c ( pPlugin, szPath, dItems, nItems );
	int iRes = DialogBox ( g_hInstance, MAKEINTRESOURCE ( IDD_FIND_RESULTS ), g_PSI.m_hMainWindow, FindResultsDlgProc );
	delete g_pFindProgressDlg;
	g_pFindProgressDlg = NULL;

	return iRes == IDOK;
}