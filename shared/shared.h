#pragma once

#ifdef _KERNEL_MODE
	#include <ntddk.h>
#else
	#include <windows.h>
	#include <winioctl.h>
#endif

#define MY_DEVICE_TYPE FILE_DEVICE_UNKNOWN

#define IOCTL_PROTECT_PROCESS CTL_CODE(MY_DEVICE_TYPE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

// kernel defs
#ifdef _KERNEL_MODE
	#define PROCESS_TERMINATE         (0x0001)  
	#define PROCESS_CREATE_THREAD     (0x0002)  
	#define PROCESS_SET_SESSIONID     (0x0004)  
	#define PROCESS_VM_OPERATION      (0x0008)  
	#define PROCESS_VM_READ           (0x0010)  
	#define PROCESS_VM_WRITE          (0x0020)  
	#define PROCESS_DUP_HANDLE        (0x0040)  
	#define PROCESS_CREATE_PROCESS    (0x0080)  
	#define PROCESS_SET_QUOTA         (0x0100)  
	#define PROCESS_SET_INFORMATION   (0x0200)  
	#define PROCESS_QUERY_INFORMATION (0x0400)  
	#define PROCESS_SUSPEND_RESUME    (0x0800)  
	#define PROCESS_QUERY_LIMITED_INFORMATION (0x1000)  

	#define PROCESS_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFFF)
#endif

#ifdef _KERNEL_MODE
	#define MEM_IMAGE   0x1000000
	//#define MEM_MAPPED  0x40000
	//#define MEM_PRIVATE 0x20000
#endif

#pragma once

typedef struct _KERNEL_ALERT {
	ULONG ViolationType;
	WCHAR Description[128];
} KERNEL_ALERT, * PKERNEL_ALERT;


typedef unsigned long VIOLATION_ID;

// ============================================================================
// 1000 - 1999: CALLBACKS SUBSYSTEM (Folder: callbacks/)
// ============================================================================
#define AC_VIOLATION_THREAD_UNBACKED        1001  // ThreadCallbacks.c
#define AC_VIOLATION_IMAGE_UNSIGNED       1002  // ImageCallbacks.c

// ============================================================================
// 3000 - 3999: KERNEL DETECTIONS (Folder: detections/kernel/)
// ============================================================================
#define AC_VIOLATION_DRIVER_ATTACHED    3001  // AttachedDetection.c 
#define AC_VIOLATION_UNBACKED_DRIVER     3002  // DetectDrivers.c (e.g., mapped driver)

// ============================================================================
// 4000 - 4999: MEMORY SUBSYSTEM (Folder: memory/)
// ============================================================================
#define AC_VIOLATION_APC_INJECTION        4001  // memory/apc/ApcChecker.c
#define AC_VIOLATION_VAD_MANIPULATION     4002  // memory/vad/VadWalker.c (e.g., pte flips)