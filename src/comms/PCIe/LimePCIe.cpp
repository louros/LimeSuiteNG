#include "comms/PCIe/LimePCIe.h"

#include "limesuiteng/Logger.h"

#include <iostream>
#include <cerrno>
#include <cstring>
#include <thread>
#include "protocols/LMS64CProtocol.h"

#ifdef __unix__
    #include <unistd.h>
    #include <fcntl.h>
    #include <poll.h>
    #include <sys/mman.h>
    #include <sys/ioctl.h>
    #include "linux-kernel-module/limepcie.h"
#endif

#ifdef __WIN32__
    #include <windows.h>
    #include <setupapi.h>
    #include <iostream>
    #include <vector>
    #include <string>
#endif

using namespace std;
using namespace lime;
using namespace std::literals::string_literals;

std::vector<std::string> LimePCIe::GetEndpointsWithPattern(const std::string& devicePath, const std::string& regex)
{
    std::vector<std::string> devices;
    
    return devices;
}

std::vector<std::string> LimePCIe::GetPCIeDeviceList()
{
    // GUID for LitePCIe device class, replace this with the actual LitePCIe GUID if needed.
    GUID LitePCIeGUID = { 164ADC02-E1AE-4FE1-A904-9A013577B891 };
    HDEVINFO deviceInfoSet;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData;
    DWORD requiredSize = 0;
    int deviceIndex = 0;
    std::vector<std::string> devicePaths; // Vector to hold device paths

    // Get device info set for all installed devices matching the LitePCIe GUID
    deviceInfoSet = SetupDiGetClassDevs(&LitePCIeGUID, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to get device info set" << std::endl;
        return devicePaths; // Return empty vector on failure
    }

    // Initialize the device interface data structure
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    // Enumerate all devices in the device info set
    while (SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &LitePCIeGUID, deviceIndex, &deviceInterfaceData)) {
        deviceIndex++;

        // Get the required buffer size for the device interface detail
        SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);
        deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
        deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        // Get the device interface detail
        if (SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, deviceInterfaceDetailData, requiredSize, NULL, NULL)) {
            // Store device path in the vector
            devicePaths.push_back(std::string(deviceInterfaceDetailData->DevicePath, deviceInterfaceDetailData->DevicePath + wcslen(deviceInterfaceDetailData->DevicePath)));
        } else {
            std::cerr << "Failed to get device interface detail" << std::endl;
        }

        free(deviceInterfaceDetailData);
    }

    if (GetLastError() != ERROR_NO_MORE_ITEMS) {
        std::cerr << "Error enumerating devices" << std::endl;
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return devicePaths; // Return the vector of device paths
}

LimePCIe::LimePCIe()
    : mFilePath()
    , mFileDescriptor(-1)
{
}

LimePCIe::~LimePCIe()
{
    Close();
}

OpStatus LimePCIe::RunControlCommand(uint8_t* request, uint8_t* response, size_t length, int timeout_ms)
{
#ifndef ENOIOCTLCMD
    constexpr int ENOIOCTLCMD = 515; // not a standard Posix error code, but exists in linux kernel headers
#endif
    
    return OpStatus::Error;
}

OpStatus LimePCIe::RunControlCommand(uint8_t* data, size_t length, int timeout_ms)
{
    return RunControlCommand(data, data, length, timeout_ms);
}

OpStatus LimePCIe::Open(const std::filesystem::path& deviceFilename, uint32_t flags)
{
    return OpStatus::Error;
}

bool LimePCIe::IsOpen() const
{
    return mFileDescriptor >= 0;
}

void LimePCIe::Close()
{
    
}

int LimePCIe::WriteControl(const uint8_t* buffer, const int length, int timeout_ms)
{
    return 0;
}

int LimePCIe::ReadControl(uint8_t* buffer, const int length, int timeout_ms)
{
   return 0;
}
