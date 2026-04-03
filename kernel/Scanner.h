#pragma once

extern KTIMER gTimer;
extern KDPC gDpc;
extern HANDLE gThreadHandle;
extern volatile BOOLEAN gRunning;
extern KEVENT gWakeEvent;

VOID ScannerThread(PVOID Context);