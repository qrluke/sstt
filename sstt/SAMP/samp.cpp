#pragma once
#include "samp.h"
#include <iostream>
#include <fstream>

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



