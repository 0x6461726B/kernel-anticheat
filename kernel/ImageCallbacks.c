#include <ntifs.h>
#include "ImageCallbacks.h"
#include "ProcessList.h"
#include "globals.h"

VOID OnLoadImage(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo) {
	UNREFERENCED_PARAMETER(FullImageName);

	if (!ProcessId) return;

	if (!ProcessList_IsProtectedPid(ProcessId))
		return;

	if (ImageInfo->ImageSignatureLevel <= SE_SIGNING_LEVEL_DEVELOPER) {
		SendAlertToUserMode(AC_VIOLATION_IMAGE_UNSIGNED, L"Unsigned image loaded into protected process.");
		KdPrint(("Suspicious unsigned module loaded: %wZ with signature level: %u\n", FullImageName, ImageInfo->ImageSignatureLevel));
	}
	
	
}

NTSTATUS ImageCallbacks_Register(VOID) {
	return PsSetLoadImageNotifyRoutine(OnLoadImage);
}

VOID ImageCallbacks_Unregister(VOID) {
	PsRemoveLoadImageNotifyRoutine(OnLoadImage);
}