#include <ntifs.h>
#include "ThreadCallbacks.h"
#include "ProcessList.h"
#include "shared.h"

#define THREAD_QUERY_INFORMATION         (0x0040)  

NTSYSAPI NTSTATUS NTAPI ZwQueryInformationThread(
	HANDLE ThreadHandle,
	THREADINFOCLASS ThreadInformationClass,
	PVOID ThreadInformation,
	ULONG ThreadInformationLength,
	PULONG ReturnLength
);

VOID OnThreadCreate(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create) {
	if (!Create)
		return;

	PEPROCESS process = NULL;
	if (!NT_SUCCESS(PsLookupProcessByProcessId(ProcessId, &process))) {
		return;
	}

	if (!ProcessList_IsProtected(process)) {
		ObDereferenceObject(process);
		return;
	}

	PETHREAD thread = NULL;
	if (!NT_SUCCESS(PsLookupThreadByThreadId(ThreadId, &thread))) {
		ObDereferenceObject(process);
		return;
	}
	HANDLE hThread = NULL;
	NTSTATUS status = ObOpenObjectByPointer(
		thread,
		OBJ_KERNEL_HANDLE,
		NULL,
		THREAD_QUERY_INFORMATION,
		NULL,
		KernelMode,
		&hThread
	);

	if (!NT_SUCCESS(status)) {
		ObDereferenceObject(thread);
		ObDereferenceObject(process);
		return;
	}

	PVOID startAddy = NULL;
	ULONG retLen;
	status = ZwQueryInformationThread(hThread, ThreadQuerySetWin32StartAddress, &startAddy, sizeof(startAddy), &retLen);

	if (!NT_SUCCESS(status)) {
		ObDereferenceObject(thread);
		ObDereferenceObject(process);
		ZwClose(hThread);
		return;
	}


	HANDLE hProcess = NULL;

	 status = ObOpenObjectByPointer(
		process,
		OBJ_KERNEL_HANDLE,
		NULL,
		PROCESS_ALL_ACCESS,
		NULL,
		KernelMode,
		&hProcess
		);

	if (!NT_SUCCESS(status)) {
		ZwClose(hThread);
		ObDereferenceObject(thread);
		ObDereferenceObject(process);
		return;
	}

	MEMORY_BASIC_INFORMATION mbi = { 0 };
	SIZE_T retLen2;

	if (!NT_SUCCESS(ZwQueryVirtualMemory(
		hProcess,
		startAddy,
		MemoryBasicInformation,
		&mbi,
		sizeof(mbi),
		&retLen2
	))) {
		ZwClose(hProcess);
		ObDereferenceObject(thread);
		ObDereferenceObject(process);
		return;
	}

	if (mbi.Type != MEM_IMAGE) {
		KdPrint(("[ScoutAC] Unbacked Thread detected at %p EPROCESS: %p\n", startAddy, process));
	}
	ZwClose(hThread);
	ZwClose(hProcess);
	ObDereferenceObject(thread);
	ObDereferenceObject(process);


}

NTSTATUS ThreadCallbacks_Register(VOID) {
	return PsSetCreateThreadNotifyRoutine(OnThreadCreate);
}

VOID ThreadCallbacks_Unregister(VOID) {
	PsRemoveCreateThreadNotifyRoutine(OnThreadCreate);
}