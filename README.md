# ScoutAC

A kernel-mode anti-cheat driver for Windows, written in C. More techniques will be added as time passes.

## What it does

ScoutAC monitors a protected process and looks for common cheat injection techniques using a combination of kernel callbacks and periodic scanning.

### Callbacks

- **`PsSetCreateProcessNotifyRoutineEx`** — tracks protected process lifetime and cleans up state on exit
- **`PsSetCreateThreadNotifyRoutine`** — on every new thread created inside the protected process, queries the Win32 start address via `ZwQueryInformationThread` and flags anything not backed by a mapped image (`MEM_IMAGE`)
- **`PsSetLoadImageNotifyRoutine`** — alerts on modules loading into the protected process below a configurable signature level, catching unsigned and developer-signed DLLs
- **`ObRegisterCallbacks`** — strips dangerous access rights (VM read/write, terminate, suspend/resume) from any handle opened to the protected process or its threads by an external process

### Periodic scan (system thread, 1s interval)

- **VAD tree walk** — walks the target process's virtual address descriptor tree looking for private executable regions, protection flips from non-executable to executable (indicating `VirtualProtect`-based injection), and anonymous mapped sections with execute permissions (indicating `NtMapViewOfSection`-based injection)
- **Kernel thread audit** — cross-references all system thread start addresses against `PsLoadedModuleList`, flagging threads not backed by any loaded driver
- **APC queue inspection** — walks the user-mode APC queue (`ApcListHead[1]`) for each thread in the protected process, flagging `NormalRoutine` pointers landing in private or anonymous executable memory

## Project structure

```
kernel/     — the driver
usermode/   — the IOCTL client (connects, registers itself as protected)
shared/     — IOCTL definitions shared between kernel and usermode
```

## Building

Requires the Windows Driver Kit (WDK) and Visual Studio with the kernel driver toolset installed. Open `kernel-anticheat.slnx` and build.

The post-build step copies the output to `Z:\defender\` if the drive is mapped — this is a VM network share used for testing. It will skip silently if the drive isn't present.

## Usage

Enable test signing and kernel debugging on the target machine:

```
bcdedit /set testsigning on
bcdedit /debug on
```

Load the driver, then run the usermode client as administrator. The client will connect to `\\.\ScoutAC`, register itself as the protected process.

Detection output goes to the kernel debugger via `KdPrint`. The driver is currently detect-only by design — no process termination or injection blocking, just observation and logging.

## Notes

All the hardcoded offsets are from Windows 11 25H2.