#include "main.h"
#include <fstream>
#include <string>
#include <sstream>
#include <curl/curl.h>
#include "bass.h"
#include "plugin.h"
#include "CTimer.h"
#include "CHud.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iconvlite.h>

using namespace std;


#define FREQ 48000
#define CHANS 1
#define BUFSTEP 200000	// memory allocation unit

int input;				// current input source
BYTE* recbuf = NULL;	// recording buffer
DWORD reclen;			// recording length
HRECORD rchan = 0;		// recording channel
HSTREAM chan = 0;		// playback channel


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
	size_t result = f->gcount();
	return result;
}

int recognition(std::string filename)
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
		url << "http://asr.yandex.net/asr_xml?uuid=12345678123456781234567812345678&topic=general&lang=ru-RU&key=6372dda5-9674-4413-85ff-e9d0eb2f99a7";

		curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());

		std::ifstream fileStream(filename, std::ifstream::binary);
		fileStream.seekg(0, fileStream.end);
		int length = fileStream.tellg();
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
		ofstream myfile;
		myfile.open("example.txt");
		myfile << "Http code is " << httpCode << "\n";
		myfile << contentStream.str() << "\n";
		myfile.close();

		std::string str = contentStream.str();
		std::size_t pos = str.find("<variant confidence=\"0\">");
		std::string str3 = str.substr(pos);

		str3 = str3.substr(24);
		str3 = str3.substr(0, str3.find("<"));
		str3 = utf2cp(str3);

		char* cstr = &str3[0];
		pSAMP->SendChat(cstr);
		curl_free(headers);
		curl_easy_cleanup(curl);

	}

	return 0;
}

// display error messages
void Error(const char* es)
{
	char mes[200];
	sprintf(mes, "%s\n(error code: %d)", es, BASS_ErrorGetCode());
	pSAMP->AddChatMessage(-1, mes);
}

BOOL CALLBACK RecordingCallback(HRECORD handle, const void* buffer, DWORD length, void* user)
{
	// increase buffer size if needed
	if ((reclen % BUFSTEP) + length >= BUFSTEP) {
		recbuf = (BYTE*)realloc(recbuf, ((reclen + length) / BUFSTEP + 1) * BUFSTEP);
		if (!recbuf) {
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
	if (recbuf) { // free old recording
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
	if (!rchan) {
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
	if (!(fp = fopen("SSTT.wav", "wb"))) {
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
	if (!BASS_RecordInit(device)) {
		Error("Can't initialize recording device");
		return FALSE;
	}
	{ // get list of inputs
		int c;
		const char* i;
		input = 0;
		for (c = 0; i = BASS_RecordGetInputName(c); c++) {
			if (!(BASS_RecordGetInput(c, NULL) & BASS_INPUT_OFF)) { // this one is currently "on"
				input = c;
			}
		}
	}
	return TRUE;
}


void MainThread()
{
	bool initialized = false;

	while (true)
	{
		if (!initialized)
		{
			if (!pSAMP->IsInitialized()) continue;

			pSAMP->AddChatMessage(-1, "SSTT Loaded. Author: http://qrlk.me");

			{
				int c, def;
				BASS_DEVICEINFO di;
				for (c = 0; BASS_RecordGetDeviceInfo(c, &di); c++) {
					if (di.flags & BASS_DEVICE_DEFAULT) {
						def = c;
					}
				}
				InitDevice(def);
			}

			initialized = true;

			/*code*/
		}
		if (GetKeyState('R') & 0x8000)
		{
			Sleep(200);
			if (!(GetKeyState('R') & 0x8000)) {
				continue;
			}
			pSAMP->AddChatMessage(-1, "Started Recording");
			StartRecording();
			while (GetKeyState('R') & 0x8000) {
				Sleep(100);
				if (reclen > 1024000)
				{
					pSAMP->AddChatMessage(-1, "[SSTT]: Вы достигли максимального размера файла. Запись прекращена.");
					break;
				}
			}
			pSAMP->AddChatMessage(-1, "Recording Finished");
			StopRecording();
			pSAMP->AddChatMessage(-1, "Saving...");
			WriteToDisk();
			pSAMP->AddChatMessage(-1, "Saved!");
			recognition("SSTT.wav");
			pSAMP->AddChatMessage(-1, "Done!");
		}
	}
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReasonForCall, LPVOID lpReserved)
{
	switch (dwReasonForCall)
	{
	case DLL_PROCESS_ATTACH:
		pSAMP = new SAMP(GetModuleHandleA("SAMP.DLL"));
		beginThread(MainThread);
		break;
	}
	return TRUE;
}