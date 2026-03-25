#include <ntifs.h>

#include "processlist.h"

#define MAX_PROTECTED 1

typedef struct _PROTECTED_PROCESS {
	HANDLE Pid;
	PEPROCESS Process;
} PROTECTED_PROCESS;

static PROTECTED_PROCESS g_ProtectedList[MAX_PROTECTED];
static ULONG g_ProtectedCount;
static KSPIN_LOCK g_Lock;

VOID ProcessList_Init(VOID) {
	KeInitializeSpinLock(&g_Lock);
}

NTSTATUS ProcessList_Add(HANDLE pid) {


	PEPROCESS process = NULL;
	NTSTATUS status = PsLookupProcessByProcessId(pid, &process);
	if (!NT_SUCCESS(status))
		return status;

	KIRQL irql;
	KeAcquireSpinLock(&g_Lock, &irql);
	if (g_ProtectedCount >= MAX_PROTECTED) {
		KeReleaseSpinLock(&g_Lock, irql);
		ObDereferenceObject(process);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	g_ProtectedList[g_ProtectedCount].Pid = pid;
	g_ProtectedList[g_ProtectedCount].Process = process;
	g_ProtectedCount++;

	KeReleaseSpinLock(&g_Lock, irql);

	return STATUS_SUCCESS;
}

VOID ProcessList_Remove(HANDLE pid) {
	PEPROCESS toDeref = NULL;
	KIRQL irql;
	KeAcquireSpinLock(&g_Lock, &irql);


	for (ULONG i = 0; i < g_ProtectedCount; i++) {
		if (g_ProtectedList[i].Pid == pid) {
			toDeref = g_ProtectedList[i].Process;
			g_ProtectedList[i] = g_ProtectedList[g_ProtectedCount - 1];
			RtlZeroMemory(&g_ProtectedList[g_ProtectedCount - 1], sizeof(PROTECTED_PROCESS));
			g_ProtectedCount--;
			break;
		}
	}

	KeReleaseSpinLock(&g_Lock, irql);

	if (toDeref)
		ObDereferenceObject(toDeref);
}

BOOLEAN ProcessList_IsProtected(PEPROCESS process) {
	KIRQL irql;
	KeAcquireSpinLock(&g_Lock, &irql);

	for (ULONG i = 0; i < g_ProtectedCount; i++) {
		if (g_ProtectedList[i].Process == process) {
			KeReleaseSpinLock(&g_Lock, irql);
			return TRUE;
		}
	}
	KeReleaseSpinLock(&g_Lock, irql);
	return FALSE;
}

VOID ProcessList_Cleanup(VOID) {
	KIRQL irql;
	KeAcquireSpinLock(&g_Lock, &irql);
 
	for (ULONG i = 0; i < g_ProtectedCount; i++) {
		if (g_ProtectedList[i].Process) {
			ObDereferenceObject(g_ProtectedList[i].Process);
		}
	}
	RtlZeroMemory(g_ProtectedList, sizeof(g_ProtectedList));
	g_ProtectedCount = 0;
	KeReleaseSpinLock(&g_Lock, irql);

}