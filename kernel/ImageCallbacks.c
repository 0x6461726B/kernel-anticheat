#include <ntifs.h>
#include "ImageCallbacks.h"

VOID OnLoadImage(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo) {
	UNREFERENCED_PARAMETER(FullImageName);
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(ImageInfo);


	//blacklist / whitelist  implement it later
}

NTSTATUS ImageCallbacks_Register(VOID) {
	return PsSetLoadImageNotifyRoutine(OnLoadImage);
}

VOID ImageCallbacks_Unregister(VOID) {
	PsRemoveLoadImageNotifyRoutine(OnLoadImage);
}