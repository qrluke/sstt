#pragma once

#include "iostream"
#include "plugin.h"
#include "common.h"

using namespace plugin;

#include "SAMP/samp.h"

extern SAMP *pSAMP;
extern CPed *PEDSELF;

SAMP *pSAMP;
CPed *PEDSELF;

void beginThread(void* __startAddress) { CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)__startAddress, NULL, NULL, NULL); }

LONG prevWndProc;
