#include <string>
#include <chrono>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <curl\curl.h>

#include "libs/Yet-another-hook-library/hook.h"
#include "libs/coro_wait/coro_wait.h"
#include "libs/SAMP-API/sampapi/CInput.h"

#include "libs/audio/bass.h"

using namespace sampapi::v037r1;

CInput*& pInput = RefInputBox();

std::string to_send;
bool time_to_send = false;

#include <windows.h>

class SAMP* pSAMP;

#define version "02.07.2020\n\n"

#define __URL "http://asr.yandex.net/asr_xml?uuid=12345678123456781234546112345678&disableAntimat=true&topic=general&lang=ru-RU&key=6372dda5-9674-4413-85ff-e9d0eb2f99a7"

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

std::string recognition(curlfile_t *file)
{
	CURL * curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_POST, TRUE);
		curl_easy_setopt(curl, CURLOPT_HEADER, TRUE);

		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

		curl_slist* headers = curl_slist_append(NULL, "Content-Type: audio/x-wav");

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		curl_easy_setopt(curl, CURLOPT_URL, __URL);

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

	std::string text = recognition(&record);

	if (text.empty())
	{
		sampAddMessage(-1, "[SSTT]: Не удалось распознать/нет доступа к API!");
		return;
	}

	switch (key)
	{
	case 'N':
		text.insert(0, "/r ");
		break;
	case 'P':
		text.insert(0, "/s ");
		break;
	case 'B':
		text.insert(0, "/b ");
		break;
	case 'L':
		text.insert(0, "/m ");
		break;
	case 'J':
		text.insert(0, "/me ");
		break;
	default:
		break;
	}

	to_send = lite_conv(text, CP_UTF8, CP_ACP);
	if (to_send.length() > 128)
		to_send.resize(128);

	time_to_send = true;

	/*
	static int counter = 0;
	counter++;
	sampAddMessage(-1, "[SSTT]: Done! Times: %d", counter);
	*/
}

void checkUpd(std::string url)
{
	CURL *curl;
	CURLcode res;
	std::string readBuffer;

	curl = curl_easy_init();
	if (curl)
	{
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
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_responce_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, readBuffer);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		uint32_t httpCode;
		curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &httpCode);

		if (httpCode == 200)
		{
			if (readBuffer.compare(version) != 0)
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

	sampAddMessage(-1, "SSTT v42.60.0202 инициализирован. Держите клавишу, потом отпустите. Автор: {348cb2}em.klrq");
	sampAddMessage(-1, "Клавиши: R - говорить, P - крикнуть, N - рация, M - /me, L - мегафон, B - /b");
	//checkUpd("http://qrlk.me/dev/moonloader/sstt/stats.php");

	while (true)
	{
		CheckKey('R');
		CheckKey('P');
		CheckKey('N');
		CheckKey('B');
		CheckKey('L');
		CheckKey('J');

		Sleep(10);
	}
	ExitThread(0);
}

void foo()
{
	using namespace std::chrono_literals;
	this_coro::wait(5s);

	CreateThread(NULL, 0, MainThread, NULL, NULL, NULL);

	unsigned int counter = 0;
	while (true)
	{
		this_coro::wait(50ms);
		if (time_to_send)
		{
			static int counter = 0;
			counter++;
			sampSendChat("Result: " + to_send);
			this_coro::wait(50ms);
			sampAddMessage(-1, "message sent. Times: " + std::to_string(counter));
			//time_to_send = false;
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
		using Idle_t = void(__cdecl *)();
		Idle_t Idle = reinterpret_cast<Idle_t>(0x53BEE0);

		static hook game_loop_Idle_hook(Idle, game_loop_Idle);
	}
} sstt;