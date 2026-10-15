#include <iostream>
#include <windows.h>
#include <process.h>
#include <fltUser.h>
#include "shared.h"
#include <thread>


std::atomic<bool> g_Running = true;
HANDLE g_hShutdownEvent = nullptr;

void DetectionWorkerThread(HANDLE hDevice) {
    KERNEL_ALERT alert = { 0 };
    DWORD bytesReturned = 0;

    OVERLAPPED ov = { 0 };
    ov.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    HANDLE waitHandles[2] = { ov.hEvent, g_hShutdownEvent };

    while (g_Running.load()) {
        ResetEvent(ov.hEvent);
        ov.Internal = 0;
        ov.InternalHigh = 0;
        ov.Offset = 0;
        ov.OffsetHigh = 0;

        BOOL success = DeviceIoControl(
            hDevice,
            IOCTL_LISTEN_DETECTION,
            nullptr, 0,
            &alert, sizeof(alert),
            nullptr,
            &ov
        );

        if (!success && GetLastError() != ERROR_IO_PENDING) {
            break;
        }

        DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

        if (waitResult == WAIT_OBJECT_0) {
            // driver completed the IRP

            if (GetOverlappedResult(hDevice, &ov, &bytesReturned, FALSE)) {

                std::cout << "\n[!] --- SCOUT AC TELEMETRY ---" << std::endl;
                std::cout << "[*] Violation: " << alert.ViolationType << std::endl;
                std::wcout << L"[*] Details:   " << alert.Description << std::endl;
                std::cout << "------------------------------\n" << std::endl;
            }
        }
        else if (waitResult == WAIT_OBJECT_0 + 1) {
            std::cout << "[*] Receieved stop signal." << std::endl;
            CancelIoEx(hDevice, &ov);
            break;
        }
    }

    CloseHandle(ov.hEvent);
}



int main()
{
    LPCWSTR driverName = L"\\\\.\\ScoutAC";

    HANDLE hDevice = CreateFile(
        driverName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        nullptr);

    if (hDevice == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        std::cout << "Failed to connect to the driver. Error Code: " << error << std::endl;

        if (error == 2) std::cout << "Error 2: File Not Found (Is the driver actually running?)" << std::endl;
        if (error == 5) std::cout << "Error 5: Access Denied (Did you run this .exe as Administrator?)" << std::endl;

        system("pause");
        return 1;
    }


    std::cout << "SUCCESS! Connected to ScoutAC." << std::endl;

    std::cout << "Enter PID to protect: ";

    DWORD PID = 0;

    std::cin >> PID;

    auto processID = PID;//GetCurrentProcessId();

    DWORD bytesReturned;

    OVERLAPPED syncOv = { 0 };
    syncOv.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    BOOL success = DeviceIoControl(
        hDevice,
        IOCTL_PROTECT_PROCESS,
        &processID,
        sizeof(processID),
        nullptr, 0,
        &bytesReturned,
        &syncOv);

    if (!success && GetLastError() == ERROR_IO_PENDING) {
        WaitForSingleObject(syncOv.hEvent, INFINITE);
        success = GetOverlappedResult(hDevice, &syncOv, &bytesReturned, FALSE);
    }

    if (success) {
        std::cout << "Protected process: " << std::dec << processID << " successfully." << std::endl;
    } else {
        std::cout << "Failed to protect process. Error code: " << GetLastError() << std::endl;
    }

    g_hShutdownEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    std::thread worker(DetectionWorkerThread, hDevice);

    std::cout << "Client running, press any key to terminate." << std::endl;
    std::cin.get();
    std::cin.get();

    
    g_Running = false;
    SetEvent(g_hShutdownEvent);


    if (worker.joinable()) {
        worker.join();
    }

    CloseHandle(g_hShutdownEvent);
    CloseHandle(hDevice);

    return 0;
}

