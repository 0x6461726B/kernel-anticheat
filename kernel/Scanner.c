#include <ntifs.h>
#include "scanner.h"
#include "ProcessList.h"
#include "ApcChecker.h"
#include "VadWalker.h"
#include "DetectDrivers.h"
#include "AttachedDetection.h"

KTIMER gTimer;
KDPC gDpc;
HANDLE gThreadHandle;
BOOLEAN gRunning = TRUE;
KEVENT gWakeEvent;

static VOID ScanDpc(KDPC Dpc, PVOID Context, PVOID Arg1, PVOID Arg2) {
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(Arg1);
    UNREFERENCED_PARAMETER(Arg2);
    KeSetEvent((PKEVENT)Context, 0, FALSE);
}

VOID ScannerThread(PVOID Context) {
    UNREFERENCED_PARAMETER(Context);

    KeInitializeEvent(&gWakeEvent, SynchronizationEvent, FALSE);
    KeInitializeTimer(&gTimer);
    KeInitializeDpc(&gDpc, (PKDEFERRED_ROUTINE)ScanDpc, &gWakeEvent);

    KdPrint(("[ScoutAC] Initialized watchdog thread.\n"));

    while (gRunning) {
        LARGE_INTEGER interval;
        interval.QuadPart = -10000000LL; // 1 second in 100ns units

        KeSetTimer(&gTimer, interval, &gDpc);
        KeWaitForSingleObject(&gWakeEvent, Executive, KernelMode, FALSE, NULL);

        if (!gRunning) break;



        PEPROCESS proc = ProcessList_GetProtectedProcess();
        if (proc) {

            VadWalk(proc);
            //PteScan(proc);
            //CheckAPCForSus(proc);
            ObDereferenceObject(proc);

        }

        CheckKernelThreads();
        CheckSusAttachements();
       

    }
    PsTerminateSystemThread(STATUS_SUCCESS);
}