#ifndef _cbtdefs_
#define _cbtdefs_

#define BLUETOOTH_MAX_NAME_SIZE 248

typedef ULONGLONG BTH_ADDR;

struct BLUETOOTH_ADDRESS
{
	union
	{
		BTH_ADDR	ullLong;
		BYTE		rgBytes[6];  
	};
};

struct BLUETOOTH_DEVICE_INFO
{
	DWORD				dwSize;
	BLUETOOTH_ADDRESS	Address;
	ULONG				ulClassofDevice;
	BOOL				fConnected;
	BOOL				fRemembered;
	BOOL				fAuthenticated;
	SYSTEMTIME			stLastSeen;
	SYSTEMTIME			stLastUsed;
	WCHAR				szName[BLUETOOTH_MAX_NAME_SIZE];
};

#endif