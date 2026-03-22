#include <iostream>
#include <windows.h>
#include "shared.h"

int main()
{
    LPCWSTR driverName = L"\\\\.\\LilAnticheat";

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

    std::cout << "IOCTL Code: " << std::hex << IOCTL_AC_ECHO_TEST << std::endl;

    std::cout << "SUCCESS! Connected to the Ring 0 Driver." << std::endl;


    DWORD bytesReturned = 0;
    BOOL success = DeviceIoControl(
        hDevice, 
        IOCTL_AC_ECHO_TEST, 
        nullptr, 
        0, 
        nullptr, 
        0, 
        &bytesReturned, 
        nullptr);
    
    if (success) {
        std::cout << "Echo IOCTL sent successfully" << std::endl;
    }
    else {
        std::cout << "IOCTL failed. Error code: " << GetLastError() << std::endl;
    }

    CloseHandle(hDevice);
    std::cout << "Connection Closed." << std::endl;

    system("pause");
    return 0;

}

