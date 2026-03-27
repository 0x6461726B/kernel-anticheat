#include <ntifs.h>

#include "shared.h"
#include "ObCallbacks.h"
#include "ProcessList.h"

OB_PREOP_CALLBACK_STATUS PreObjectCallback(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION OperationInformation) {
	UNREFERENCED_PARAMETER(RegistrationContext);

	if (OperationInformation->KernelHandle)
		return OB_PREOP_SUCCESS;

	PEPROCESS targetProcess = NULL;
	PEPROCESS sourceProcess = PsGetCurrentProcess();

	if (OperationInformation->ObjectType == *PsThreadType) {
		targetProcess = PsGetThreadProcess((PETHREAD)OperationInformation->Object);
	}
	else {
		targetProcess = (PEPROCESS)OperationInformation->Object;
	}

	if (sourceProcess == targetProcess || sourceProcess == PsInitialSystemProcess)
		return OB_PREOP_SUCCESS;

	if (!ProcessList_IsProtected(targetProcess))
		return OB_PREOP_SUCCESS;

	PACCESS_MASK target;

	if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE) {
		target = &OperationInformation->Parameters->CreateHandleInformation.DesiredAccess;
	}
	else {
		target = &OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess;
	}

	char* targetName = (char*)((PUCHAR)targetProcess + 0x338); // win11 25h2 ImageFileName
	char* sourceName = (char*)((PUCHAR)sourceProcess + 0x338);

	ACCESS_MASK access = *target;

	if (OperationInformation->ObjectType == *PsProcessType) {
		if (access & (PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_TERMINATE | PROCESS_VM_OPERATION | PROCESS_SUSPEND_RESUME | PROCESS_SET_INFORMATION)) {
			*target &= ~PROCESS_ALL_ACCESS;
			*target |= PROCESS_QUERY_LIMITED_INFORMATION;


			KdPrint(("[ScoutAC] Process [%s] tried to access [%s] - PERMS STRIPPED\n", sourceName, targetName));
		}
	}
	else if (OperationInformation->ObjectType == *PsThreadType) {
		if (access & (THREAD_TERMINATE | THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT)) {
			*target &= ~THREAD_ALL_ACCESS;
			*target |= THREAD_QUERY_LIMITED_INFORMATION;

			KdPrint(("[ScoutAC] Process [%s] tried to access [%s] - PERMS STRIPPED\n", sourceName, targetName));

		}
	}


	return OB_PREOP_SUCCESS;
}

NTSTATUS ObCallbacks_Register(OB_CALLBACK_CONTEXT* ctx) {
	OB_OPERATION_REGISTRATION opReg[2] = {0};
	OB_CALLBACK_REGISTRATION cbReg = { 0 };
	UNICODE_STRING altitude;

	RtlInitUnicodeString(&altitude, L"321000");

	opReg[0].ObjectType = PsProcessType;
	opReg[0].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	opReg[0].PreOperation = PreObjectCallback;

	opReg[1].ObjectType = PsThreadType;
	opReg[1].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	opReg[1].PreOperation = PreObjectCallback;

	cbReg.Version = OB_FLT_REGISTRATION_VERSION;
	cbReg.OperationRegistrationCount = 2;
	cbReg.Altitude = altitude;
	cbReg.OperationRegistration = opReg;

	return ObRegisterCallbacks(&cbReg, &ctx->RegistrationHandle);

}

VOID ObCallbacks_Unregister(OB_CALLBACK_CONTEXT* ctx) {
	if (ctx->RegistrationHandle != NULL) {
		ObUnRegisterCallbacks(ctx->RegistrationHandle);
		ctx->RegistrationHandle = NULL;
	}
}
