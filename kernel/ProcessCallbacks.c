#include <ntifs.h>
#include "ProcessCallbacks.h"
#include "ProcessList.h"
VOID OnProcessNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO  CreateInfo) {

	if (!CreateInfo) {
		if (ProcessList_IsProtected(Process)) {
			ProcessList_Remove(Process);
			KdPrint(("[ScoutAC] Protected process %llu exited, removed from list\n", (ULONG64)ProcessId));
		}
	}
}

NTSTATUS ProcessCallbacks_Register(VOID) {
	return PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, FALSE);
}

VOID ProcessCallbacks_Unregister(VOID) {
	PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);

}