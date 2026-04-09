#include "ntifs.h"
#include "AttachedDetection.h"
#include "ProcessList.h"

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

		//UCHAR apcStateIndex = *(UCHAR*)((PUCHAR)realThread + 0x24a);

		if (/*apcStateIndex == 1 && */apcState->Process == protectedProcess) {

			PEPROCESS sourceProc = ((PKAPC_STATE)((PUCHAR)realThread + 0x258))->Process;
			
			KdPrint(("[ScoutAC] Suspicious attach detected! Thread: %p, SavedApcState.Process: %p\n",
				realThread,
				((PKAPC_STATE)((PUCHAR)realThread + 0x258))->Process //savedapcstate
				)); 

		}



		threadEntry = threadEntry->Flink;
	}

	ExReleasePushLockSharedEx(pushLockProcess, 0);
	KeLeaveCriticalRegion();

}