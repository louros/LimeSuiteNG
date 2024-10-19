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

std::vector<std::string> LimePCIe::GetPCIeDeviceList()
{
    std::vector<std::string> devices;
    
    return devices;
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
