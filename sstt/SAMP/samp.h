#pragma once

#include "d3d9.h"
#include "d3dx9.h"
#include "iostream"
#include "Windows.h"

#define SAMP_INFO_OFFSET 0x21A0F8
#define SAMP_CHAT_INFO_OFFSET 0x21A0E4
#define SAMP_CHAT_INPUT_INFO_OFFSET 0x21A0E8
#define SAMP_FUNC_ADDTOCHATWND 0x064010
#define SAMP_SENDSAY 0x57F0
#define SAMP_SENDCMD 0x65C60

enum
{
	SAMP_MAX_ACTORS = 1000,
	SAMP_MAX_PLAYERS = 1004,
	SAMP_MAX_VEHICLES = 2000,
	SAMP_MAX_PICKUPS = 4096,
	SAMP_MAX_OBJECTS = 1000,
	SAMP_MAX_GANGZONES = 1024,
	SAMP_MAX_3DTEXTS = 2048,
	SAMP_MAX_TEXTDRAWS = 2048,
	SAMP_MAX_PLAYERTEXTDRAWS = 256,
	SAMP_MAX_CLIENTCMDS = 144,
	SAMP_MAX_MENUS = 128,
	SAMP_MAX_PLAYER_NAME = 24,
	SAMP_ALLOWED_PLAYER_NAME_LENGTH = 20,
	SAMP_TOGGLECURSOR = 0x09BD30,
	SAMP_CURSORUNLOCKACTORCAM = 0x09BC10,
};

#pragma pack(push, 1)

struct stSAMPPools
{
	struct stActorPool *pActor;
	struct stObjectPool *pObject;
	struct stGangzonePool *pGangzone;
	struct stTextLabelPool *pText3D;
	struct stTextdrawPool *pTextdraw;
	void *pPlayerLabels;
	struct stPlayerPool *pPlayer;
	struct stVehiclePool *pVehicle;
	struct stPickupPool *pPickup;
};

struct stSAMP
{
	void *pUnk0[2];
	uint8_t byteSpace[24];
	char szIP[257];
	char szHostname[259];
	uint8_t byteUnk1;
	uint32_t ulPort;
	uint32_t ulMapIcons[100];
	int iLanMode;
	int iGameState;
	uint32_t ulConnectTick;
	struct stServerPresets *pSettings;
	void *pRakClientInterface;
	struct stSAMPPools *pPools;
};

struct stPlayerPool
{
	uint32_t ulUnk0;
	uint16_t sLocalPlayerID;
	void *pVTBL_txtHandler;
	union {
		char szLocalPlayerName[16];
		char *pszLocalPlayerName;
	};
	int iLocalPlayerNameLen;
	int iLocalPlayerNameAllocated;
	struct stLocalPlayer *pLocalPlayer;
	int iLocalPlayerPing;
	int iLocalPlayerScore;
	struct stRemotePlayer *pRemotePlayer[SAMP_MAX_PLAYERS];
	int iIsListed[SAMP_MAX_PLAYERS];
	uint32_t ulUnk1[SAMP_MAX_PLAYERS];
};

struct stInputBox
{
	void *pUnknown;
	uint8_t bIsChatboxOpen;
	uint8_t bIsMouseInChatbox;
	uint8_t bMouseClick_related;
	uint8_t unk;
	DWORD dwPosChatInput[2];
	uint8_t unk2[263];
	int iCursorPosition;
	uint8_t unk3;
	int iMarkedText_startPos;
	uint8_t unk4[20];
	int iMouseLeftButton;
};

typedef void(__cdecl *CMDPROC)(PCHAR);
struct stInputInfo
{
	void *pD3DDevice;
	void *pDXUTDialog;
	stInputBox *pDXUTEditBox;
	CMDPROC pCMDs[SAMP_MAX_CLIENTCMDS];
	char szCMDNames[SAMP_MAX_CLIENTCMDS][33];
	int iCMDCount;
	int iInputEnabled;
	char szInputBuffer[129];
	char szRecallBufffer[10][129];
	char szCurrentBuffer[129];
	int iCurrentRecall;
	int iTotalRecalls;
	CMDPROC pszDefaultCMD;
};

#pragma pack(pop)

class SAMP
{
public:
	bool isInited;

	SAMP(HMODULE hModule)
	{
		isInited = false;
		if (hModule)
			dwSAMPAddr = (DWORD)hModule;
	};

	~SAMP()
	{
		delete g_SAMP;
		delete g_Chat;
		delete g_Input;
	};

	bool IsInitialized();

	void AddChatMessage(D3DCOLOR, char *);
	void SendChat(char *);

	struct stSAMP *getInfo() { return g_SAMP; };
	struct stPlayerPool *getPlayers() { return g_Players; };
	struct stVehiclePool *getVehicles() { return g_Vehicles; };
	struct stChatInfo *getChat() { return g_Chat; };
	struct stInputInfo *getInput() { return g_Input; };

private:
	DWORD dwSAMPAddr;
	struct stSAMP *g_SAMP;
	struct stPlayerPool *g_Players;
	struct stVehiclePool *g_Vehicles;
	struct stChatInfo *g_Chat;
	struct stInputInfo *g_Input;
};