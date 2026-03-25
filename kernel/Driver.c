#include <ntddk.h>

#include "shared.h"
#include "ObCallbacks.h"
#include "ProcessList.h"

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

	KdPrint(("IOCTL Received: 0x%X\n", controlCode));

	switch (controlCode) {
	case IOCTL_AC_ECHO_TEST: {
		KdPrint(("Hello from ScoutAC!\n"));
		status = STATUS_SUCCESS;
		bytesReturned = 0;
		break;
	}
	case IOCTL_PROTECT_PROCESS: {
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		ULONG pid = *(ULONG*)Irp->AssociatedIrp.SystemBuffer;
		status = ProcessList_Add((HANDLE)pid);
		KdPrint(("Protected process 0x%X", pid));
		break;
	}

	default: {
		KdPrint(("Unknown IOCTL: 0x%X\n", controlCode));
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
	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, L"\\DosDevices\\ScoutAC");

	IoDeleteSymbolicLink(&dosDeviceName);

	ObCallbacks_Unregister(&g_ObCtx);
	ProcessList_Cleanup();

	if (DriverObject->DeviceObject != NULL) {
		IoDeleteDevice(pDeviceObject);
	}


	KdPrint(("Driver unloaded!\n"));
};


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);
	ProcessList_Init();

	NTSTATUS status;

	UNICODE_STRING ntDeviceName;
	RtlInitUnicodeString(&ntDeviceName, L"\\Device\\ScoutAC");

	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, L"\\DosDevices\\ScoutAC");

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
		KdPrint(("Failed to create device object! Error: 0x%X\n", status));
		return status;
	}

	status = IoCreateSymbolicLink(&dosDeviceName, &ntDeviceName);

	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create symbolic link! Error: 0x%X\n", status));
		IoDeleteDevice(pDeviceObject); 
		pDeviceObject = NULL;
		return status;
	}


	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MyIrpDeviceControlHandler;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchCreateClose;
	DriverObject->DriverUnload = DriverUnload;


	status = ObCallbacks_Register(&g_ObCtx);

	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to register callbacks! Error: 0x%X\n", status));

		IoDeleteSymbolicLink(&dosDeviceName);
		IoDeleteDevice(pDeviceObject);
		pDeviceObject = NULL;

		return status;
		
	}




	KdPrint(("Driver loaded!\n"));


	return STATUS_SUCCESS;
}