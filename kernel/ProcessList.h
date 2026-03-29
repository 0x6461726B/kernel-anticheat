#pragma once

NTSTATUS ProcessList_Add(HANDLE pid);
VOID ProcessList_Remove(HANDLE pid);
BOOLEAN ProcessList_IsProtected(PEPROCESS process);
BOOLEAN ProcessList_IsProtectedPid(HANDLE pid);
VOID ProcessList_Init(VOID);
VOID ProcessList_Cleanup(VOID);
PEPROCESS ProcessList_GetProtectedProcess(VOID);