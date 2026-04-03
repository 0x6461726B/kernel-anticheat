#include <ntifs.h>
#include "ApcChecker.h"

//offsets are for win11 25h2
VOID CheckAPCForSus(PEPROCESS Process) {
	KAPC_STATE apcStateStack;
	KeStackAttachProcess(Process, &apcStateStack);

	LIST_ENTRY* threadListHead = (LIST_ENTRY*)((PUCHAR)Process + 0x370);
	LIST_ENTRY* entry = threadListHead->Flink;

	while (entry != threadListHead) {
		PKTHREAD thread = (PKTHREAD)((PUCHAR)entry - 0x578);

		PKAPC_STATE apcState = (PKAPC_STATE)((PUCHAR)thread + 0x98);

		PLIST_ENTRY apcListHead = (PLIST_ENTRY)((PUCHAR)apcState + 0x10); // ApcListHead[1]

		PLIST_ENTRY apcEntry = apcListHead->Flink;



		while (apcEntry != apcListHead) {
			PKAPC apc = CONTAINING_RECORD(apcEntry, KAPC, ApcListEntry);

			ULONG_PTR normalRoutine = *(ULONG_PTR*)((PUCHAR)apc + 0x030);

			if (normalRoutine != 0) {
				MEMORY_BASIC_INFORMATION mbi;
				SIZE_T retLen;
				NTSTATUS status = ZwQueryVirtualMemory(ZwCurrentProcess(), (PVOID)normalRoutine, MemoryBasicInformation, &mbi, sizeof(mbi), &retLen);
				if (NT_SUCCESS(status)) {
					if (mbi.Type == MEM_MAPPED && mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) {
						KdPrint(("[ScoutAC] Sus APC routine in anonymous mapped region: 0x%llx\n", normalRoutine));
					}
					if (mbi.Type == MEM_PRIVATE && mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) {
						KdPrint(("[ScoutAC] Sus APC routine in private executable region: 0x%llx\n", normalRoutine));
					}
				}
			}
			apcEntry = apcEntry->Flink;
		}
		entry = entry->Flink;
	
	}
	KeUnstackDetachProcess(&apcStateStack);
}