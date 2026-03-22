#include <ntddk.h>
#include <wdf.h>
#include "shared.h"

extern NTSTATUS SeLocateProcessImageName(
	_In_ PEPROCESS Process,
	_Outptr_ PUNICODE_STRING* ImageFileName
);

//callbacks

OB_PREOP_CALLBACK_STATUS PreObjectCallback(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION OperationInformation) {
	UNREFERENCED_PARAMETER(RegistrationContext);


	PEPROCESS targetProcess = (PEPROCESS)OperationInformation->Object;
	PEPROCESS sourceProcess = PsGetCurrentProcess();

	char* processName = (char*)((PUCHAR)targetProcess + 0x338);
	//char* sourceName = (char*)((PUCHAR)sourceProcess + 0x338);


	auto access = OperationInformation->Parameters->CreateHandleInformation.DesiredAccess;

	if (access & (PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_TERMINATE)) {

		if (strcmp(processName, "usermode.exe") == 0) {

			PUNICODE_STRING sourceName2 = NULL;
			if (NT_SUCCESS(SeLocateProcessImageName(sourceProcess, &sourceName2))) {

				KdPrint(("Proces [%wZ] tried to acces [usermode.exe] - PERMS STRIPPED\n", sourceName2));
				ExFreePoolWithTag(sourceName2, 0);
			}

			OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_ALL_ACCESS;
			OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_VM_READ;
			OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_VM_WRITE;
			OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_VM_OPERATION;


			OperationInformation->Parameters->CreateHandleInformation.DesiredAccess |= PROCESS_QUERY_LIMITED_INFORMATION;


		}
	}

	return OB_PREOP_SUCCESS;
}

PVOID RegistrationHandle = NULL;

NTSTATUS RegisterCallbacks() {
	OB_OPERATION_REGISTRATION opReg;
	OB_CALLBACK_REGISTRATION cbReg;
	UNICODE_STRING altitude;

	RtlInitUnicodeString(&altitude, L"321000");

	memset(&opReg, 0, sizeof(opReg));
	opReg.ObjectType = PsProcessType;
	opReg.Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	opReg.PreOperation = PreObjectCallback;
	opReg.PostOperation = NULL;

	memset(&cbReg, 0, sizeof(cbReg));
	cbReg.Version = OB_FLT_REGISTRATION_VERSION;
	cbReg.OperationRegistrationCount = 1;
	cbReg.Altitude = altitude;
	cbReg.RegistrationContext = NULL;
	cbReg.OperationRegistration = &opReg;

	return ObRegisterCallbacks(&cbReg, &RegistrationHandle);

}

void UnregisterCallbacks() {
	if (RegistrationHandle != NULL) {
		ObUnRegisterCallbacks(RegistrationHandle);
		RegistrationHandle = NULL;
	}
}

PDEVICE_OBJECT pDeviceObject = NULL;

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
	case IOCTL_AC_ECHO_TEST:
		KdPrint(("Hello from WDM kernel IOCTL!\n"));
		status = STATUS_SUCCESS;
		bytesReturned = 0;
		break;

	default:
		KdPrint(("Unknown IOCTL: 0x%X\n", controlCode));
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = bytesReturned;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}


VOID DriverUnload(PDRIVER_OBJECT DriverObject) {
	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, L"\\DosDevices\\LilAnticheat");

	IoDeleteSymbolicLink(&dosDeviceName);

	UnregisterCallbacks();

	if (DriverObject->DeviceObject != NULL) {
		IoDeleteDevice(pDeviceObject);
	}


	KdPrint(("Driver unloaded!\n"));
};


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);
	NTSTATUS status;

	UNICODE_STRING ntDeviceName;
	RtlInitUnicodeString(&ntDeviceName, L"\\Device\\Lilanticheat");

	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, L"\\DosDevices\\Lilanticheat");

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


	status = RegisterCallbacks();

	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to register callbacks! Error: 0x%X\n", status));

		IoDeleteSymbolicLink(&dosDeviceName);
		IoDeleteDevice(pDeviceObject);
		pDeviceObject = NULL;

		return status;
		
	}

	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MyIrpDeviceControlHandler;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchCreateClose;
	DriverObject->DriverUnload = DriverUnload;


	KdPrint(("Driver loaded!\n"));


	return STATUS_SUCCESS;
}