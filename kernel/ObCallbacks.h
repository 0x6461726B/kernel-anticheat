#pragma once

typedef struct _OB_CALLBACK_CONTEXT {
	PVOID RegistrationHandle;
} OB_CALLBACK_CONTEXT;

NTSTATUS ObCallbacks_Register(OB_CALLBACK_CONTEXT* ctx);
VOID ObCallbacks_Unregister(OB_CALLBACK_CONTEXT* ctx);