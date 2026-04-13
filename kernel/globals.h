#pragma once
#include <ntifs.h>

typedef PVOID(*MiGetPteAdress_t)(ULONG64 va);
extern MiGetPteAdress_t MiGetPteAddress;