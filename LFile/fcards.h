#ifndef _fcards_
#define _fcards_

#include "LCore/cmain.h"

void 	RefreshStorageCards ();
int  	GetNumStorageCards ();
void	GetStorageCardData ( int iCard, WIN32_FIND_DATA & tData );

#endif
