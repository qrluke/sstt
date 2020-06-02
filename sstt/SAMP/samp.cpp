#pragma once

#include "samp.h"

bool SAMP::IsInitialized()
{
	g_SAMP = *(stSAMP **)(dwSAMPAddr + SAMP_INFO_OFFSET);
	if (g_SAMP == nullptr)
		return false;
	g_Chat = *(stChatInfo **)(dwSAMPAddr + SAMP_CHAT_INFO_OFFSET);
	if (g_Chat == nullptr)
		return false;
	g_Input = *(stInputInfo **)(dwSAMPAddr + SAMP_CHAT_INPUT_INFO_OFFSET);
	if (g_Input == nullptr)
		return false;

	g_Vehicles = g_SAMP->pPools->pVehicle;
	g_Players = g_SAMP->pPools->pPlayer;

	isInited = true;

	return true;
};

void SAMP::AddChatMessage(D3DCOLOR cColor, char *szMsg)
{
	Sleep(50);
	if (g_Chat == nullptr)
		return;

	void(__thiscall * AddToChatWindowBuffer)(const void *_this, int iType, char *szText, char *szPrefix, DWORD cColor, DWORD cPrefixColor) =
		(void(__thiscall *)(const void *, int, char *, char *, DWORD, DWORD))(dwSAMPAddr + SAMP_FUNC_ADDTOCHATWND);

	if (szMsg == NULL)
		return;

	va_list ap;
	char tmp[512];
	memset(tmp, 0, 512);
	va_start(ap, szMsg);
	vsnprintf(tmp, sizeof(tmp) - 1, szMsg, ap);
	va_end(ap);

	return AddToChatWindowBuffer((void *)g_Chat, 8, tmp, NULL, cColor, 0x00);
}
// https://github.com/BlastHackNet/mod_s0beit_sa-1/blob/dc9b3b13599a8b6325e566f567b5391b0b2a6dc8/src/samp.cpp#L734
void SAMP::SendChat(char *szMsg)
{
	Sleep(50);

	if (g_Chat == nullptr)
		return;

	if (szMsg == NULL)
		return;

	va_list ap;
	char tmp[512];
	memset(tmp, 0, 512);
	va_start(ap, szMsg);
	vsnprintf(tmp, sizeof(tmp) - 1, szMsg, ap);
	va_end(ap);

	if (tmp[0] == '/')
	{
		((void(__thiscall*) (void* _this, char* message)) (dwSAMPAddr + SAMP_SENDCMD))(g_Input, tmp);
	}
	else
	{
		((void(__thiscall*) (void* _this, char* message)) (dwSAMPAddr + SAMP_SENDSAY)) (g_Players->pLocalPlayer, tmp);
	}
}
