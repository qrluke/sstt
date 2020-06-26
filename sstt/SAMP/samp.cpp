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
void SAMP::AddChatMessage(D3DCOLOR color, const char *format, ...)
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
}
