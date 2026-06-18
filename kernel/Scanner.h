#pragma once

extern HANDLE gThreadHandle;
extern volatile BOOLEAN gRunning;
extern KEVENT gWakeEvent;

VOID ScannerThread(PVOID Context);