#pragma once
#include <ntifs.h>
#include <shared.h>
#include "alerts.h"

typedef PVOID(*MiGetPteAdress_t)(ULONG64 va);
extern MiGetPteAdress_t MiGetPteAddress;

extern PDEVICE_OBJECT pDeviceObject;
extern PDRIVER_UNLOAD g_OriginalUnloadRoutine;

VOID SendAlertToUserMode(ULONG ViolationId, PCWSTR Details);

extern KSPIN_LOCK g_QueueLock;
extern PIRP g_PendingDetectionIrp;

void StorePendingIrp(PIRP Irp);
