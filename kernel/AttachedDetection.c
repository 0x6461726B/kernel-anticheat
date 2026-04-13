#include "ntifs.h"
#include "AttachedDetection.h"
#include "ProcessList.h"

typedef struct _LDR_DATA_TABLE_ENTRY {
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;


extern PLIST_ENTRY PsLoadedModuleList;

extern PVOID g_DriverBase;
extern ULONG g_DriverSize;


BOOLEAN IsAddressInKnownModule(PVOID Address) {

	PLIST_ENTRY moduleHead = PsLoadedModuleList;
	PLIST_ENTRY moduleListEntry = moduleHead->Flink;


	BOOLEAN isModuleLegit = FALSE;

	while (moduleListEntry != moduleHead) {
		PLDR_DATA_TABLE_ENTRY moduleEntry = CONTAINING_RECORD(moduleListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		ULONG_PTR dllBase = (ULONG_PTR)moduleEntry->DllBase;
		ULONG_PTR dllEnd = dllBase + moduleEntry->SizeOfImage;

		if (Address >= (PVOID)dllBase && Address < (PVOID)dllEnd) {
			isModuleLegit = TRUE;
			break;
		}
		moduleListEntry = moduleListEntry->Flink;
	}

	return isModuleLegit;
}

//this only catches lazy memory readers who stay attached to process 
VOID CheckSusAttachements(VOID) {



	PEPROCESS protectedProcess = ProcessList_GetProtectedProcess();

	if (!protectedProcess)
		return;


	PEPROCESS sysProc = PsInitialSystemProcess;

	PULONG_PTR pushLockProcess = (PULONG_PTR)((PUCHAR)sysProc + 0x1C8); // ProcessLock

	KeEnterCriticalRegion();
	ExAcquirePushLockSharedEx(pushLockProcess, 0);

	LIST_ENTRY* threadListHead = (LIST_ENTRY*)((PUCHAR)sysProc + 0x370);
	LIST_ENTRY* threadEntry = threadListHead->Flink;

	while (threadEntry != threadListHead) {
		PKTHREAD realThread = (PKTHREAD)((PUCHAR)threadEntry - 0x578);
		PKAPC_STATE apcState = (PKAPC_STATE)((PUCHAR)realThread + 0x98);

		UCHAR apcStateIndex = *(UCHAR*)((PUCHAR)realThread + 0x24a);

		if (apcStateIndex == 1 && apcState->Process == protectedProcess) {			

			PVOID startAddy = *(PVOID*)((PUCHAR)realThread + 0x560);

			if ((ULONG_PTR)startAddy >= (ULONG_PTR)g_DriverBase && (ULONG_PTR)startAddy < (ULONG_PTR)g_DriverBase + (ULONG_PTR)g_DriverSize)
				goto next;


			if (!IsAddressInKnownModule(startAddy)) {
				KdPrint(("[ScoutAC] Manually mapped driver attached: %p\n", startAddy));

			}
			else {
				KdPrint(("[ScoutAC] Loaded driver attached: %p\n", startAddy));

			}

		}


		next:
		   threadEntry = threadEntry->Flink;
	}

	ExReleasePushLockSharedEx(pushLockProcess, 0);
	KeLeaveCriticalRegion();
	ObDereferenceObject(protectedProcess);

}