#include "main.h"

class SAMP *pSAMP;

#define version "24.06.2020\n\n"

#define __URL "http://asr.yandex.net/asr_xml?uuid=12345678123456781234567812345678&disableAntimat=true&topic=general&lang=ru-RU&key=6372dda5-9674-4413-85ff-e9d0eb2f99a7"

#define FREQ 48000
#define CHANS 1
#define BUFSTEP 200000 // memory allocation unit

byte* recbuf = NULL;
size_t reclen = 0;
HRECORD rchan = 0;

byte* readptr = NULL;
size_t available = 0;

size_t read_request_data(void* ptr, size_t size, size_t nmemb, void* userdata)
{
	size_t _cur = min(size * nmemb, available);
	memcpy(ptr, readptr, _cur);
	readptr += _cur;
	available -= _cur;
	return _cur;
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

std::string recognition(void* file, size_t size)
{
	CURL* curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_POST, TRUE);
		curl_easy_setopt(curl, CURLOPT_HEADER, TRUE);

		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

		struct curl_slist* headers = NULL;
		headers = curl_slist_append(headers, "Content-Type: audio/x-wav");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		curl_easy_setopt(curl, CURLOPT_URL, __URL);

		readptr = (byte*) file;
		available = size;

		curl_easy_setopt(curl, CURLOPT_READFUNCTION, &read_request_data);
		//curl_easy_setopt(curl, CURLOPT_READDATA, file);
		curl_easy_setopt(curl, CURLOPT_INFILESIZE, size);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, size);

		std::string result;
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
		

		CURLcode code = curl_easy_perform(curl);

		uint32_t httpCode;
		curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &httpCode);

		std::string str3;

		if (httpCode == 200)
		{
			if (!strstr(result.c_str(), "<recognitionResults success=\"0\" />"))
			{
				str3 = result.substr(result.find("<variant confidence=\"0\">"));
				str3 = str3.substr(24);
				str3 = str3.substr(0, str3.find("<"));
			}
		}
		else
		{
			pSAMP->AddChatMessage(-1, "[SSTT]: Непредвиденная ошибка сетевого характера :(");
		}
		curl_free(headers);
		curl_easy_cleanup(curl);
		return str3;
	}
	return std::string();
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
			return FALSE;
		}
	}
	// buffer the data
	memcpy(recbuf + reclen, buffer, length);
	reclen += length;
	return TRUE;
}

void StartRecording()
{
	WAVEFORMATEX* wf;
	if (recbuf)
	{
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
		pSAMP->AddChatMessage(-1, "[SSTT] Can't start recording. BASS_ErrorGetCode() = %d", BASS_ErrorGetCode());
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

void CheckKey(char key)
{
	if (!(GetKeyState(key) & 0x8000))
		return;

	StartRecording();
	while (GetKeyState(key) & 0x8000)
	{
		Sleep(100);
		if (reclen > 1024000)
		{
			pSAMP->AddChatMessage(-1, "[SSTT]: Вы достигли максимального размера файла. Запись прекращена.");
			break;
		}
	}
	StopRecording();

	std::string text = recognition(recbuf, reclen);

	if (text.empty())
	{
		pSAMP->AddChatMessage(-1, "[SSTT]: Не удалось распознать/нет доступа к API!");
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
	case 'M':
		text.insert(0, "/me ");
		break;
	default:
		break;
	}

	std::string to_send = lite_conv(text, CP_UTF8, CP_ACP);
	if (to_send.length() > 128)
		to_send.resize(128);

	pSAMP->SendChat(to_send.c_str());

	/*
	static int counter = 0;
	counter++;
	pSAMP->AddChatMessage(-1, "[SSTT]: Done! Times: %d", counter);
	*/
}

DWORD WINAPI MainThread(LPVOID p)
{
	DWORD dwSAMPAddr = NULL;

	while (!(dwSAMPAddr = (DWORD)GetModuleHandleA("samp.dll")))
		Sleep(100);

	pSAMP = new SAMP(dwSAMPAddr);
	while (!pSAMP->IsInitialized())
		Sleep(100);

	if (!BASS_RecordInit(-1) && BASS_ErrorGetCode() != BASS_ERROR_ALREADY)
	{
		pSAMP->AddChatMessage(-1, "[SSTT] Ошибка инициализации. Микрофон не найден.");
		ExitThread(0);
	}

	pSAMP->AddChatMessage(-1, "SSTT v42.60.0202 инициализирован. Держите клавишу, потом отпустите. Автор: {348cb2}em.klrq");
	pSAMP->AddChatMessage(-1, "Клавиши: R - говорить, P - крикнуть, N - рация, M - /me, L - мегафон, B - /b");
	//checkUpd("http://qrlk.me/dev/moonloader/sstt/stats.php");

	while (true)
	{
		CheckKey('R');
		CheckKey('P');
		CheckKey('N');
		CheckKey('B');
		CheckKey('L');
		CheckKey('M');

		Sleep(10);
	}
	ExitThread(0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReasonForCall, LPVOID lpReserved)
{
	if (dwReasonForCall == DLL_PROCESS_ATTACH)
	{
		CreateThread(NULL, 0, MainThread, NULL, NULL, NULL);
	}
	return TRUE;
}