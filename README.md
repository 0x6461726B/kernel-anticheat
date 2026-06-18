# ScoutAC

A kernel-mode anti-cheat driver for Windows, written in C. More techniques will be added as time passes.

## What it does

ScoutAC monitors a protected process and looks for common cheat injection techniques using a combination of kernel callbacks and periodic scanning.

### Callbacks

- **Process Lifetime Tracking (`PsSetCreateProcessNotifyRoutineEx`)**: Registers a process creation/destruction callback to dynamically track when the protected process spawns or exits. It handles internal memory cleanup and securely resets driver unload states on target termination.
- **Thread Context Validation (`PsSetCreateThreadNotifyRoutine`)**: Registers a thread tracking routine that intercepts every newly created thread inside the target process. It queries the thread's Win32 start address via `ZwQueryInformationThread` and evaluates the memory allocation type to verify it is backed by a valid, mapped module image (`MEM_IMAGE`), successfully identifying unbacked execution lines.
- **Module Load Auditing (`PsSetLoadImageNotifyRoutine`)**: Monitors libraries loading into the process context. It references image signature levels to flag and log modules that fall below safe code-integrity tiers (such as unsigned or test-signed binaries).
- **Access Mask Stripping (`ObRegisterCallbacks`)**: Sets up a pre-operation handle filter targeting process and thread object creation/duplication requests. When external actors attempt to open handles to the game, it strictly strips invasive permissions (like `PROCESS_VM_READ`, `PROCESS_VM_WRITE`, or `PROCESS_TERMINATE`) down to restricted query-only masks.

### 2. Periodic Memory Scan
- **VAD Tree Audit**: Spawns a background thread to walk the target process's Virtual Address Descriptor (`MMVAD`) binary search tree to identify memory irregularities:
  - Searches for private allocations erroneously marked with executable flags (RWX memory space).
  - Evaluates individual Page Table Entries (PTEs) to catch **PTE Flips**, where page flags are hidden/changed to executable while the higher-level VAD mapping declares it non-executable.
  - Flags anomalous section-mapped memory regions operating without a valid underlying `FILE_OBJECT` backing them on disk.
- **Cross-Process Attachment Audit**: Periodically loops through system threads to evaluate their thread attachment states. It flags threads that have attached themselves to the target process's virtual address space via `KeStackAttachProcess` mechanics (auditing the internal `ApcStateIndex`).
- **Kernel Thread Verification**: Scans running system-space threads against the global `PsLoadedModuleList` to ensure their start vectors are located inside officially registered drivers, detecting manual mapped drivers.
- **Asynchronous Procedure Call (APC) Inspection**: Examines the user-mode APC dispatch queues (`ApcListHead[1]`) for active threads inside the protected context. It flags `NormalRoutine` target addresses pointing toward unsigned, private, or anonymous executable spaces.

## Project structure

```
kernel/     — the driver
usermode/   — the client app
shared/     — shared headers, IOCTL codes, and alert definitions
```

## Building

Requires the Windows Driver Kit (WDK) and Visual Studio with the kernel driver toolset installed. Open `kernel-anticheat.slnx` and build.

The post-build step copies the output to `Z:\defender\` if the drive is mapped — this is a VM network share used for testing. It will skip silently if the drive isn't present.

## Notes

All the hardcoded offsets are from Windows 11 25H2.