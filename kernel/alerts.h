#pragma once
#include <ntifs.h>
#include <fltKernel.h>
extern PFLT_FILTER g_FilterHandle;
extern PFLT_PORT   g_ServerPort;
extern PFLT_PORT   g_ClientPort;


NTSTATUS InitFltComms(PDRIVER_OBJECT driverObject);