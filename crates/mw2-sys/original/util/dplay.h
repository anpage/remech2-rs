#ifndef DPLAY_H
#define DPLAY_H

// Stand-in for the DirectPlay 1 header: the types, constants and interface the network code
// (network.c, netio.c) is written against, declared here as they are in dplay.h. There is no
// DirectPlay behind them. Creating the interface fails, so a network game can't start and none
// of the calls through it are reached: the network code is kept as the reference for
// reimplementing it.
#include <windows.h>

#define DP_OK ((HRESULT) 0)
#define DPERR_UNSUPPORTED ((HRESULT) 0x80004001)
#define DPERR_NOMESSAGES ((HRESULT) 0x887700be)
#define DPERR_BUSY ((HRESULT) 0x8877010e)

#define DPSESSIONNAMELEN 32
#define DPPASSWORDLEN 16
#define DPUSERRESERVED 16

#define DPOPEN_OPENSESSION 0x00000001
#define DPOPEN_CREATESESSION 0x00000002

#define DPSEND_GUARANTEE 0x00000001
#define DPSEND_TRYONCE 0x00000004

#define DPRECEIVE_ALL 0x00000001

#define DPENUMPLAYERS_ALL 0x00000000
#define DPENUMPLAYERS_GROUP 0x00000020

#define DPENUMSESSIONS_AVAILABLE 0x00000001

#define DPESC_TIMEDOUT 0x00000001

typedef DWORD DPID, *LPDPID;

typedef struct tagDPSESSIONDESC {
	DWORD dwSize;
	GUID guidSession;
	DWORD dwSession;
	DWORD dwMaxPlayers;
	DWORD dwCurrentPlayers;
	DWORD dwFlags;
	char szSessionName[DPSESSIONNAMELEN];
	char szUserField[DPUSERRESERVED];
	DWORD dwReserved1;
	char szPassword[DPPASSWORDLEN];
	DWORD dwReserved2;
	DWORD dwUser1;
	DWORD dwUser2;
	DWORD dwUser3;
	DWORD dwUser4;
} DPSESSIONDESC, *LPDPSESSIONDESC;

typedef BOOL(PASCAL* LPDPENUMDPCALLBACK)(
	LPGUID p_guid,
	LPSTR p_name,
	DWORD p_majorVersion,
	DWORD p_minorVersion,
	LPVOID p_context
);
typedef BOOL(PASCAL* LPDPENUMSESSIONSCALLBACK)(
	LPDPSESSIONDESC p_desc,
	LPVOID p_context,
	LPDWORD p_timeout,
	DWORD p_flags
);
typedef BOOL(PASCAL* LPDPENUMPLAYERSCALLBACK)(
	DPID p_id,
	LPSTR p_friendlyName,
	LPSTR p_formalName,
	DWORD p_flags,
	LPVOID p_context
);

typedef struct IDirectPlay* LPDIRECTPLAY;

// The methods of IDirectPlay the game calls. This isn't the COM interface's layout: nothing
// implements it.
typedef struct IDirectPlayVtbl {
	HRESULT (*Close)(LPDIRECTPLAY p_this);
	HRESULT (*CreatePlayer)(
		LPDIRECTPLAY p_this,
		LPDPID p_id,
		LPSTR p_friendlyName,
		LPSTR p_formalName,
		LPHANDLE p_event
	);
	HRESULT (*DestroyPlayer)(LPDIRECTPLAY p_this, DPID p_id);
	HRESULT (*EnumPlayers)(
		LPDIRECTPLAY p_this,
		DWORD p_session,
		LPDPENUMPLAYERSCALLBACK p_callback,
		LPVOID p_context,
		DWORD p_flags
	);
	HRESULT (*EnumSessions)(
		LPDIRECTPLAY p_this,
		LPDPSESSIONDESC p_desc,
		DWORD p_timeout,
		LPDPENUMSESSIONSCALLBACK p_callback,
		LPVOID p_context,
		DWORD p_flags
	);
	HRESULT (*Open)(LPDIRECTPLAY p_this, LPDPSESSIONDESC p_desc);
	HRESULT (*Receive)(LPDIRECTPLAY p_this, LPDPID p_from, LPDPID p_to, DWORD p_flags, LPVOID p_data, LPDWORD p_size);
	HRESULT (*Send)(LPDIRECTPLAY p_this, DPID p_from, DPID p_to, DWORD p_flags, LPVOID p_data, DWORD p_size);
} IDirectPlayVtbl;

typedef struct IDirectPlay {
	IDirectPlayVtbl* lpVtbl;
} IDirectPlay;

// No service providers, and no interface.
#define DirectPlayEnumerate(p_callback, p_context) DPERR_UNSUPPORTED
#define DirectPlayCreate(p_guid, p_directPlay, p_outer) DPERR_UNSUPPORTED

#endif // DPLAY_H
