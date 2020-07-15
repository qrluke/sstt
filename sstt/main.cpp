#include <string>
#include <chrono>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <curl\curl.h>
#include <windows.h>
#include <d3d9.h>

#include "libs/DX9Hook/imgui.h"
#include "libs/DX9Hook/imgui_impl_dx9.h"
#include "libs/DX9Hook/vtable.h"
#include "libs/Yet-another-hook-library/hook.h"
#include "libs/coro_wait/coro_wait.h"
#include "libs/SAMP-API/sampapi/CInput.h"
#include "libs/audio/bass.h"
#include "libs/json.hpp"

typedef void(__cdecl* CMDPROC) (PCHAR);

using json = nlohmann::json;

using namespace sampapi::v037r1;

CInput*& pInput = RefInputBox();

std::string to_send;

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

bool time_to_send = false;

#include <windows.h>
json settings;
json stats;
static float stats_array[] = { 0, 0, 0, 0, 0, 0, 0 };


class SAMP* pSAMP;

#define version "15.07.2020"


#define RECORD_SAMPLERATE 48000
#define RECORD_CHANNELS 1
#define RECORD_BPS 16
#define RECORD_BUFSTEP (RECORD_SAMPLERATE * RECORD_CHANNELS * (RECORD_BPS / 8)) // = 1 sec

#define RECORD_MAX_SECONDS 15
#define RECORD_MAX_LENGTH (RECORD_BUFSTEP * RECORD_MAX_SECONDS)

#define _free(x)  \
	if (x)        \
	{             \
		free(x);  \
		x = NULL; \
	}


#define D3D_VFUNCTIONS 119
#define DEVICE_PTR 0xC97C28
#define ENDSCENE_INDEX 42
#define RESET_INDEX 16

typedef HRESULT(__stdcall* _EndScene)(IDirect3DDevice9* pDevice);
_EndScene oEndScene;

typedef long(__stdcall* _Reset)(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pp);
_Reset oReset = nullptr;

bool g_bwasInitialized = false;
bool menuOpen = false;
bool wndproc = false;
bool* p_open = NULL;
static int item = 0;
int startstop;
int close;
int hwndd;
int startmenu;


bool en_custom = false;
bool en_yandex = false;

char str0[1024];
char str1[1024];


DWORD key;
HMODULE samp = GetModuleHandleA("samp.dll");
DWORD address = (DWORD)samp + 0x64230;
DWORD procID;
DWORD g_dwSAMP_Addr = NULL;
DWORD* g_Chat = NULL;
HANDLE handle;
HWND hWnd;

WNDPROC oriWndProc = NULL;
extern LRESULT ImGui_ImplDX9_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


void imgui_update_str() {
	int i = 0;

	for (char& c : settings["url"].get<std::string>()) {
		str0[i] = c;
		i++;
	}

	for (char& c : settings["header"].get<std::string>()) {
		str1[i] = c;
		i++;
	}
}


void sampSendChat(const std::string& text)
{
	typedef int(__stdcall* SendCommand)(const char*);
	typedef int(__stdcall* SendText)(const char*);
	static SendCommand sendCommand = (SendCommand)((DWORD)GetModuleHandle("samp.dll") + 0x65C60);
	static SendText sendText = (SendText)((DWORD)GetModuleHandle("samp.dll") + 0x57F0);

	char* cstr = new char[text.length() + 1];
	std::strcpy(cstr, text.c_str());

	if (text[0] == '/')
		sendCommand(cstr);
	else
		sendText(cstr);
	delete[] cstr;
}

void sampAddMessage(int color, const std::string& string)
{
	typedef int(__thiscall* AddMessage)(DWORD, int, const char*, int, int, int);
	static AddMessage addMessage = (AddMessage)((DWORD)GetModuleHandle("samp.dll") + 0x64010);
	static DWORD chatPointer = (DWORD)GetModuleHandle("samp.dll") + 0x21A0E4;

	addMessage(*(DWORD*)chatPointer, 4, string.c_str(), 0, color, 0);
}

void registerChatCommand(char* szCmd, CMDPROC pFunc)
{
	if (pInput == nullptr)
		return;

	void(__thiscall * AddClientCommand) (const void* _this, char* szCommand, CMDPROC pFunc) =
		(void(__thiscall*) (const void*, char*, CMDPROC)) ((DWORD)GetModuleHandle("samp.dll") + 0x065AD0);

	if (szCmd == NULL)
		return;

	return AddClientCommand(pInput, szCmd, pFunc);
}

void test() {
	menuOpen = !menuOpen;
}

void read_json(const std::string& name) {
	if (FILE* file = fopen(name.c_str(), "r")) {
		fclose(file);
	}
	else {
		auto j3 = json::parse(u8"{\"backend\":1,\"header\":\"Content-Type: audio/x-wav\", \"url\":\"http://asr.yandex.net/asr_xml?uuid=12345678123456781234546112345678&disableAntimat=true&topic=general&lang=ru-RU&key=6372dda5-9674-4413-85ff-e9d0eb2f99a7\",\"presets\":{\"prosto v chat\":{\"key\":\"R\",\"text\":\"\"},\"/s\":{\"key\":\"P\",\"text\":\"/s \"},\"/r\":{\"key\":\"N\",\"text\":\"/r \"},\"/me\":{\"key\":\"J\",\"text\":\"/me \"},\"/b\":{\"key\":\"B\",\"text\":\"/b \"},\"/m\":{\"key\":\"L\",\"text\":\"/m \"}}}");

		std::ofstream MyFile(name.c_str());
		MyFile << j3.dump(4) << std::endl;
		MyFile.close();
	}

	std::ifstream t(name.c_str());
	t >> settings;


	switch (settings["backend"].get<int>())
	{
	case 0:
		en_custom = true;
		break;
	case 1:
		en_yandex = true;
	default:
		break;
	}

	imgui_update_str();
	t.close();
}

void save_json(const std::string& name) {
	std::ofstream t(name.c_str());
	t << settings.dump(4) << std::endl;
	t.close();
}

void everything()
{
	GetWindowThreadProcessId(hWnd, &procID);
	handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);
}

void MouseFix()
{
	float xaxis;
	float yaxis;
	everything();
	ReadProcessMemory(handle, (PBYTE*)0xB6EC1C, &xaxis, 4, 0);
	ReadProcessMemory(handle, (PBYTE*)0xB6EC18, &yaxis, 4, 0);
	if (xaxis != yaxis)
	{
		WriteProcessMemory(handle, (LPVOID)0xB6EC18, &xaxis, 4, 0);
	}
}

void toggleSAMPCursor(int iToggle)
{
	void* obj = *(void**)((DWORD)samp + 0x21A10C);
	((void(__thiscall*) (void*, int, bool)) ((DWORD)samp + 0x9BD30))(obj, iToggle ? 3 : 0, !iToggle);
	if (!iToggle)
		((void(__thiscall*) (void*)) ((DWORD)samp + 0x9BC10))(obj);
}

void toggleChat(int toggle)
{
	int togchattrue = 0xC3;
	int togchatfalse = 2347862870;
	everything();
	if (toggle == 1)
	{
		WriteProcessMemory(handle, (LPVOID)((DWORD)samp + 0x64230), &togchattrue, sizeof(togchattrue), 0);
	}
	else
	{
		WriteProcessMemory(handle, (LPVOID)((DWORD)samp + 0x64230), &togchatfalse, sizeof(togchatfalse), 0);
	}
}

void Shutdown()
{
	void** vTableDevice = *(void***)(*(DWORD*)DEVICE_PTR);
	VTableHookManager* vmtHooks = new VTableHookManager(vTableDevice, D3D_VFUNCTIONS);
	vmtHooks->Unhook(ENDSCENE_INDEX);
	menuOpen = false;
	toggleSAMPCursor(0);
	toggleChat(0);
	close = 1;
}

void set_imgui_style()
{
	ImGuiStyle& style = ImGui::GetStyle();

	style.Colors[ImGuiCol_Text] = ImVec4(1.00, 1.00, 1.00, 1.00);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50, 0.50, 0.50, 1.00);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06, 0.06, 0.06, 0.94);
	style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(1.00, 1.00, 1.00, 0.00);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08, 0.08, 0.08, 0.94);
	style.Colors[ImGuiCol_ComboBg] = style.Colors[ImGuiCol_PopupBg];
	style.Colors[ImGuiCol_Border] = ImVec4(0.43, 0.43, 0.50, 0.50);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00, 0.00, 0.00, 0.00);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.16, 0.29, 0.48, 0.54);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26, 0.59, 0.98, 0.40);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26, 0.59, 0.98, 0.67);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.04, 0.04, 0.04, 1.00);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.16, 0.29, 0.48, 1.00);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00, 0.00, 0.00, 0.51);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14, 0.14, 0.14, 1.00);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02, 0.02, 0.02, 0.53);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31, 0.31, 0.31, 1.00);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41, 0.41, 0.41, 1.00);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51, 0.51, 0.51, 1.00);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26, 0.59, 0.98, 1.00);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24, 0.52, 0.88, 1.00);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26, 0.59, 0.98, 1.00);
	style.Colors[ImGuiCol_Button] = ImVec4(0.26, 0.59, 0.98, 0.40);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0, 0, 0, 1.00);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06, 0.53, 0.98, 1.00);
	style.Colors[ImGuiCol_Header] = ImVec4(0.26, 0.59, 0.98, 0.31);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26, 0.59, 0.98, 0.80);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26, 0.59, 0.98, 1.00);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.26, 0.59, 0.98, 0.25);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26, 0.59, 0.98, 0.67);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26, 0.59, 0.98, 0.95);
	style.Colors[ImGuiCol_CloseButton] = ImVec4(0.41, 0.41, 0.41, 0.50);
	style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.98, 0.39, 0.36, 1.00);
	style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.98, 0.39, 0.36, 1.00);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61, 0.61, 0.61, 1.00);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00, 0.43, 0.35, 1.00);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90, 0.70, 0.00, 1.00);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00, 0.60, 0.00, 1.00);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26, 0.59, 0.98, 0.35);
	style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.80, 0.80, 0.80, 0.35);
}


void RenderGUI()
{
	static float f = 0.0f;
	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowPos(ImVec2(300, 310), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(500.f, 600.f), ImGuiSetCond_FirstUseEver);
	if (!ImGui::Begin("SA:MP Speech To Text", p_open = NULL, window_flags))
	{
		ImGui::End();
		return;
	}


	ImGui::TextColored(ImVec4(1, 1, 0, 1), "Select backend API (ne trogat)");

	if (ImGui::RadioButton("yandex demo 2016", en_yandex)) {
		en_yandex = not en_yandex;
		if (en_yandex) {
			en_custom = false;
			settings["url"] = "http://asr.yandex.net/asr_xml?uuid=12345678123456781234546112345678&disableAntimat=true&topic=general&lang=ru-RU&key=6372dda5-9674-4413-85ff-e9d0eb2f99a7";
			settings["header"] = "Content-Type: audio/x-wav";
			settings["backend"] = 1;
			save_json("sstt.json");
			read_json("sstt.json");
		}
	}

	if (ImGui::RadioButton("custom", en_custom)) {
		en_custom = not en_custom;
		if (en_custom) {
			en_yandex = false;
			settings["backend"] = 0;
			settings["url"] = "http://asr.yandex.net/asr_xml?uuid=12345678123456781234546112345678&disableAntimat=true&topic=general&lang=ru-RU&key=6372dda5-9674-4413-85ff-e9d0eb2f99a7";
			settings["header"] = "Content-Type: audio/x-wav";

			save_json("sstt.json");
			read_json("sstt.json");
		}
	}

	if (en_custom) {
		ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize("API url").x);

		if (ImGui::InputText("API url", str0, IM_ARRAYSIZE(str0), ImGuiInputTextFlags_ReadOnly)) {
			settings["url"] = str0;
			save_json("sstt.json");
		}
		ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() - ImGui::CalcTextSize("request header").x);

		if (ImGui::InputText("request header", str1, IM_ARRAYSIZE(str1), ImGuiInputTextFlags_ReadOnly)) {
			settings["header"] = str1;
			save_json("sstt.json");
		}
	}
	static float stats_max = *std::max_element(stats_array, stats_array + 6);
	ImGui::TextColored(ImVec4(1, 1, 0, 1), "Unique users per day (last week)");
	ImGui::PlotHistogram("", stats_array, IM_ARRAYSIZE(stats_array), 0, "column = day", 0.0f, stats_max + 5, ImVec2(ImGui::GetWindowContentRegionWidth(), 150));

	ImGui::TextColored(ImVec4(1, 1, 0, 1), "Presets");
	ImGui::Text("Edit sstt.json in your game folder and restart game.");
	ImGui::BeginChild("Scrolling");

	ImGui::Columns(3, NULL, true);
	ImGui::Text("name");
	ImGui::NextColumn();
	ImGui::Text("button");
	ImGui::NextColumn();
	ImGui::Text("prefix");
	ImGui::Separator();
	ImGui::NextColumn();


	for (auto& el : settings["presets"].items())
	{
		ImGui::Text(el.key().c_str());
		ImGui::NextColumn();
		ImGui::Text(el.value()["key"].get<std::string>().c_str());
		ImGui::NextColumn();
		ImGui::Text(el.value()["text"].get<std::string>().c_str());
		ImGui::Separator();
		ImGui::NextColumn();
	}

	ImGui::Columns(1);



	ImGui::EndChild();


	ImGui::End();
}



LRESULT CALLBACK hWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplDX9_WndProcHandler(hwnd, uMsg, wParam, lParam) && GetKeyState(key) == 1 && menuOpen && wndproc)
	{
		return 1l;
	}

	return CallWindowProc(oriWndProc, hwnd, uMsg, wParam, lParam);
}

HRESULT __stdcall hkReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pp)
{
	if (g_bwasInitialized)
	{
		ImGui_ImplDX9_InvalidateDeviceObjects();
		g_bwasInitialized = false;
	}
	return oReset(pDevice, pp);
}

HRESULT __stdcall hkEndScene(IDirect3DDevice9* pDevice)
{
	if (!g_bwasInitialized)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		io.IniFilename = NULL;
		io.DeltaTime = 1.0f / 60.0f;
		ImFont* pFont = io.Fonts->AddFontDefault();
		D3DDEVICE_CREATION_PARAMETERS d3dcp;
		pDevice->GetCreationParameters(&d3dcp);
		hWnd = d3dcp.hFocusWindow;
		io.Fonts->AddFontDefault();
		style.AntiAliasedLines = false;
		if (hwndd == 0)
		{
			oriWndProc = (WNDPROC)SetWindowLongPtr(d3dcp.hFocusWindow,
				GWL_WNDPROC, (LONG)(LONG_PTR)hWndProc);
			hwndd++;
		}
		ImGui_ImplDX9_Init(d3dcp.hFocusWindow, pDevice);
		ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Tahoma.TTF", 14.0F, NULL, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());

		g_bwasInitialized = true;
	}

	ImGui_ImplDX9_NewFrame();
	if (menuOpen)
	{
		toggleSAMPCursor(1);
		//toggleChat(1);
		RenderGUI();
	}
	else
	{
		if (startstop == 0)
		{
			toggleSAMPCursor(0);
			toggleChat(0);
			startstop++;
		}
	}
	ImGui::Render();
	return oEndScene(pDevice);
}





typedef class CBuffer
{
private:
	byte* buffer = nullptr;
	byte* readPtr = nullptr;
	byte* writePtr = nullptr;
	size_t buffer_size = 0;

public:
	CBuffer() {};
	CBuffer(size_t size)
	{
		SetSize(size);
	};
	~CBuffer()
	{
		_free(buffer);
	};
	bool SetSize(size_t size, bool reset = true)
	{
		if (reset)
			_free(buffer);

		byte* second = nullptr;
		if ((buffer && (second = (byte*)realloc(buffer, size))) || (buffer = (byte*)malloc(size)))
		{
			buffer_size = size;
			if (reset)
			{
				readPtr = buffer;
				writePtr = buffer;
			}
			else
			{
				readPtr = second + (readPtr - buffer);
				writePtr = second + (writePtr - buffer);
				buffer = second;
			}
			return true;
		}
		buffer_size = 0;
		readPtr = writePtr = buffer = 0;
		return false;
	}
	size_t Read(void* dst, size_t size)
	{
		if (!buffer || !dst)
			return 0;
		size = min(size, buffer_size - (readPtr - buffer));
		if (size > 0)
		{
			memcpy(dst, readPtr, size);
			readPtr += size;
		}
		return size;
	};
	size_t Write(const void* src, size_t size, size_t pos = 0)
	{
		if (!buffer || !src)
			return 0;
		if (pos && pos < buffer_size)
			writePtr = buffer + pos;
		size = min(size, buffer_size - (writePtr - buffer));
		if (size > 0)
		{
			memcpy(writePtr, src, size);
			writePtr += size;
		}
		return size;
	};
	byte* data()
	{
		return buffer;
	};
	size_t size()
	{
		return writePtr - buffer;
	};
	size_t bufsize()
	{
		return buffer_size;
	};
} curlfile_t;

///////////////////////////////////////////////////////////////////////////////

HRECORD rchan;
curlfile_t record;


size_t read_request_data(void* ptr, size_t size, size_t nmemb, curlfile_t* userp)
{
	if (userp == nullptr)
		return 0;

	return userp->Read(ptr, size * nmemb);
}

size_t write_responce_data(char* contents, size_t size, size_t nmemb, std::string* userp)
{
	if (userp == nullptr)
		return 0;

	userp->append(contents, size * nmemb);
	return size * nmemb;
}

static size_t WriteCallback(char* contents, size_t size, size_t nmemb, std::string* userp)
{
	userp->append(contents, size * nmemb);
	return size * nmemb;
}


void checkUpd(std::string url)
{
	CURL* curl;
	CURLcode res;
	std::string readBuffer;

	curl = curl_easy_init();
	if (curl) {
		std::string tmpver = version;
		size_t pos = tmpver.find("\n");
		tmpver = tmpver.substr(0, pos);
		url = url + "?v=" + tmpver;
		DWORD VSNumber;
		if (GetVolumeInformation(NULL, NULL, 0, &VSNumber, NULL, NULL, NULL, 0))
		{
			std::ostringstream id;
			int i = 5;
			id << VSNumber;
			std::string strid = id.str();
			url = url + "&id=" + strid;
		}
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		uint32_t httpCode;
		curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &httpCode);


		if (httpCode == 200)
		{
			stats = json::parse(readBuffer);

			int stats_iter = 0;

			for (auto& el : stats["users"].items())
			{
				stats_array[stats_iter] = ::atof(el.value().get<std::string>().c_str());
				stats_iter++;
			}

			if (stats["version"].get<std::string>().compare(version) != 0)
			{
				sampAddMessage(-1, "{FF4500}~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{FFFFFF}SAMP SPEECH-TO-TEXT{FF4500}~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
				sampAddMessage(-1, "{FF4500}SSTT: Обнаружено обновление!{FFD900} Прямая ссылка на скачивание:{FFFFFF}  https://qrlk.me/sstt");
				sampAddMessage(-1, "{FFD900}Скачайте архив по ссылке, разархивируйте его содержимое в папку игры с заменой.");
				sampAddMessage(-1, "{FF4500}SSTT: Обнаружено обновление!{FFD900} Подробная информация:{FFFFFF} https://github.com/qrlk/sstt/releases");
				sampAddMessage(-1, "{FF4500}~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
			}
			else
				sampAddMessage(-1, "SSTT: {348cb2}Вы используете самую свежую версию! Приятной игры! {ffffff}GH: https://github.com/qrlk/sstt");
		}
		else
			sampAddMessage(-1, "{FF4500}SSTT: Невозможно проверить наличие обновления.{FFFFFF} Проверьте вручную: https://github.com/qrlk/sstt/releases");
	}
	return;
}

std::string recognition(curlfile_t* file, const char* url)
{
	CURL* curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_POST, TRUE);
		curl_easy_setopt(curl, CURLOPT_HEADER, TRUE);

		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

		curl_slist* headers = curl_slist_append(NULL, settings["header"].get<std::string>().c_str());

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		curl_easy_setopt(curl, CURLOPT_URL, url);

		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_request_data);
		curl_easy_setopt(curl, CURLOPT_READDATA, file);
		curl_easy_setopt(curl, CURLOPT_INFILESIZE, file->size());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, file->size());

		std::string result;
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_responce_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

		CURLcode code = curl_easy_perform(curl);

		uint32_t httpCode;
		curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &httpCode);

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);

		std::string str3;

		if (httpCode == 200)
		{
			if (!strstr(result.c_str(), "<recognitionResults success=\"0\" />"))
			{
				str3 = result.substr(result.find("<variant confidence=\"0\">"));
				str3 = str3.substr(24);
				str3 = str3.substr(0, str3.find("<"));
				return str3;
			}
		}
		else
		{
			sampAddMessage(-1, "[SSTT]: Непредвиденная ошибка сетевого характера :(");
		}
	}
	return std::string();
}

BOOL CALLBACK RecordingCallback(HRECORD handle, const void* buffer, DWORD length, void* user)
{
	if (record.size() + length > record.bufsize())
	{
		if (record.bufsize() + RECORD_BUFSTEP > RECORD_MAX_LENGTH)
		{
			sampAddMessage(-1, "[SSTT]: Вы достигли максимального размера файла. Запись прекращена.");
			return false;
		}

		if (!record.SetSize(record.bufsize() + RECORD_BUFSTEP, false))
			return false;
	}
	record.Write(buffer, length);
	return true;
}

void StartRecording()
{
	if (record.SetSize(RECORD_BUFSTEP * 3, true))
	{
		rchan = BASS_RecordStart(RECORD_SAMPLERATE, RECORD_CHANNELS, NULL, RecordingCallback, NULL);
		if (!rchan)
		{
			sampAddMessage(-1, "[SSTT] Can't start recording. BASS_ErrorGetCode() = " + std::to_string(BASS_ErrorGetCode()));
			return;
		}
		record.Write("RIFF\x00\x00\x00\x00\WAVEfmt \x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\data\x00\x00\x00\x00", 44, 0);
	}
}

void StopRecording()
{
	BASS_ChannelStop(rchan);

	WAVEFORMATEX* wf = (WAVEFORMATEX*)(record.data() + 20);

	wf->wFormatTag = WAVE_FORMAT_PCM;
	wf->nChannels = RECORD_CHANNELS;
	wf->nSamplesPerSec = RECORD_SAMPLERATE;
	wf->nAvgBytesPerSec = wf->nSamplesPerSec * wf->nBlockAlign;
	wf->wBitsPerSample = RECORD_BPS;
	wf->nBlockAlign = wf->nChannels * wf->wBitsPerSample / 8;

	*(DWORD*)(record.data() + 4) = record.size() - 8;
	*(DWORD*)(record.data() + 40) = record.size() - 44;

	/*
	FILE *pFile = fopen("SSTT.wav", "wb");
	if (pFile)
	{
		fwrite(record.data(), 1, record.size(), pFile);
		fclose(pFile);
	}
	*/
}

std::string lite_conv(std::string src, UINT cp_from, UINT cp_to)
{
	if (src.empty())
		return std::string();

	int wstr_len = MultiByteToWideChar(cp_from, NULL, src.c_str(), src.length(), NULL, 0);
	std::wstring wide_str(wstr_len, L'\x00');
	MultiByteToWideChar(cp_from, NULL, src.c_str(), src.length(), &wide_str[0], wstr_len);

	int outstr_len = WideCharToMultiByte(cp_to, NULL, &wide_str[0], wide_str.length(), NULL, 0, NULL, NULL);
	std::string final_str(outstr_len, '\x00');
	WideCharToMultiByte(cp_to, NULL, &wide_str[0], wide_str.length(), &final_str[0], outstr_len, NULL, NULL);

	return final_str;
}

void CheckKey(const char key)
{
	if (!(GetKeyState(key) & 0x8000))
		return;

	if (pInput->m_bEnabled)
		return;

	Sleep(250);

	if (!(GetKeyState(key) & 0x8000))
		return;

	StartRecording();

	while ((GetKeyState(key) & 0x8000) && BASS_ChannelIsActive(rchan))
		Sleep(100);

	StopRecording();

	std::string text = recognition(&record, settings["url"].get<std::string>().c_str());

	if (text.empty())
	{
		sampAddMessage(-1, "[SSTT]: Не удалось распознать/нет доступа к API!");
		return;
	}

	for (auto& el : settings["presets"].items())
	{
		if (el.value()["key"].get<std::string>().c_str()[0] == key) {
			text.insert(0, el.value()["text"].get<std::string>());
			break;
		}
	}


	to_send = lite_conv(text, CP_UTF8, CP_ACP);
	if (to_send.length() > 128)
		to_send.resize(128);

	time_to_send = true;
}


DWORD WINAPI MainThread(LPVOID p)
{
	DWORD dwSAMPAddr = NULL;

	while (!(dwSAMPAddr = (DWORD)GetModuleHandleA("samp.dll")))
		Sleep(100);

	if (!BASS_RecordInit(-1) && BASS_ErrorGetCode() != BASS_ERROR_ALREADY)
	{
		sampAddMessage(-1, "[SSTT] Ошибка инициализации. Микрофон не найден.");
		ExitThread(0);
	}

	CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
	if (code != CURLE_OK)
	{
		sampAddMessage(0xFFAA4040, "CURL ERROR");
		ExitThread(0);
	}

	std::string ver = version;

	checkUpd("http://qrlk.me/dev/moonloader/sstt/stats_1.php");


	void** vTableDevice = *(void***)(*(DWORD*)DEVICE_PTR);
	VTableHookManager* vmtHooks = new VTableHookManager(vTableDevice, D3D_VFUNCTIONS);

	oEndScene = (_EndScene)vmtHooks->Hook(ENDSCENE_INDEX, (void*)hkEndScene);
	oReset = (_Reset)vmtHooks->Hook(RESET_INDEX, (void*)hkReset);

	everything();
	sampAddMessage(-1, "SSTT v" + ver + " инициализирован. Автор: {348cb2}qrlk{ffffff}. Спасибо: {cc0000}redcode{ffffff}, {ffa500}BlackKnigga{ffffff}, {17bb17}imring");




	read_json("sstt.json");
	std::stringstream ss;
	ss << "Меню: /sstt || Держите клавишу, потом отпустите: ";
	for (auto& el : settings["presets"].items())
	{
		ss << el.value()["key"].get<std::string>() << " - " << el.key() << " | ";
	}
	sampAddMessage(-1, ss.str());



	registerChatCommand("sstt", CMDPROC(test));
	set_imgui_style();


	while (true)
	{
		for (auto& el : settings["presets"].items())
		{
			CheckKey(el.value()["key"].get<std::string>().c_str()[0]);
		}

		Sleep(10);

		if (close == 1)
		{
			return 0;
		}
	}
	ExitThread(0);
}


void foo()
{
	using namespace std::chrono_literals;
	while (!pInput) {
		this_coro::wait(100ms);
	}
	this_coro::wait(500ms);

	CreateThread(NULL, 0, MainThread, NULL, NULL, NULL);

	unsigned int counter = 0;
	while (true)
	{
		this_coro::wait(50ms);
		if (time_to_send)
		{
			static int counter = 0;
			counter++;
			sampSendChat(to_send);
			/*this_coro::wait(50ms);
			sampAddMessage(-1, "message sent. Times: " + std::to_string(counter));*/
			time_to_send = false;
		}
	}
}

void game_loop_Idle()
{
	static coro_wait instance{ foo };

	instance.process();
}

class sstt
{
public:
	sstt()
	{
		using Idle_t = void(__cdecl*)();
		Idle_t Idle = reinterpret_cast<Idle_t>(0x53BEE0);

		static hook game_loop_Idle_hook(Idle, game_loop_Idle);
	}
} sstt;