#pragma once
#include "shared.h"

NTSTATUS ProcessList_Add(HANDLE pid);
VOID ProcessList_Remove(HANDLE pid);
BOOLEAN ProcessList_IsProtected(PEPROCESS process);
VOID ProcessList_Init(VOID);
VOID ProcessList_Cleanup(VOID);
