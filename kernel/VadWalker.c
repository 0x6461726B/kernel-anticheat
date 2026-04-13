#include <ntifs.h>
#include "globals.h"
#include "VadWalker.h"
#include "VadTypes.h"


static VOID CheckPteFlipsInRange(PEPROCESS Process, ULONG64 startVa, ULONG64 endVa) {
    ULONGLONG cr3 = *(ULONGLONG*)((PUCHAR)Process + 0x28); // KPROCESS DirectoryTableBase
    PHYSICAL_ADDRESS physAddy;
    physAddy.QuadPart = cr3;

    PPML4E pml4 = (PPML4E)MmGetVirtualForPhysical(physAddy);
    if (!pml4 || !MmIsAddressValid(pml4)) return;

    for (ULONG64 va = startVa; va < endVa; va += PAGE_SIZE) {
        // decompose VA into table indices
        ULONG pml4Index = (va >> 39) & 0x1FF;
        ULONG pdptIndex = (va >> 30) & 0x1FF;
        ULONG pdeIndex = (va >> 21) & 0x1FF;
        ULONG pteIndex = (va >> 12) & 0x1FF;

        if (!pml4[pml4Index].Present) { va = (va & ~0x7FFFFFFFFFULL) + 0x8000000000ULL - PAGE_SIZE; continue; } // align to boundary, then add size of region to skip rest

        physAddy.QuadPart = (ULONGLONG)pml4[pml4Index].PageFrameNumber << PAGE_SHIFT;
        PPDPT pdpt = (PPDPT)MmGetVirtualForPhysical(physAddy);
        if (!pdpt || !MmIsAddressValid(pdpt)) continue;

        if (!pdpt[pdptIndex].Present) { va = (va & ~0x3FFFFFFFULL) + 0x40000000ULL - PAGE_SIZE; continue; }
        if (pdpt[pdptIndex].PageSize) continue; // 1gb page, skip

        physAddy.QuadPart = (ULONGLONG)pdpt[pdptIndex].PageFrameNumber << PAGE_SHIFT;
        PPDE pde = (PPDE)MmGetVirtualForPhysical(physAddy);
        if (!pde || !MmIsAddressValid(pde)) continue;

        if (!pde[pdeIndex].Present) { va = (va & ~0x1FFFFFULL) + 0x200000ULL - PAGE_SIZE; continue; }
        if (pde[pdeIndex].PageSize) continue; // 2mb page, skip

        physAddy.QuadPart = (ULONGLONG)pde[pdeIndex].PageFrameNumber << PAGE_SHIFT;
        PPTE pte = (PPTE)MmGetVirtualForPhysical(physAddy);
        if (!pte || !MmIsAddressValid(pte)) continue;

        if (!pte[pteIndex].Present) continue;

        // PTE says executable but VAD said non-executable -> flip
        if (pte[pteIndex].ExecuteDisable == 0) {
            KdPrint(("[ScoutAC] PTE flip detected at VA: 0x%llx\n", va));
        }
    }
}

//MiGetFirstVad implementation
static RTL_BALANCED_NODE* GetFirstVad(PEPROCESS Process) {
    RTL_BALANCED_NODE* Root = *(RTL_BALANCED_NODE**)((PUCHAR)Process + 0x558); // win11 25h2 (https://www.vergiliusproject.com/kernels/x64/windows-11/25h2/_EPROCESS)

    if (!Root)
        return NULL;

    while (Root->Children[0])
        Root = Root->Children[0];

    return Root;

}

//MiGetNextVad implementation
static RTL_BALANCED_NODE* GetNextVad(RTL_BALANCED_NODE* Node) {
    RTL_BALANCED_NODE* RetNode = Node->Children[1];

    if (RetNode) {
        for (RTL_BALANCED_NODE* i = RetNode->Children[0]; i; i = i->Children[0]) {
            RetNode = i;
        }
    }
    else {
        for (ULONG_PTR j = Node->ParentValue; ; j = RetNode->ParentValue) {
            RetNode = (RTL_BALANCED_NODE*)(j & 0xFFFFFFFFFFFFFFFC);

            if (!RetNode || RetNode->Children[0] == Node) {
                break;
            }
            Node = RetNode;
        }
    }

    return RetNode;

}

static BOOLEAN IsExecuteableVadProtection(ULONG protection) {
    return protection == MM_EXECUTE ||
        protection == MM_EXECUTE_READ ||
        protection == MM_EXECUTE_READWRITE ||
        protection == MM_EXECUTE_WRITECOPY;
}


static BOOLEAN IsNonExecutableVadProtection(ULONG protection) {
    return protection == MM_READONLY ||
        protection == MM_READWRITE ||
        protection == MM_WRITECOPY;
}

// 1. check for private RWX regions
// 2. check for protection flipping from non execute to execute
// 3. check for section mapped regions (ZwMapViewOfSection) with no file object backing it
VOID VadWalk(PEPROCESS Process) {

    KAPC_STATE apcState;
    KeStackAttachProcess(Process, &apcState);


    KeEnterCriticalRegion();
    ExAcquirePushLockSharedEx((PEX_PUSH_LOCK)((PUCHAR)Process + 0x258), 0); // 0x258 = AddressCreationLock

    RTL_BALANCED_NODE* node = GetFirstVad(Process);

    while (node) {

        //BOOLEAN protectionFlipped = FALSE;
        PMMVAD vad = (PMMVAD)node;
        ULONG baseProtection = vad->Core.u.VadFlags.Protection & 0x7;

        /*KdPrint(("[ScoutAC] Base protection: %u\n", baseProtection));
        KdPrint(("[ScoutAC] Private Memory: %u\n", vad->Core.u.VadFlags.PrivateMemory));*/

        ULONG64 start = ((ULONG64)vad->Core.StartingVpnHigh << 32 | vad->Core.StartingVpn) << PAGE_SHIFT;
        ULONG64 end = (((ULONG64)vad->Core.EndingVpnHigh << 32 | vad->Core.EndingVpn) + 1) << PAGE_SHIFT;

        if (vad->Core.u.VadFlags.PrivateMemory && IsExecuteableVadProtection(baseProtection)) {

            KdPrint(("[ScoutAC] RWX private region: 0x%llx - 0x%llx\n", start, end));
        }
        // VAD stores original protection from allocation
        // If current protection has execute but VAD has non execute, the region was likely injected code
        if (vad->Core.u.VadFlags.PrivateMemory && IsNonExecutableVadProtection(baseProtection)) {

            CheckPteFlipsInRange(Process, start, end);

             //ULONG64 addr = start;


             //while (addr < end) {
             //    MEMORY_BASIC_INFORMATION mbi;
             //    SIZE_T retLen;
             //    NTSTATUS status = ZwQueryVirtualMemory(NtCurrentProcess(),
             //        (PVOID)addr,
             //        MemoryBasicInformation,
             //        &mbi,
             //        sizeof(mbi),
             //        &retLen
             //    );



             //    if (!NT_SUCCESS(status)) break;

             //    //KdPrint(("[ScoutAC] Region size %u\n", mbi.RegionSize));


             //    if (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ |
             //        PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) {
             //        KdPrint(("[ScoutAC] Protection flip: VAD=%u, Current=0x%X at %llx\n", vad->Core.u.VadFlags.Protection, mbi.Protect, addr));
             //    }
             //    addr = (ULONG64)mbi.BaseAddress + mbi.RegionSize;

             //}
        }
        

        if (!vad->Core.u.VadFlags.PrivateMemory && vad->FileObject == NULL && vad->Subsection != NULL) {
            PCONTROL_AREA ca = vad->Subsection->ControlArea;

            if (ca != NULL) {
                if (!ca->u.Flags.Image) {
                    ULONG protection = vad->Core.u.VadFlags.Protection;
                    if (IsExecuteableVadProtection(protection))
                        KdPrint(("[ScoutAC] Sus mapped region found: 0x%llx - 0x%llx\n", start, end));
                }
            }

        }

        //if (protectionFlipped) {
        //    KdPrint(("[ScoutAC] Protection flip detected.\n"));
        //}


        node = GetNextVad(node);
    }

    ExReleasePushLockSharedEx((PEX_PUSH_LOCK)((PUCHAR)Process + 0x258), 0);
    KeLeaveCriticalRegion();

    KeUnstackDetachProcess(&apcState);

}

