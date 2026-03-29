#include <ntifs.h>
#include "VadWalker.h"
#include "VadTypes.h"


VOID WalkVADTree(RTL_BALANCED_NODE* node, PEPROCESS Process) {
    if (!node) return;

    PMMVAD vad = (PMMVAD)node;
    ULONG baseProtection = vad->Core.u.VadFlags.Protection & 0x7;

    KdPrint(("[ScoutAC] Base protection: %u\n", baseProtection));
    KdPrint(("[ScoutAC] Private Memory: %u\n", vad->Core.u.VadFlags.PrivateMemory));

    ULONG64 start = ((ULONG64)vad->Core.StartingVpnHigh << 32 | vad->Core.StartingVpn) << PAGE_SHIFT;
    ULONG64 end = ((ULONG64)vad->Core.EndingVpnHigh << 32 | vad->Core.EndingVpn) << PAGE_SHIFT;

    if (vad->Core.u.VadFlags.PrivateMemory && (baseProtection == 6 || baseProtection == 7)) { //MM_EXECUTE_READWRITE and MM_EXECUTE_WRITECOPY

        KdPrint(("[ScoutAC] RWX private region: 0x%llx - 0x%llx\n", start, end));
    }
    // VAD stores original protection from allocation
    // If current protection has execute but VAD says RW, the region was likely injected code
    if (vad->Core.u.VadFlags.PrivateMemory && baseProtection == 4) { 
       
        ULONG64 addr = start;
        while (addr < end) {
            MEMORY_BASIC_INFORMATION mbi;
            SIZE_T retLen;
            NTSTATUS status = ZwQueryVirtualMemory(NtCurrentProcess(),
                (PVOID)addr,
                MemoryBasicInformation,
                &mbi,
                sizeof(mbi),
                &retLen
            );

            if (!NT_SUCCESS(status)) break;

            if (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ |
                PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) {
                KdPrint(("[ScoutAC] Protection flip: VAD=%u, Current=0x%X at %llx\n",vad->Core.u.VadFlags.Protection, mbi.Protect, addr));
            }
            addr = (ULONG64)mbi.BaseAddress + mbi.RegionSize;

        }
    }



    WalkVADTree(node->Left, Process);
    WalkVADTree(node->Right, Process);
}

VOID VadWalk(PEPROCESS Process) {
	PRTL_AVL_TREE vadTree = (PRTL_AVL_TREE)((PUCHAR)Process + 0x558); // win11 25h2 (https://www.vergiliusproject.com/kernels/x64/windows-11/25h2/_EPROCESS)


    HANDLE Pid = PsGetProcessId(Process);

    KdPrint(("[ScoutAC] Starting VAD scan for pid: %X\n", Pid));

    KAPC_STATE apcState;
    KeStackAttachProcess(Process, &apcState);
	WalkVADTree(vadTree->Root, Process);
    KeUnstackDetachProcess(&apcState);

    KdPrint(("[ScoutAC] Finished VAD scan for pid: %X\n", Pid));
}

