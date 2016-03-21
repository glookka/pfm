#ifndef _fapps_
#define _fapps_

#include "pfm/main.h"
#include "pfm/std.h"

namespace apps
{
	struct AppInfo_t
	{
		Str_c		m_sName;
		Str_c		m_sFileName;
		bool		m_bBuiltIn;
		int			m_iIcon;
		bool		m_bRecommended;

		void 		LoadIcon ();
	};

	bool			EnumApps ();
	bool			EnumAppsFor ( const Str_c & sFileName );

	int				GetNumApps ();
	const AppInfo_t & GetApp ( int iApp );
	int				GetAppFor ( const Str_c & sFileName );
	int				FindAppByFilename ( const Str_c & sFileName );
	int				AddApp ( const AppInfo_t & tApp );

	void			Associate ( const Str_c & sFileName, int iApp, bool bQuotes );
}

namespace newmenu
{
	struct NewItem_t
	{
		Str_c		m_sName;
		GUID		m_GUID;

		void		Run () const;
	};

	bool			EnumNewItems ();
	int				GetNumItems ();
	const NewItem_t & GetItem ( int iItem );
}

#endif
