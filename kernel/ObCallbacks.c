#include <ntifs.h>

#include "shared.h"
#include "ObCallbacks.h"
#include "ProcessList.h"

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

	PACCESS_MASK target;

	if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE) {
		target = &OperationInformation->Parameters->CreateHandleInformation.DesiredAccess;
	}
	else {
		target = &OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess;
	}

	ACCESS_MASK access = *target;

	if (access & (PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_TERMINATE)) {

		if (ProcessList_IsProtected(targetProcess)){
			PUNICODE_STRING sourceName2 = NULL;
			if (NT_SUCCESS(SeLocateProcessImageName(sourceProcess, &sourceName2))) {

				KdPrint(("[ScoutAC] Process [%wZ] tried to acces [usermode.exe] - PERMS STRIPPED\n", sourceName2));
				ExFreePoolWithTag(sourceName2, 0);
			}
		
			*target &= ~PROCESS_ALL_ACCESS;
			*target |= PROCESS_QUERY_LIMITED_INFORMATION;

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
