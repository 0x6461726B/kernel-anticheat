#include <ntifs.h>
#include "VadWalker.h"
#include "VadTypes.h"


VOID WalkVADTree(RTL_BALANCED_NODE* node) {
    if (!node) return;

    PMMVAD vad = (PMMVAD)node;
    ULONG baseProtection = vad->Core.u.VadFlags.Protection & 0x7;

    KdPrint(("Base protection: %u\n", baseProtection));
    KdPrint(("Private Memory: %u\n", vad->Core.u.VadFlags.PrivateMemory));

    if (vad->Core.u.VadFlags.PrivateMemory && (baseProtection == 6 || baseProtection == 7)) { //MM_EXECUTE_READWRITE and MM_EXECUTE_WRITECOPY

        ULONG64 start = ((ULONG64)vad->Core.StartingVpnHigh << 32 | vad->Core.StartingVpn) << PAGE_SHIFT;
        ULONG64 end = ((ULONG64)vad->Core.EndingVpnHigh << 32 | vad->Core.EndingVpn) << PAGE_SHIFT;

        KdPrint(("[ScoutAC] RWX private region: 0x%llx - 0x%llx\n", start, end));

    }

    WalkVADTree(node->Left);
    WalkVADTree(node->Right);
}

VOID VadWalk(PEPROCESS Process) {
	PRTL_AVL_TREE vadTree = (PRTL_AVL_TREE)((PUCHAR)Process + 0x558); // win11 25h2 (https://www.vergiliusproject.com/kernels/x64/windows-11/25h2/_EPROCESS)

    HANDLE Pid = PsGetProcessId(Process);

    KdPrint(("[ScoutAC] Starting VAD scan for pid: %X\n", Pid));

	WalkVADTree(vadTree->Root);

    KdPrint(("[ScoutAC] Finished VAD scan for pid: %X\n", Pid));
}

