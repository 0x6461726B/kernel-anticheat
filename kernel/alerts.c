#pragma once
#include "alerts.h"
#include <shared.h>

PFLT_FILTER g_FilterHandle = NULL;
PFLT_PORT   g_ServerPort = NULL;
PFLT_PORT   g_ClientPort = NULL;

NTSTATUS FilterUnloadCallback(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

NTSTATUS ConnectNotifyCallback(
	_In_ PFLT_PORT Port,
	_In_opt_ PVOID ServerPortCookie,
	_In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
	_In_ ULONG SizeOfContext,
	_Outptr_result_maybenull_ PVOID* ConnectionPortCookie
);

VOID DisconnectNotifyCallback(
	_In_opt_ PVOID ConnectionCookie
);

NTSTATUS MessageNotifyCallback(
	_In_opt_ PVOID PortCookie,
	_In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
	_In_ ULONG InputBufferLength,
	_Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer,
	_In_ ULONG OutputBufferLength,
	_Out_ PULONG ReturnOutputBufferLength
);

const FLT_CONTEXT_REGISTRATION Contexts[] = {
	{ FLT_CONTEXT_END }
};


const FLT_REGISTRATION FilterRegistration = {
	sizeof(FLT_REGISTRATION),           // Size of this structure
	FLT_REGISTRATION_VERSION,           // Standard framework version
	0,                                  // Flags
	Contexts,                           // Context registration array
	NULL,                               // Operation callbacks array

	FilterUnloadCallback,               // FilterUnloadCallback

	NULL,                               // InstanceSetupCallback (Set to NULL)
	NULL,                               // InstanceQueryTeardownCallback (Set to NULL)
	NULL,                               // InstanceTeardownStartCallback (Set to NULL)
	NULL                                // InstanceTeardownCompleteCallback (Set to NULL)
};

NTSTATUS FilterUnloadCallback(_In_ FLT_FILTER_UNLOAD_FLAGS Flags) {
	UNREFERENCED_PARAMETER(Flags);

	if (g_ServerPort != NULL) {
		FltCloseCommunicationPort(g_ServerPort);
		g_ServerPort = NULL;
	}
	if (g_FilterHandle != NULL) {
		FltUnregisterFilter(g_FilterHandle);
		g_FilterHandle = NULL;
	}

	return STATUS_SUCCESS;
}

NTSTATUS ConnectNotifyCallback(_In_ PFLT_PORT Port, _In_opt_ PVOID ServerPortCookie, _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext, _In_ ULONG SizeOfContext, _Outptr_result_maybenull_ PVOID* ConnectionPortCookie) {
	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionPortCookie);
	*ConnectionPortCookie = NULL;

	g_ClientPort = Port;
	KdPrint(("[ScoutAC] Successfully estabilished communication with UM service.\n"));
	return STATUS_SUCCESS;


}

VOID DisconnectNotifyCallback(_In_opt_ PVOID ConnectionCookie) {
	UNREFERENCED_PARAMETER(ConnectionCookie);

	FltCloseClientPort(g_FilterHandle, &g_ClientPort);
	g_ClientPort = NULL;
	KdPrint(("[ScoutAC] Disconnected from UM service.\n"));
}

NTSTATUS MessageNotifyCallback(_In_opt_ PVOID PortCookie, _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer, _In_ ULONG InputBufferLength, _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer, _In_ ULONG OutputBufferLength, _Out_ PULONG ReturnOutputBufferLength) {
	UNREFERENCED_PARAMETER(PortCookie);
	UNREFERENCED_PARAMETER(InputBuffer);
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferLength);

	if (ReturnOutputBufferLength) *ReturnOutputBufferLength = 0;
	return STATUS_SUCCESS;
}


VOID SendAlertToUserMode(ULONG ViolationId, PCWSTR Details)
{
	// Safety check: If the user-mode app isn't running or connected, 
	// ClientPort will be NULL. Skip sending to prevent memory timeouts.
	if (g_ClientPort == NULL || g_FilterHandle == NULL) {
		return;
	}

	KERNEL_ALERT Alert = { 0 };
	Alert.ViolationType = ViolationId;

	if (Details != NULL) {
		wcscpy_s(Alert.Description, 128, Details);
	}

	// Define a timeout for the send operation (e.g., 500ms)
	// Negative value specifies relative time in 100-nanosecond units
	LARGE_INTEGER Timeout;
	Timeout.QuadPart = -5000000LL;

	// We pass NULL for the output buffer parameters because we are strictly 
	// broadcasting outbound alerts and do not expect a synchronous response.
	NTSTATUS Status = FltSendMessage(
		g_FilterHandle,
		&g_ClientPort,
		&Alert,
		sizeof(Alert),
		NULL,           // No reply buffer
		NULL,           // Reply buffer size
		&Timeout
	);

	if (!NT_SUCCESS(Status)) {
		KdPrint(("[ScoutAC] Failed to stream alert over port: 0x%X\n", Status));
	}
}



NTSTATUS InitFltComms(PDRIVER_OBJECT driverObject) {
	PSECURITY_DESCRIPTOR securityDescriptor = NULL;
	OBJECT_ATTRIBUTES objAttr;
	UNICODE_STRING portName = RTL_CONSTANT_STRING(L"\\ScoutACPort");



	NTSTATUS status = FltRegisterFilter(driverObject, &FilterRegistration, &g_FilterHandle);

	if (!NT_SUCCESS(status)) {
		KdPrint(("[ScoutAC] FltRegisterFilter failed with status: 0x%X\n", status));
		return status;
	}

	status = FltBuildDefaultSecurityDescriptor(
		&securityDescriptor,
		FLT_PORT_ALL_ACCESS
	);
	if (!NT_SUCCESS(status)) {
		KdPrint(("[ScoutAC] Failed to build security descriptor: 0x%X\n", status));
		FltUnregisterFilter(g_FilterHandle);
		return status;
	}

	InitializeObjectAttributes(&objAttr, &portName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, securityDescriptor);

	status = FltCreateCommunicationPort(
		g_FilterHandle,
		&g_ServerPort,
		&objAttr,
		NULL,
		ConnectNotifyCallback,
		DisconnectNotifyCallback,
		MessageNotifyCallback,
		1
	);

	if (securityDescriptor != NULL) {
		FltFreeSecurityDescriptor(securityDescriptor);
	}

	if (NT_SUCCESS(status)) {
		FltStartFiltering(g_FilterHandle);
	}
	
	if (!NT_SUCCESS(status)) {
		if (g_ServerPort != NULL) {
			FltCloseCommunicationPort(g_ServerPort);
			g_ServerPort = NULL;
		}
		if (g_FilterHandle != NULL) {
			FltUnregisterFilter(g_FilterHandle);
			g_FilterHandle = NULL;
		}
	}

	return status;
}