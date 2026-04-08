#include <ntifs.h>
#include "VadWalker.h"
#include "VadTypes.h"


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
    ExAcquirePushLockSharedEx((PEX_PUSH_LOCK)(PUCHAR)Process + 0x258, 0); // 0x258 = AddressCreationLock

    RTL_BALANCED_NODE* node = GetFirstVad(Process);

    while (node) {

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
                    KdPrint(("[ScoutAC] Protection flip: VAD=%u, Current=0x%X at %llx\n", vad->Core.u.VadFlags.Protection, mbi.Protect, addr));
                }
                addr = (ULONG64)mbi.BaseAddress + mbi.RegionSize;

            }
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


        node = GetNextVad(node);
    }

    ExReleasePushLockSharedEx((PEX_PUSH_LOCK)(PUCHAR)Process + 0x258, 0);
    KeLeaveCriticalRegion();

    KeUnstackDetachProcess(&apcState);

}

