#pragma once
#include "alerts.h"
#include "globals.h"
#include <shared.h>

VOID SendAlertToUserMode(ULONG ViolationId, PCWSTR Details)
{
	PIRP IrpToComplete = NULL;
	KIRQL oldIrql;

	KeAcquireSpinLock(&g_QueueLock, &oldIrql);
	IrpToComplete = g_PendingDetectionIrp;
	g_PendingDetectionIrp = NULL;
	KeReleaseSpinLock(&g_QueueLock, oldIrql);

	if (IrpToComplete == NULL) {
		return;
	}


	PKERNEL_ALERT alert = (PKERNEL_ALERT)IrpToComplete->AssociatedIrp.SystemBuffer;
	alert->ViolationType = ViolationId;
	wcscpy_s(alert->Description, 256, Details);

	IrpToComplete->IoStatus.Status = STATUS_SUCCESS;
	IrpToComplete->IoStatus.Information = sizeof(KERNEL_ALERT);

	IoCompleteRequest(IrpToComplete, IO_NO_INCREMENT);
}


KSPIN_LOCK g_QueueLock;
PIRP g_PendingDetectionIrp = NULL;


void StorePendingIrp(PIRP Irp) {

	KIRQL oldIrql;
	KeAcquireSpinLock(&g_QueueLock, &oldIrql);

	PIRP oldIrp = g_PendingDetectionIrp;
	g_PendingDetectionIrp = Irp;

	KeReleaseSpinLock(&g_QueueLock, oldIrql);

	if (oldIrp != NULL) {
		oldIrp->IoStatus.Status = STATUS_CANCELLED;
		oldIrp->IoStatus.Information = 0;
		IoCompleteRequest(oldIrp, IO_NO_INCREMENT);
	}
}


