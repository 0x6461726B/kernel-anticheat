#include "ObCallbacks.h"
#include "shared.h"
#include "ProcessList.h"

extern NTSTATUS SeLocateProcessImageName(
	_In_ PEPROCESS Process,
	_Outptr_ PUNICODE_STRING* ImageFileName
);

OB_PREOP_CALLBACK_STATUS PreObjectCallback(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION OperationInformation) {
	UNREFERENCED_PARAMETER(RegistrationContext);

	if (OperationInformation->KernelHandle)
		return OB_PREOP_SUCCESS;

	PEPROCESS targetProcess = (PEPROCESS)OperationInformation->Object;
	PEPROCESS sourceProcess = PsGetCurrentProcess();

	if (sourceProcess == targetProcess || sourceProcess == PsInitialSystemProcess)
		return OB_PREOP_SUCCESS;


	//char* processName = (char*)((PUCHAR)targetProcess + 0x338);
	//char* sourceName = (char*)((PUCHAR)sourceProcess + 0x338);

	ACCESS_MASK access = OperationInformation->Parameters->CreateHandleInformation.DesiredAccess;

	if (access & (PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_TERMINATE)) {

		if (ProcessList_IsProtected(targetProcess)){
			PUNICODE_STRING sourceName2 = NULL;
			if (NT_SUCCESS(SeLocateProcessImageName(sourceProcess, &sourceName2))) {

				KdPrint(("Proces [%wZ] tried to acces [usermode.exe] - PERMS STRIPPED\n", sourceName2));
				ExFreePoolWithTag(sourceName2, 0);
			}

			OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_ALL_ACCESS;
			//OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_VM_READ;
			//OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_VM_WRITE;
			//OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_VM_OPERATION;


			OperationInformation->Parameters->CreateHandleInformation.DesiredAccess |= PROCESS_QUERY_LIMITED_INFORMATION;


		}
	}

	return OB_PREOP_SUCCESS;
}

NTSTATUS ObCallbacks_Register(OB_CALLBACK_CONTEXT* ctx) {
	OB_OPERATION_REGISTRATION opReg = { 0 };
	OB_CALLBACK_REGISTRATION cbReg = { 0 };
	UNICODE_STRING altitude;

	RtlInitUnicodeString(&altitude, L"321000");

	opReg.ObjectType = PsProcessType;
	opReg.Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	opReg.PreOperation = PreObjectCallback;

	cbReg.Version = OB_FLT_REGISTRATION_VERSION;
	cbReg.OperationRegistrationCount = 1;
	cbReg.Altitude = altitude;
	cbReg.OperationRegistration = &opReg;

	return ObRegisterCallbacks(&cbReg, &ctx->RegistrationHandle);

}

VOID ObCallbacks_Unregister(OB_CALLBACK_CONTEXT* ctx) {
	if (ctx->RegistrationHandle != NULL) {
		ObUnRegisterCallbacks(ctx->RegistrationHandle);
		ctx->RegistrationHandle = NULL;
	}
}
