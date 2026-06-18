#include <ntifs.h>
#include "scanner.h"
#include "ProcessList.h"
#include "ApcChecker.h"
#include "VadWalker.h"
#include "DetectDrivers.h"
#include "AttachedDetection.h"


HANDLE gThreadHandle;
volatile BOOLEAN gRunning = TRUE;
KEVENT gWakeEvent;

static VOID ScanDpc(PKDPC Dpc, PVOID Context, PVOID Arg1, PVOID Arg2) {
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(Arg1);
    UNREFERENCED_PARAMETER(Arg2);
    KeSetEvent((PKEVENT)Context, 0, FALSE);
}

VOID ScannerThread(PVOID Context) {
    UNREFERENCED_PARAMETER(Context);

    KeInitializeEvent(&gWakeEvent, SynchronizationEvent, FALSE);

    KdPrint(("[ScoutAC] Initialized watchdog thread.\n"));

    while (gRunning) {

        LARGE_INTEGER interval;
        interval.QuadPart = -10000000LL; // 1 second in 100ns units

        NTSTATUS status = KeWaitForSingleObject(&gWakeEvent, Executive, KernelMode, FALSE, &interval);

        if (!gRunning || status == STATUS_SUCCESS) {
            break;
        }



        PEPROCESS proc = ProcessList_GetProtectedProcess();
        if (proc) {

            VadWalk(proc);
            CheckAPCForSus(proc); // doesnt rlly work for kernel, it can catch usermode apc injections tho
            ObDereferenceObject(proc);

        }

        CheckKernelThreads();
        CheckSusAttachements();
       

    }
    PsTerminateSystemThread(STATUS_SUCCESS);
}