#include "samp.h"

bool SAMP::IsInitialized()
{
	g_SAMP = *(stSAMP**)(dwSAMPAddr + SAMP_INFO_OFFSET);
	if (g_SAMP == nullptr)
		return false;
	g_Chat = *(stChatInfo**)(dwSAMPAddr + SAMP_CHAT_INFO_OFFSET);
	if (g_Chat == nullptr)
		return false;
	g_Input = *(stInputInfo**)(dwSAMPAddr + SAMP_CHAT_INPUT_INFO_OFFSET);
	if (g_Input == nullptr)
		return false;

	//g_Vehicles = g_SAMP->pPools->pVehicle;
	g_Players = g_SAMP->pPools->pPlayer;

	isInited = true;

	return true;
};

// Функция sampAddChatMessage с форматированием текста.
void SAMP::AddChatMessage(D3DCOLOR color, const char* format, ...)
{
	if (format == NULL)
		return;

	va_list ap;
	char text[256];

	va_start(ap, format);
	vsprintf_s(text, 256, format, ap);
	va_end(ap);

	((void(__thiscall*)(stChatInfo*, ChatMessageType, const char*, const char*, D3DCOLOR, D3DCOLOR))(dwSAMPAddr + SAMP_FUNC_ADDTOCHATWND))(g_Chat, CHAT_TYPE_DEBUG, text, NULL, color, NULL);
}

// Фунция sampSendChat без форматирования текста.
void SAMP::SendChat(const char* text)
{
	if (text == nullptr)
		return;

	if (text[0] == '/')
	{
		((void(__thiscall*) (void* _this, const char* message)) (dwSAMPAddr + SAMP_SENDCMD))(g_Input, text);
	}
	else
	{
		((void(__thiscall*) (void* _this, const char* message)) (dwSAMPAddr + SAMP_SENDSAY)) (g_Players->pLocalPlayer, text);
	}

	free((char*)text);
}


// I have not a single idea what it actually is. But it works while I don't understand yet how to make similar functianality. 
// Source: https://github.com/SAMPProjects/Open-SAMP-API/blob/d409a384bda26b996f2023d5199904186788708c/src/Open-SAMP-API/Client/SAMPFunctions.cpp#L176
bool dataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask)
{
	for (; *szMask; ++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask)
			return false;
	return (*szMask) == NULL;
}

DWORD findPattern(DWORD addr, DWORD len, const BYTE* bMask, const char* szMask)
{
	for (DWORD i = 0; i < len; i++)
		if (dataCompare((BYTE*)(addr + i), bMask, szMask))
			return (DWORD)(addr + i);
	return 0;
}

bool SAMP::isInput()
{
	static auto addr = findPattern(
		dwSAMPAddr,
		dwSAMPAddr,
		(const BYTE*)"\x8B\x0D\x00\x00\x00\x00\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00\x68\x00\x00\x00\x00\x68\x00\x00\x00\x00\xE8",
		"xx????x????x????xx????x????x????x"
	);

	if (addr == 0)
		return false;

	stInputInfo* pInputInfo = *(stInputInfo**)*(DWORD*)(addr + 0x2);

	if (pInputInfo == NULL)
		return false;
	if (pInputInfo->pDXUTEditBox == NULL)
		return false;

	return pInputInfo->pDXUTEditBox->bIsChatboxOpen != 0;
}