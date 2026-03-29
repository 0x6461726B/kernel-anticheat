#include <ntifs.h>
#include "ImageCallbacks.h"
#include "ProcessList.h"

VOID OnLoadImage(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo) {

	if (!ProcessId) return;

	if (!ProcessList_IsProtectedPid(ProcessId))
		return;

	if (ImageInfo->ImageSignatureLevel <= SE_SIGNING_LEVEL_DEVELOPER) {
		KdPrint(("Suspicious unsigned module loaded: %wZ with signature level: %u\n", FullImageName, ImageInfo->ImageSignatureLevel));
	}
	
	
}

NTSTATUS ImageCallbacks_Register(VOID) {
	return PsSetLoadImageNotifyRoutine(OnLoadImage);
}

VOID ImageCallbacks_Unregister(VOID) {
	PsRemoveLoadImageNotifyRoutine(OnLoadImage);
}