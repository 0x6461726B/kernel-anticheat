#include <ntifs.h>
#include "DetectDrivers.h"

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

//offsets are from win11 25h2
VOID CheckKernelThreads(VOID) {

	PEPROCESS sysProc = PsInitialSystemProcess;
	PULONG_PTR pushLockProcess = (PULONG_PTR)((PUCHAR)sysProc + 0x1C8); // ProcessLock

	KeEnterCriticalRegion();
	ExAcquirePushLockSharedEx(pushLockProcess, 0);


	LIST_ENTRY* threadListHead = (LIST_ENTRY*)((PUCHAR)sysProc + 0x370);
	LIST_ENTRY* entry = threadListHead->Flink;

	while (entry != threadListHead) {

		PETHREAD threadEntry = (PETHREAD)((PUCHAR)entry - 0x578);


		ULONG_PTR threadStartAddy = *(ULONG_PTR*)((PUCHAR)threadEntry + 0x560);

		PLIST_ENTRY moduleHead = PsLoadedModuleList;
		PLIST_ENTRY moduleListEntry = moduleHead->Flink;


		BOOLEAN isModuleLegit = FALSE;

		while (moduleListEntry != moduleHead) {
			PLDR_DATA_TABLE_ENTRY moduleEntry = CONTAINING_RECORD(moduleListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

			ULONG_PTR dllBase = (ULONG_PTR)moduleEntry->DllBase;
			ULONG_PTR dllEnd = dllBase + moduleEntry->SizeOfImage;

			if (threadStartAddy >= dllBase && threadStartAddy <= dllEnd) {
				isModuleLegit = TRUE;
				break;
			}
			moduleListEntry = moduleListEntry->Flink;
		}
		if (!isModuleLegit) {
			KdPrint(("[ScoutAC] Suspicious kernel thread start address: 0x%llx\n", threadStartAddy));
		}
			

		entry = entry->Flink;
	}

	ExReleasePushLockSharedEx(pushLockProcess, 0);
	KeLeaveCriticalRegion();
}