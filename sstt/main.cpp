#include "main.h"
#include "audio/bass.h"
#include <string>
#include <sstream>
#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <Psapi.h>

#pragma comment( lib, "psapi.lib" )


#define version "08.06.2020\n\n"

#define E_ADDR_GAMEPROCESS	0x53E981

#define FREQ 48000
#define CHANS 1
#define BUFSTEP 200000 // memory allocation unit

int counter = 0;
std::string string_to_send;
std::string text;
char str11[64];
char cursedCharArray[512];


int input;			 // current input source
BYTE* recbuf = NULL; // recording buffer
DWORD reclen;		 // recording length
HRECORD rchan = 0;	 // recording channel
HSTREAM chan = 0;	 // playback channel

size_t write_response_data(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	std::stringstream* s = (std::stringstream*)userdata;
	size_t n = size * nmemb;
	s->write(ptr, n);
	return n;
}

size_t read_request_data(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	std::ifstream* f = (std::ifstream*)userdata;
	size_t n = size * nmemb;
	f->read(ptr, n);
	size_t result = static_cast<size_t>(f->gcount());
	return result;
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
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

		unsigned httpCode;
		curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &httpCode);

		if (httpCode == 200)
		{
			if (readBuffer.compare(version) != 0)
			{
				pSAMP->AddChatMessage(-1, "{FF4500}~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{FFFFFF}SAMP SPEECH-TO-TEXT{FF4500}~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
				pSAMP->AddChatMessage(-1, "{FF4500}SSTT: Обнаружено обновление!{FFD900} Прямая ссылка на скачивание:{FFFFFF}  https://qrlk.me/sstt");
				pSAMP->AddChatMessage(-1, "{FFD900}Скачайте архив по ссылке, разархивируйте его содержимое в папку игры с заменой.");
				pSAMP->AddChatMessage(-1, "{FF4500}SSTT: Обнаружено обновление!{FFD900} Подробная информация:{FFFFFF} https://github.com/qrlk/sstt/releases");
				pSAMP->AddChatMessage(-1, "{FF4500}~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
			}
			else
				pSAMP->AddChatMessage(-1, "SSTT: {348cb2}Вы используете самую свежую версию! Приятной игры! {ffffff}GH: https://github.com/qrlk/sstt");
		}
		else
			pSAMP->AddChatMessage(-1, "{FF4500}SSTT: Невозможно проверить наличие обновления.{FFFFFF} Проверьте вручную: https://github.com/qrlk/sstt/releases");
	}
	return;
}

std::string recognition(std::string filename)
{
	CURL* curl = NULL;
	curl = curl_easy_init();

	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_HEADER, 1);
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

		struct curl_slist* headers = NULL;

		headers = curl_slist_append(headers, "Content-Type: audio/x-wav");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		std::stringstream url;
		url << "http://asr.yandex.net/asr_xml?uuid=12345678123456781234567812345678&disableAntimat=true&topic=general&lang=ru-RU&key=6372dda5-9674-4413-85ff-e9d0eb2f99a7";

		curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());

		std::ifstream fileStream(filename, std::ifstream::binary);
		fileStream.seekg(0, fileStream.end);
		int length = static_cast<size_t>(fileStream.tellg());
		fileStream.seekg(0, fileStream.beg);

		curl_easy_setopt(curl, CURLOPT_READFUNCTION, &read_request_data);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, length);
		curl_easy_setopt(curl, CURLOPT_READDATA, &fileStream);

		std::stringstream contentStream;

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_response_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &contentStream);

		CURLcode code = curl_easy_perform(curl);

		unsigned httpCode;
		curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &httpCode);
		std::ofstream myfile;
		myfile.open("SSTT-last-response.log");
		myfile << "Http code is " << httpCode << "\n";
		myfile << contentStream.str() << "\n";
		myfile.close();
		if (httpCode == 200)
		{
			std::string str = contentStream.str();
			std::string s2 = "<recognitionResults success=\"0\" />";
			if (strstr(str.c_str(), s2.c_str()))
			{
				curl_free(headers);
				curl_easy_cleanup(curl);
				return "NOT RECOGNIZED";
			}

			size_t pos = str.find("<variant confidence=\"0\">");
			std::string str3 = str.substr(pos);

			str3 = str3.substr(24);
			str3 = str3.substr(0, str3.find("<"));

			curl_free(headers);
			curl_easy_cleanup(curl);
			return str3;
		}
		else
		{
			curl_free(headers);
			curl_easy_cleanup(curl);
			return "ERROR";
		};
	}
	return 0;
}

// display error messages straight into chat
void Error(const char* es)
{
	char mes[200];
	sprintf(mes, "%s\n(error code: %d)", es, BASS_ErrorGetCode());
	pSAMP->AddChatMessage(-1, mes);
}

BOOL CALLBACK RecordingCallback(HRECORD handle, const void* buffer, DWORD length, void* user)
{
	// increase buffer size if needed
	if ((reclen % BUFSTEP) + length >= BUFSTEP)
	{
		recbuf = (BYTE*)realloc(recbuf, ((reclen + length) / BUFSTEP + 1) * BUFSTEP);
		if (!recbuf)
		{
			rchan = 0;
			Error("Out of memory!");
			return FALSE; // stop recording
		}
	}
	// buffer the data
	memcpy(recbuf + reclen, buffer, length);
	reclen += length;
	return TRUE; // continue recording
}

void StartRecording()
{
	WAVEFORMATEX* wf;
	if (recbuf)
	{ // free old recording
		BASS_StreamFree(chan);
		chan = 0;
		free(recbuf);
		recbuf = NULL;
	}
	// allocate initial buffer and make space for WAVE header
	recbuf = (BYTE*)malloc(BUFSTEP);
	reclen = 44;
	// fill the WAVE header
	memcpy(recbuf, "RIFF\0\0\0\0WAVEfmt \20\0\0\0", 20);
	memcpy(recbuf + 36, "data\0\0\0\0", 8);
	wf = (WAVEFORMATEX*)(recbuf + 20);
	wf->wFormatTag = 1;
	wf->nChannels = CHANS;
	wf->wBitsPerSample = 16;
	wf->nSamplesPerSec = FREQ;
	wf->nBlockAlign = wf->nChannels * wf->wBitsPerSample / 8;
	wf->nAvgBytesPerSec = wf->nSamplesPerSec * wf->nBlockAlign;
	// start recording
	rchan = BASS_RecordStart(FREQ, CHANS, 0, RecordingCallback, 0);
	if (!rchan)
	{
		Error("Can't start recording");
		free(recbuf);
		recbuf = 0;
		return;
	}
}

void StopRecording()
{
	BASS_ChannelStop(rchan);
	rchan = 0;
	// complete the WAVE header
	*(DWORD*)(recbuf + 4) = reclen - 8;
	*(DWORD*)(recbuf + 40) = reclen - 44;
	// create a stream from the recording
	chan = BASS_StreamCreateFile(TRUE, recbuf, 0, reclen, 0);
}

// write the recorded data to disk
void WriteToDisk()
{
	FILE* fp;
	char file[MAX_PATH] = "";
	if (!(fp = fopen("SSTT.wav", "wb")))
	{
		Error("Can't create the file");
		return;
	}

	fwrite(recbuf, reclen, 1, fp);
	fclose(fp);
}

BOOL InitDevice(int device)
{
	BASS_RecordFree(); // free current device (and recording channel) if there is one
	// initalize new device
	if (!BASS_RecordInit(device))
	{
		Error("Can't initialize recording device");
		return FALSE;
	}
	{ // get list of inputs
		int c;
		const char* i;
		input = 0;
		for (c = 0; i = BASS_RecordGetInputName(c); c++)
		{
			if (!(BASS_RecordGetInput(c, NULL) & BASS_INPUT_OFF))
			{ // this one is currently "on"
				input = c;
			}
		}
	}
	return TRUE;
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

void CheckKey(const std::string& key)
{
	Sleep(10);
	if (GetKeyState(key[0]) & 0x8000)
	{
		if (pSAMP->isInput() == 1) {
			return;
		}
		Sleep(200);
		if (!(GetKeyState(key[0]) & 0x8000))
		{
			return;
		}
		//pSAMP->AddChatMessage(-1, "[SSTT]: Started Recording");
		StartRecording();
		while (GetKeyState(key[0]) & 0x8000)
		{
			Sleep(100);
			if (reclen > 1024000)
			{
				pSAMP->AddChatMessage(-1, "[SSTT]: Вы достигли максимального размера файла. Запись прекращена.");
				break;
			}
		}
		//pSAMP->AddChatMessage(-1, "[SSTT]: Recording Finished");
		StopRecording();
		//pSAMP->AddChatMessage(-1, "[SSTT]: Saving...");
		WriteToDisk();
		//pSAMP->AddChatMessage(-1, "[SSTT]: Saved!");
		//pSAMP->AddChatMessage(-1, "[SSTT]: Recognizion...");
		text = recognition("SSTT.wav");

		if (!text.empty()) {
			if (text == "ERROR")
			{
				pSAMP->AddChatMessage(-1, "[SSTT]: Непредвиденная ошибка сетевого характера :(");
				return;
			}

			if (text == "NOT RECOGNIZED")
			{
				pSAMP->AddChatMessage(-1, "[SSTT]: Не удалось распознать/нет доступа к API!");
				return;
			}
			if (!key.compare("N"))
				text = "/r " + text;
			if (!key.compare("P"))
				text = "/s " + text;
			if (!key.compare("B"))
				text = "/b " + text;
			if (!key.compare("L"))
				text = "/m " + text;
			if (!key.compare("M"))
				text = "/me " + text;

			string_to_send = lite_conv(text, CP_UTF8, CP_ACP);
			if (string_to_send.length() > 128)
				string_to_send.resize(128);

			pSAMP->SendChat(string_to_send.c_str());

			counter++;
			//sprintf(str11, "[SSTT]: Done! Times: %d", counter);
			//pSAMP->AddChatMessage(-1, str11);
		}
	}
}

void MainThread()
{
	bool initialized = false;

	while (true)
	{
		while (!pSAMP->IsInitialized())
			Sleep(100);
		if (!initialized)
		{
			if (!pSAMP->IsInitialized()) continue;

			bool initialized = false;
			while (true)
			{
				Sleep(100);

				if (!initialized)
				{
					if (!pSAMP->IsInitialized())
						continue;
					Sleep(250);
					{
						int c, def;
						BASS_DEVICEINFO di;
						for (c = 0; BASS_RecordGetDeviceInfo(c, &di); c++)
						{
							if (di.flags & BASS_DEVICE_DEFAULT)
							{
								def = c;
							}
						}
						InitDevice(def);
					}

					initialized = true;
					pSAMP->AddChatMessage(-1, "SSTT v03.06.2020 инициализирован. Держите клавишу, потом отпустите. Автор: {348cb2}qrlk.me");
					pSAMP->AddChatMessage(-1, "Клавиши: R - говорить, P - крикнуть, N - рация, M - /me, L - мегафон, B - /b");
					checkUpd("http://qrlk.me/dev/moonloader/sstt/stats.php");
				}
				CheckKey("R");
				CheckKey("P");
				CheckKey("N");
				CheckKey("B");
				CheckKey("L");
				CheckKey("M");
			}
		}
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReasonForCall, LPVOID lpReserved)
{
	switch (dwReasonForCall)
	{
	case DLL_PROCESS_ATTACH:
		Sleep(1000);
		pSAMP = new SAMP(GetModuleHandleA("SAMP.DLL"));
		beginThread(MainThread);
		break;
	}
	return TRUE;
}