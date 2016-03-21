#ifndef _cdefines_
#define _cdefines_

// only defines allowed here!

#include "conf/defines_ext.h"

#ifndef FM_RELEASE_BUILD
	#define FM_RELEASE_BUILD 0
#endif

#if FM_RELEASE_BUILD
	#ifndef FM_NAG_SCREEN
		#define FM_NAG_SCREEN	1
	#endif

	#define FM_MEMORY_TRACK		0
	#define FM_USE_ASSERT		0
	#define FM_USE_VERIFY		0
	#define	FM_USE_LOG			0
	#define	FM_USE_WARNING		0
	#define	FM_USE_ACHTUNG		0
#else
	#ifndef FM_NAG_SCREEN
		#define FM_NAG_SCREEN	0
	#endif

	#define FM_MEMORY_TRACK		1
	#define FM_USE_ASSERT		1
	#define FM_USE_VERIFY		1
	#define	FM_USE_LOG			1
	#define	FM_USE_WARNING		1
	#define	FM_USE_ACHTUNG		1
#endif

#define FM_VERSION				L"1.4 alpha 1"

#ifndef FM_BUILD_HANDANGO
	#define FM_BUILD_HANDANGO		1
#endif
#ifndef FM_BUILD_POCKETGEAR
	#define FM_BUILD_POCKETGEAR		0
#endif
#ifndef FM_BUILD_CLICKAPPS
	#define FM_BUILD_CLICKAPPS		0
#endif
#ifndef FM_BUILD_POCKETLAND
	#define FM_BUILD_POCKETLAND		0
#endif
#ifndef FM_BUILD_POCKETSELECT
	#define FM_BUILD_POCKETSELECT 	0
#endif
#ifndef FM_BUILD_PDATOPSOFT
	#define	FM_BUILD_PDATOPSOFT		0
#endif

#endif