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


using namespace std;
using namespace lime;
using namespace std::literals::string_literals;

std::vector<std::string> LimePCIe::GetEndpointsWithPattern(const std::string& devicePath, const std::string& regex)
{
    std::vector<std::string> devices;
    
    return devices;
}

/* DNA Registers */
#define CSR_DNA_BASE (CSR_BASE + 0x1000L)
#define CSR_DNA_ID_ADDR (CSR_BASE + 0x1000L)
#define CSR_DNA_ID_SIZE 2

std::vector<std::string> LimePCIe::GetPCIeDeviceList()
{
    log(LogLevel::Info, "GetPCIeDeviceList");

    vector<std::string> handles;

    file_t fd;
    fd = litepcie_open("\\CTRL", FILE_ATTRIBUTE_NORMAL);
    if (fd == INVALID_HANDLE_VALUE)
    {
        log(LogLevel::Info, "Could not init driver\n");
        return handles;
    }

    unsigned char fpga_identifier[256]; // Filled with binary data

    for (int i = 0; i < 255; i++)
    {
        fpga_identifier[i] = litepcie_readl(fd, CSR_IDENTIFIER_MEM_BASE + 4 * i);
    }
    log(LogLevel::Info, "FPGA Identifier:  %s\n", fpga_identifier);

    std::string identifier_string(reinterpret_cast<const char*>(fpga_identifier));
    log(LogLevel::Info, "FPGA Identifier: %s\n", identifier_string.c_str());

    log(LogLevel::Info,
        "FPGA DNA:         0x%08x%08x\n",
        litepcie_readl(fd, CSR_DNA_ID_ADDR + 4 * 0),
        litepcie_readl(fd, CSR_DNA_ID_ADDR + 4 * 1));

    handles.push_back("\\CTRL");

    return handles;
}

LimePCIe::LimePCIe()
    : mFilePath()
    , mFileDescriptor(-1)
    , wfileDescriptor(NULL)
{
}

LimePCIe::~LimePCIe()
{
    Close();
}

#include <windows.h>
#include <stdexcept>
#include <wchar.h>

OpStatus LimePCIe::RunControlCommand(uint8_t* request, uint8_t* response, size_t length, int timeout_ms)
{
    file_t fd = litepcie_open("\\CTRL", FILE_ATTRIBUTE_NORMAL);

    if (fd == INVALID_HANDLE_VALUE)
    {
        // Handle error (e.g., log, return IOFailure)
        log(LogLevel::Info, "INVALID_HANDLE_VALUE");
        return OpStatus::IOFailure;
    }

    // Define the control packet structure
    struct litepcie_control_packet pkt;
    pkt.timeout_ms = timeout_ms;
    pkt.length = length;

    // Copy the request data into the packet
    memcpy(pkt.request, request, length);

    int resp = checked_ioctl(fd, LITEPCIE_IOCTL_RUN_CONTROL_COMMAND, pkt);

    // Check the result of the DeviceIoControl call
    if ((size_t) pkt.length != length)
        return OpStatus::IOFailure;

    // Copy the response data from the packet
    memcpy(response, pkt.response, length);

    return OpStatus::Success;
}


OpStatus LimePCIe::RunControlCommand(uint8_t* data, size_t length, int timeout_ms)
{
    return RunControlCommand(data, data, length, timeout_ms);
}

OpStatus LimePCIe::Open(const std::filesystem::path& deviceFilename, uint32_t flags)
{
    mFilePath = deviceFilename;
    wfileDescriptor = litepcie_open(deviceFilename.string().c_str(), FILE_ATTRIBUTE_NORMAL);
    if (wfileDescriptor == INVALID_HANDLE_VALUE)
    {
        log(LogLevel::Info, "Failed to open device %s", mFilePath.c_str());
        return OpStatus::FileNotFound;
    }

    return OpStatus::Success;
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
    file_t fd;
    fd = litepcie_open("\\DMA0", FILE_ATTRIBUTE_NORMAL);
    if (fd == INVALID_HANDLE_VALUE)
    {
        log(LogLevel::Info, "Could not init driver\n");
        return 0;
    }

    DWORD bytesWritten;
    BOOL result = WriteFile(fd,
        buffer,
        length,
        &bytesWritten,
        NULL
    );

    if (!result)
    {
        return -1;
    }

    return bytesWritten;
}

int LimePCIe::ReadControl(uint8_t* buffer, const int length, int timeout_ms)
{
    log(LogLevel::Info, "ReadControl\n");
    file_t fd;
    fd = litepcie_open("\\DMA0", FILE_ATTRIBUTE_NORMAL);
    if (fd == INVALID_HANDLE_VALUE)
    {
        log(LogLevel::Info, "Could not init driver\n");
        return 0;
    }

    // Initialize the buffer with zeroes
    memset(buffer, 0, length);
    uint32_t status = 0;
    DWORD bytesRead;
    auto t1 = std::chrono::high_resolution_clock::now();

    do
    {
        // Attempt to read status
        BOOL result = ReadFile(fd, // File handle
            &status, // Buffer to read status into
            sizeof(status), // Size of status buffer
            &bytesRead, // Number of bytes read
            NULL // No overlapped I/O
        );

        if (!result)
        {
            DWORD error = GetLastError();
            if (error != ERROR_IO_PENDING && error != ERROR_MORE_DATA)
            {
                break; // Error, not a retryable case
            }
        }

        // Check if the status byte has changed
        if ((status & 0xFF00) != 0)
        {
            break;
        }

        // Sleep for 10 microseconds
        std::this_thread::sleep_for(std::chrono::microseconds(10));

    } while (
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t1).count() < timeout_ms);

    // Check for timeout condition
    if ((status & 0xFF00) == 0)
    {
        ReportError(OpStatus::Timeout, "CMD %02X Read timeout", status & 0xFF);
    }

    // Attempt to read the actual buffer data
    BOOL finalRead = ReadFile(fd, // File handle
        buffer, // Buffer to read into
        length, // Length of data to read
        &bytesRead, // Number of bytes read
        NULL // No overlapped I/O
    );

    if (!finalRead)
    {
        return -1; // Error occurred during read
    }

    return bytesRead; // Return the number of bytes read
}
