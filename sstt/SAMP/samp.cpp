#pragma once

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

	g_Vehicles = g_SAMP->pPools->pVehicle;
	g_Players = g_SAMP->pPools->pPlayer;

	isInited = true;

	return true;
};

// Source: https://github.com/BlastHackNet/mod_s0beit_sa-1/blob/dc9b3b13599a8b6325e566f567b5391b0b2a6dc8/src/samp.cpp#L734

void SAMP::AddChatMessage(D3DCOLOR textColor, char* text)
{
	if (g_SAMP == NULL || g_Chat == NULL)
		return;

	if (text == NULL)
		return;

	void(__thiscall * AddToChatWindowBuffer) (void*, ChatMessageType, const char*, const char*, D3DCOLOR, D3DCOLOR) =
		(void(__thiscall*) (void* _this, ChatMessageType Type, const char* szString, const char* szPrefix, D3DCOLOR TextColor, D3DCOLOR PrefixColor))
		(dwSAMPAddr + SAMP_FUNC_ADDTOCHATWND);


	AddToChatWindowBuffer(g_Chat, CHAT_TYPE_DEBUG, text, nullptr, textColor, 0);


}

bool isBadPtr_handlerAny(void* pointer, ULONG size, DWORD dwFlags)
{
	DWORD						dwSize;
	MEMORY_BASIC_INFORMATION	meminfo;

	if (NULL == pointer)
		return true;

	memset(&meminfo, 0x00, sizeof(meminfo));
	dwSize = VirtualQuery(pointer, &meminfo, sizeof(meminfo));

	if (0 == dwSize)
		return true;

	if (MEM_COMMIT != meminfo.State)
		return true;

	if (0 == (meminfo.Protect & dwFlags))
		return true;

	if (size > meminfo.RegionSize)
		return true;

	if ((unsigned)((char*)pointer - (char*)meminfo.BaseAddress) > (unsigned)(meminfo.RegionSize - size))
		return true;

	return false;
}

bool isBadPtr_readAny(void* pointer, ULONG size)
{
	return isBadPtr_handlerAny(pointer, size, PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ |
		PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
}

void SAMP::SendChat(char* text)
{
	if (g_SAMP == NULL)
		return;

	if (text == NULL)
		return;
	if (isBadPtr_readAny(text, 128))
		return;

	va_list ap;
	char	tmp[128];
	memset(tmp, 0, 128);

	va_start(ap, text);
	vsprintf(tmp, text, ap);
	va_end(ap);

	if (g_SAMP == NULL)
		return;

	if (tmp == NULL)
		return;
	if (isBadPtr_readAny(tmp, 128))
		return;

	if (tmp == NULL)
		return;

	if (tmp[0] == '/')
	{
		((void(__thiscall*) (void* _this, char* message)) (dwSAMPAddr + SAMP_SENDCMD))(g_Input, tmp);
	}
	else
	{
		((void(__thiscall*) (void* _this, char* message)) (dwSAMPAddr + SAMP_SENDSAY)) (g_Players->pLocalPlayer, tmp);
	}
}

