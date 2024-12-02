#include "comms/PCIe/LimePCIeDMA.h"

#include <cassert>
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>

#include "comms/PCIe/LimePCIe.h"
#include "limesuiteng/Logger.h"
#ifdef __unix__
    #include <unistd.h>
    #include <fcntl.h>
    #include <poll.h>
    #include <sys/mman.h>
    #include <sys/ioctl.h>
    #include "linux-kernel-module/limepcie.h"
#endif

#include <windows.h>
#include <setupapi.h>
#include <iostream>
#include <vector>
#include <string>

#include <ioapiset.h>
#include "liblitepcie.h"
#include "csr.h"
#include "logger/LoggerInternal.h"
#include "litepcie.h"

using namespace std::literals::string_literals;

namespace lime {

LimePCIeDMA::LimePCIeDMA(std::shared_ptr<LimePCIe> port, DataTransferDirection dir)
    : port(port)
    , dir(dir)
    , isInitialized(false)
{
}

OpStatus LimePCIeDMA::Initialize()
{
    log(LogLevel::Info, "LimePCIeDMA::Initialize()");
    log(LogLevel::Info, "isInitialized %d" + isInitialized);

    if (isInitialized)
        return OpStatus::Success;

    if (!port->IsOpen())
        port->Open(port->GetPathName(), 0);

    struct litepcie_ioctl_mmap_dma_info info;
    int ret = checked_ioctl(port->wfileDescriptor, LITEPCIE_IOCTL_MMAP_DMA_INFO, info);

    log(LogLevel::Info, "LITEPCIE_IOCTL_MMAP_DMA_INFO %d" + ret);
    log(LogLevel::Info, "dma_rx_buf_offset %d" + info.dma_rx_buf_offset);
    log(LogLevel::Info, "dma_rx_buf_size %d" + info.dma_rx_buf_size);
    log(LogLevel::Info, "dma_rx_buf_count %d" + info.dma_rx_buf_count);

    if (ret != 0)
        return OpStatus::Error;

    mappings.reserve(info.dma_rx_buf_count);
    struct litepcie_ioctl_lock lockInfo;

    if (dir == DataTransferDirection::DeviceToHost)
    {
        log(LogLevel::Info, "DeviceToHost");
        memset(&lockInfo, 0, sizeof(lockInfo));
        lockInfo.dma_writer_request = 1;
        ret = checked_ioctl(port->wfileDescriptor, LITEPCIE_IOCTL_LOCK, lockInfo);
        if (ret != 0) //|| lockInfo.dma_writer_status == 0)
        {
            const std::string msg = ": DMA writer request denied"s;
            log(LogLevel::Info, msg);
            return OpStatus::PermissionDenied;
        }
        HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, // Use the system paging file
            NULL, // Default security
            PAGE_READONLY, // Read-only protection
            0, // Maximum object size (high-order DWORD)
            info.dma_rx_buf_size * info.dma_rx_buf_count, // Maximum object size (low-order DWORD)
            NULL); // Name of the mapping object

        if (hMapFile == NULL)
        {
            const std::string msg = ": failed to create file mapping for Rx DMA buffer";
            log(LogLevel::Info, msg);
            return OpStatus::Error;
        }

        auto buf = static_cast<uint8_t*>(MapViewOfFile(hMapFile, // Handle to the mapping object
            FILE_MAP_READ, // Read access
            0, // File offset (high-order DWORD)
            info.dma_rx_buf_offset, // File offset (low-order DWORD)
            info.dma_rx_buf_size * info.dma_rx_buf_count)); // Number of bytes to map

        if (buf == NULL)
        {
            const std::string msg = ": failed to map view of Rx DMA buffer";
            log(LogLevel::Info, msg);
            CloseHandle(hMapFile);
            return OpStatus::Error;
        }

        // Populate `mappings` similarly
        for (size_t i = 0; i < info.dma_rx_buf_count; ++i)
        {
            mappings.push_back({ buf + info.dma_rx_buf_size * i, info.dma_rx_buf_size });
        }

        // Clean up
        UnmapViewOfFile(buf);
        CloseHandle(hMapFile);
    }
    else
    {
        log(LogLevel::Info, "HostToDevice");
        memset(&lockInfo, 0, sizeof(lockInfo));
        lockInfo.dma_reader_request = 1;
        ret = checked_ioctl(port->wfileDescriptor, LITEPCIE_IOCTL_LOCK, lockInfo);
        if (ret != 0) // || lockInfo.dma_reader_status == 0)
        {
            const std::string msg = ": DMA reader request denied"s;
            log(LogLevel::Info, msg);
            return OpStatus::PermissionDenied;
        }
        HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, // Use system paging file
            NULL, // Default security
            PAGE_READWRITE, // Read-write protection
            0, // Maximum object size (high-order DWORD)
            info.dma_tx_buf_size * info.dma_tx_buf_count, // Maximum object size (low-order DWORD)
            NULL); // Name of the mapping object

        if (hMapFile == NULL)
        {
            const std::string msg = ": failed to create file mapping for Tx DMA buffer";
            log(LogLevel::Info, msg);
            return OpStatus::Error;
        }

        auto buf = static_cast<uint8_t*>(MapViewOfFile(hMapFile, // Handle to the mapping object
            FILE_MAP_WRITE, // Write access
            0, // File offset (high-order DWORD)
            info.dma_tx_buf_offset, // File offset (low-order DWORD)
            info.dma_tx_buf_size * info.dma_tx_buf_count)); // Number of bytes to map

        if (buf == NULL)
        {
            const std::string msg = ": failed to map view of Tx DMA buffer";
            log(LogLevel::Info, msg);
            CloseHandle(hMapFile);
            return OpStatus::Error;
        }

        // Populate `mappings` similarly
        for (size_t i = 0; i < info.dma_tx_buf_count; ++i)
        {
            mappings.push_back({ buf + info.dma_tx_buf_size * i, info.dma_tx_buf_size });
        }

        // Clean up
        UnmapViewOfFile(buf);
        CloseHandle(hMapFile);
    }
    isInitialized = true;
    return OpStatus::Success;
}

LimePCIeDMA::~LimePCIeDMA()
{
    if (!port->IsOpen() || !isInitialized)
    {
        return;
    }

    Enable(false);

    if (mappings.empty())
        return;

   // munmap(mappings.front().buffer, mappings.front().size * mappings.size());
    UnmapViewOfFile(mappings.front().buffer);

    litepcie_ioctl_lock lockInfo{ 0, 0, 0, 0, 0, 0 };
    if (dir == DataTransferDirection::DeviceToHost)
        lockInfo.dma_writer_release = 1;
    else
        lockInfo.dma_reader_release = 1;
    checked_ioctl(port->wfileDescriptor, LITEPCIE_IOCTL_LOCK, lockInfo);
}

OpStatus LimePCIeDMA::Enable(bool enabled)
{
    log(LogLevel::Debug, "LimePCIeDMA::Enable %d", enabled);

    if (!isInitialized)
    {
        return OpStatus::Error;
    }

    struct litepcie_ioctl_dma_control args;
    args.enabled = enabled;
    args.directionFromDevice = (dir == DataTransferDirection::DeviceToHost);
    log(LogLevel::Debug, "LimePCIeDMA::directionFromDevice %d", (dir == DataTransferDirection::DeviceToHost));
    int ret = checked_ioctl(port->wfileDescriptor, LITEPCIE_IOCTL_DMA_CONTROL, args);
    if (ret < 0)
        return ReportError(OpStatus::IOFailure, "Failed DMA Enable ioctl. errno(%i) %s", errno, strerror(errno));
    return OpStatus::Success;
}

OpStatus LimePCIeDMA::EnableContinuous(bool enabled, uint32_t maxTransferSize, uint8_t irqPeriod)
{
    log(LogLevel::Debug, "LimePCIeDMA::EnableContinuous %d", enabled);

    if (!isInitialized)
    {
        return OpStatus::Error;
    }

    //assert(port->IsOpen());

    struct litepcie_ioctl_dma_control_continuous args;
    args.control.enabled = enabled;
    args.control.directionFromDevice = (dir == DataTransferDirection::DeviceToHost);
    args.transferSize = maxTransferSize;
    args.irqPeriod = irqPeriod;

    log(LogLevel::Debug, "LimePCIeDMA::directionFromDevice %d", (dir == DataTransferDirection::DeviceToHost));
    int ret = checked_ioctl(port->wfileDescriptor, LITEPCIE_IOCTL_DMA_CONTROL_CONTINUOUS, args);
    log(LogLevel::Debug, "LimePCIeDMA::ret %d", ret);
    if (ret < 0)
    {
        switch (ret)
        {
        case EBUSY:
            return ReportError(OpStatus::Busy, "DMA is already enabled. errno(%i) %s", errno, strerror(errno));
        default:
            return ReportError(OpStatus::IOFailure, "Failed DMA Enable continuous ioctl. errno(%i) %s", errno, strerror(errno));
        }
    }
    return OpStatus::Success;
}

IDMA::State LimePCIeDMA::GetCounters()
{
    struct IDMA::State dma;
    if (!isInitialized)
    {
        return dma;
    }

    struct litepcie_ioctl_dma_status status;
    status.wait_for_read = false;
    status.wait_for_write = false;
    int ret = checked_ioctl(port->wfileDescriptor, LITEPCIE_IOCTL_DMA_STATUS, status);
    if (ret)
        throw std::runtime_error("TransmitLoop IOCTL failed to get DMA counters");
    dma.transfersCompleted = (dir == DataTransferDirection::DeviceToHost) ? status.fromDeviceCounter : status.toDeviceCounter;

    
    //log(LogLevel::Debug, "dma.transfersCompleted %d", dma.transfersCompleted);
    //log(LogLevel::Debug, "status.fromDeviceCounter %d", status.fromDeviceCounter);
    //log(LogLevel::Debug, "  ");


    return dma;
}

OpStatus LimePCIeDMA::SubmitRequest(uint64_t index, uint32_t bytesCount, DataTransferDirection direction, bool generateIRQ)
{
    log(LogLevel::Info, "SubmitRequest %d", index);

    if (!isInitialized)
    {
        return OpStatus::Error;
    }

    //assert(port->IsOpen());

    struct litepcie_ioctl_dma_request request;
    request.bufferIndex = index;
    request.transferSize = bytesCount;
    request.generateIRQ = generateIRQ;
    request.directionFromDevice = (direction == DataTransferDirection::DeviceToHost);

    int ret = checked_ioctl(port->wfileDescriptor, LITEPCIE_IOCTL_DMA_REQUEST, request);
    if (ret != 0)
        return OpStatus::Error;
    return OpStatus::Success;
}

OpStatus LimePCIeDMA::Wait()
{
    if (!isInitialized)
    {
        return OpStatus::Error;
    }

    //assert(port->IsOpen());

    struct litepcie_ioctl_dma_status status;
    status.wait_for_read = (dir == DataTransferDirection::DeviceToHost);
    status.wait_for_write = (dir == DataTransferDirection::HostToDevice);
    int ret = checked_ioctl(port->wfileDescriptor, LITEPCIE_IOCTL_DMA_STATUS, status);
    if (ret)
        throw std::runtime_error("TransmitLoop IOCTL failed to get DMA counters");

    return OpStatus::Success;
}

void LimePCIeDMA::BufferOwnership(uint16_t index, DataTransferDirection bufferDirection)
{
    if (!isInitialized)
    {
        return;
    }

    struct litepcie_cache_flush args;
    args.directionFromDevice = (dir == DataTransferDirection::DeviceToHost);
    args.sync_to_cpu = (bufferDirection == DataTransferDirection::DeviceToHost);
    args.bufferIndex = index;
    checked_ioctl(port->wfileDescriptor, LITEPCIE_IOCTL_CACHE_FLUSH, args);
}

std::vector<IDMA::Buffer> LimePCIeDMA::GetBuffers() const
{
    return mappings;
}

std::string LimePCIeDMA::GetName() const
{
    return port->GetPathName().string();
}

} // namespace lime
