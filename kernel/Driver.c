#include <ntifs.h>

#include "alerts.h"
#include "shared.h"
#include "ObCallbacks.h"
#include "ThreadCallbacks.h"
#include "ProcessList.h"
#include "ImageCallbacks.h"
#include "ProcessCallbacks.h"
#include "VadWalker.h"
#include "Scanner.h"
#include "globals.h"

PVOID g_DriverBase = NULL;
ULONG g_DriverSize = 0;


PDRIVER_UNLOAD g_OriginalUnloadRoutine = NULL;

PDEVICE_OBJECT pDeviceObject = NULL;
static OB_CALLBACK_CONTEXT g_ObCtx = { 0 };

NTSTATUS DispatchCreateClose(PDEVICE_OBJECT DriverObject, PIRP irp) {
	UNREFERENCED_PARAMETER(DriverObject);

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
};

NTSTATUS MyIrpDeviceControlHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG controlCode = stack->Parameters.DeviceIoControl.IoControlCode;

	NTSTATUS status = STATUS_SUCCESS;
	ULONG_PTR bytesReturned = 0;

	KdPrint(("[ScoutAC] IOCTL Received: 0x%X\n", controlCode));

	switch (controlCode) {

	case IOCTL_PROTECT_PROCESS: {
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		ULONG pid = *(ULONG*)Irp->AssociatedIrp.SystemBuffer;
		status = ProcessList_Add((HANDLE)pid);

		if (NT_SUCCESS(status)) {
			pDeviceObject->DriverObject->DriverUnload = NULL;
			KdPrint(("[ScoutAC] Protected process 0x%X\n", pid));
		}

		break;
	}

	default: {
		KdPrint(("[ScoutAC] Unknown IOCTL: 0x%X\n", controlCode));
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = bytesReturned;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}


VOID DriverUnload(PDRIVER_OBJECT DriverObject) {



	if (g_ServerPort != NULL) {
		FltCloseCommunicationPort(g_ServerPort);
		g_ServerPort = NULL;
	}

	if (g_ClientPort != NULL && g_FilterHandle != NULL) {
		FltCloseClientPort(g_FilterHandle, &g_ClientPort);
		g_ClientPort = NULL;
	}

	if (g_FilterHandle != NULL) {
		FltUnregisterFilter(g_FilterHandle);
		g_FilterHandle = NULL;
	}

	UNICODE_STRING dosDeviceName = RTL_CONSTANT_STRING(L"\\DosDevices\\ScoutAC");

	IoDeleteSymbolicLink(&dosDeviceName);

	gRunning = FALSE;
	KeSetEvent(&gWakeEvent, 0, FALSE);
	if (gThreadHandle) {
		ZwWaitForSingleObject(gThreadHandle, FALSE, NULL);
		ZwClose(gThreadHandle);
	}

	ObCallbacks_Unregister(&g_ObCtx);
	ThreadCallbacks_Unregister();
	ProcessCallbacks_Unregister();
	ImageCallbacks_Unregister();
	ProcessList_Cleanup();

	if (DriverObject->DeviceObject != NULL) {
		IoDeleteDevice(DriverObject->DeviceObject);
	}


	KdPrint(("[ScoutAC] Driver unloaded!\n"));
};


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);
	ProcessList_Init();

	NTSTATUS status;

	UNICODE_STRING ntDeviceName = RTL_CONSTANT_STRING(L"\\Device\\ScoutAC");
	UNICODE_STRING dosDeviceName = RTL_CONSTANT_STRING(L"\\DosDevices\\ScoutAC");

	status = IoCreateDevice(
		DriverObject,
		0,
		&ntDeviceName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&pDeviceObject
	);

	if (!NT_SUCCESS(status)) {
		KdPrint(("[ScoutAC] Failed to create device object! Error: 0x%X\n", status));
		return status;
	}

	status = IoCreateSymbolicLink(&dosDeviceName, &ntDeviceName);

	if (!NT_SUCCESS(status)) {
		KdPrint(("[ScoutAC] Failed to create symbolic link! Error: 0x%X\n", status));
		IoDeleteDevice(pDeviceObject); 
		pDeviceObject = NULL;
		return status;
	}


	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MyIrpDeviceControlHandler;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchCreateClose;
	DriverObject->DriverUnload = DriverUnload;

	g_OriginalUnloadRoutine = DriverUnload;


	g_DriverBase = DriverObject->DriverStart;
	g_DriverSize = DriverObject->DriverSize;


	status = ProcessCallbacks_Register();

	if (!NT_SUCCESS(status)) {
		KdPrint(("[ScoutAC] Failed to register process callbacks! Error: 0x%X\n", status));

		IoDeleteSymbolicLink(&dosDeviceName);
		IoDeleteDevice(pDeviceObject);
		pDeviceObject = NULL;

		return status;

	}

	status = InitFltComms(DriverObject);

	if (!NT_SUCCESS(status)) {
		KdPrint(("[ScoutAC] Failed to init comms to UM service! Error: 0x%X\n", status));

		IoDeleteSymbolicLink(&dosDeviceName);
		IoDeleteDevice(pDeviceObject);
		pDeviceObject = NULL;

		return status;
	}


	status = ObCallbacks_Register(&g_ObCtx);

	if (!NT_SUCCESS(status)) {
		KdPrint(("[ScoutAC] Failed to register object callbacks! Error: 0x%X\n", status));

		ProcessCallbacks_Unregister();
		IoDeleteSymbolicLink(&dosDeviceName);
		IoDeleteDevice(pDeviceObject);
		pDeviceObject = NULL;

		return status;
		
	}

	status = ThreadCallbacks_Register();

	if (!NT_SUCCESS(status)) {
		KdPrint(("[ScoutAC] Failed to register thread callbacks! Error: 0x%X\n", status));

		ProcessCallbacks_Unregister();
		ObCallbacks_Unregister(&g_ObCtx);
		IoDeleteSymbolicLink(&dosDeviceName);
		IoDeleteDevice(pDeviceObject);
		pDeviceObject = NULL;

		return status;

	}

	status = ImageCallbacks_Register();

	if (!NT_SUCCESS(status)) {
		KdPrint(("[ScoutAC] Failed to register image callbacks! Error: 0x%X\n", status));

		ProcessCallbacks_Unregister();
		ObCallbacks_Unregister(&g_ObCtx);
		ThreadCallbacks_Unregister();
		IoDeleteSymbolicLink(&dosDeviceName);
		IoDeleteDevice(pDeviceObject);
		pDeviceObject = NULL;

		return status;

	}




	status = PsCreateSystemThread(&gThreadHandle, THREAD_ALL_ACCESS, NULL, NULL, NULL, ScannerThread, NULL);

	if (!NT_SUCCESS(status)) {
		KdPrint(("[ScoutAC] Failed to create watchdog thread! Error: 0x%X\n", status));

		ProcessCallbacks_Unregister();
		ObCallbacks_Unregister(&g_ObCtx);
		ThreadCallbacks_Unregister();
		ImageCallbacks_Unregister();
		IoDeleteSymbolicLink(&dosDeviceName);
		IoDeleteDevice(pDeviceObject);
		pDeviceObject = NULL;

		return status;

	}


	KdPrint(("[ScoutAC] Driver loaded!\n"));


	return STATUS_SUCCESS;
}
