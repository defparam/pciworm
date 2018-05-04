#pragma once

#include <windows.h>
#include <strsafe.h>
#include <cfgmgr32.h>
#include <stdio.h>
#include <stdlib.h>
#include <initguid.h>
#define INITGUID
#include <Public.h>

#define MAX_DEVPATH_LENGTH 256

class pciworm_if
{
public:
	pciworm_if();
	int open();
	int close();
	int test();
	int fpgaRead64(LONG64 addr, PULONG64 data);
	int fpgaWrite64(LONG64 addr, ULONG64 data);
	int memRead64(LONG64 addr, PULONG64 data);
	int memWrite64(LONG64 addr, ULONG64 data);
private:
	WCHAR G_DevicePath[MAX_DEVPATH_LENGTH];
	HANDLE hDevice = INVALID_HANDLE_VALUE;
};

