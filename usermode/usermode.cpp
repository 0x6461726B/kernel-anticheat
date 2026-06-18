#include <iostream>
#include <windows.h>
#include <process.h>
#include <fltUser.h>
#include "shared.h"

#pragma comment(lib, "fltlib.lib")

typedef struct _MINIFILTER_MESSAGE {
    FILTER_MESSAGE_HEADER Header;
    KERNEL_ALERT AlertData;
} MINIFILTER_MESSAGE, * PMINIFILTER_MESSAGE;

unsigned __stdcall MinifilterListenerThread(void* lpParam)
{
    HANDLE hPort = INVALID_HANDLE_VALUE;

    HRESULT hr = FilterConnectCommunicationPort(L"\\ScoutACPort", 0, NULL, 0, NULL, &hPort);
    if (FAILED(hr)) {
        std::cout << "[-] Failed to connect to ScoutACPort: 0x" << std::hex << hr << std::endl;
        return 1;
    }

    std::cout << "[+] Connected to ScoutACPort. Awaiting kernel alerts..." << std::endl;

    MINIFILTER_MESSAGE IncomingPacket = { 0 };

    while (true) {
        hr = FilterGetMessage(hPort, &IncomingPacket.Header, sizeof(IncomingPacket), NULL);

        if (SUCCEEDED(hr)) {
            KERNEL_ALERT* alert = &IncomingPacket.AlertData;

            std::cout << "\n[!] --- SCOUT AC TELEMETRY ---" << std::endl;
            std::cout << "[*] Violation: " << alert->ViolationType << std::endl;
            std::wcout << L"[*] Details:   " << alert->Description << std::endl;
            std::cout << "------------------------------\n" << std::endl;
        }
        else {
            if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)) {
                std::cout << "[-] Communication port closed by kernel component." << std::endl;
            }
            else {
                std::cout << "[-] FilterGetMessage error: 0x" << std::hex << hr << std::endl;
            }
            break;
        }
    }

    CloseHandle(hPort);
    return 0;
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
        FILE_ATTRIBUTE_NORMAL,
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

    auto PID = 0;

    std::cin >> PID;

    auto processID = PID;//GetCurrentProcessId();

    DWORD bytesReturned;

    NTSTATUS success = DeviceIoControl(
        hDevice,
        IOCTL_PROTECT_PROCESS,
        &processID,
        sizeof(processID),
        nullptr,
        0,
        &bytesReturned,
        nullptr);

    if (success) {
        std::cout << "Protected process: " << std::dec << processID << " successfully." << std::endl;
    }
    else {
        std::cout << "Failed to protect process. Error code: " << GetLastError() << std::endl;
    }
    
    HANDLE hThread = (HANDLE)_beginthreadex(
        nullptr,
        0,
        MinifilterListenerThread,
        nullptr,
        0,
        nullptr
    );
    if (hThread == NULL) {
        std::cout << "[-] Failed to create MinifilterListenerThread!" << std::endl;
    }
    else {
        std::cout << "[+] MinifilterListenerThread active in background." << std::endl;
    }

    while (true) {

        if (GetAsyncKeyState(VK_END) & 1)
            break;

        SleepEx(50, FALSE);

    }

    CloseHandle(hDevice);
    std::cout << "Connection Closed." << std::endl;

    if (hThread != NULL) {
        CloseHandle(hThread);
    }

    system("pause");
    return 0;

}

