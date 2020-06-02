#include "iostream"
#include "SAMP/samp.h"

extern SAMP *pSAMP;

SAMP *pSAMP;


void beginThread(void *__startAddress) { CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)__startAddress, NULL, NULL, NULL); }

LONG prevWndProc;