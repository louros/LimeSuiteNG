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
#include "streaming/DataPacket.h"

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
    log(LogLevel::Info, "isInitialized %d" + (isInitialized == true));


    if (isInitialized)
        return OpStatus::Success;

    if (!port->IsOpen())
        port->Open(port->GetPathName(), 0);


    HANDLE dupHandle;
    if (!DuplicateHandle(GetCurrentProcess(),
            GetCurrentProcess(),
            GetCurrentProcess(),
            &dupHandle,
            PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION,
            FALSE,
            0))
    {
        printf("Failed to duplicate handle. Error: %d\n", GetLastError());
        abort();
        return OpStatus::Error;
    }


    struct litepcie_ioctl_mmap_dma_info info;
    info.processHandle = dupHandle;

    int ret = checked_ioctl(port->wfileDescriptor, LITEPCIE_IOCTL_MMAP_DMA_INFO, info);

    //log(LogLevel::Info, "LITEPCIE_IOCTL_MMAP_DMA_INFO %d" + ret);
    //log(LogLevel::Info, "dma_rx_buf_offset %llu" + info.dma_rx_buf_offset);
    //log(LogLevel::Info, "dma_rx_buf_size %llu" + info.dma_rx_buf_size);
  // log(LogLevel::Info, "dma_rx_buf_count %llu" + info.dma_rx_buf_count);

    if (ret != 0)
        return OpStatus::Error;

    mappings.reserve(info.dma_rx_buf_count);

    /* HANDLE hFileMapping;
    LPVOID pMappedMemory;

    // This handle should be provided by the kernel driver (via IOCTL, for example)
    // Replace with the actual handle or method to acquire the section mapping
    hFileMapping = OpenFileMapping(EVENT_ALL_ACCESS, FALSE, "Global\\LitePCIEMem");

    if (hFileMapping == NULL)
    {
        DWORD err = GetLastError();
        log(LogLevel::Info, "err: %d", err);
        abort();
        return OpStatus::Error;
    }

    // Map the memory into the application's address space
    auto buf = static_cast<uint8_t*>(MapViewOfFile(hFileMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, info.dma_rx_buf_size * info.dma_rx_buf_count));
    if (buf == NULL)
    {
        printf("MapViewOfFile failed with error: %d\n", GetLastError());
        CloseHandle(hFileMapping);
        abort();
        return OpStatus::Error;
    }

    // Populate `mappings` with buffer offsets
    for (size_t i = 0; i < info.dma_rx_buf_count; ++i)
    {
        mappings.push_back({ buf + info.dma_rx_buf_size * i, info.dma_rx_buf_size });
    }

    isInitialized = true;
    return OpStatus::Success;
    */
    
    // Assuming `userSpaceAddress` is retrieved via DeviceIoControl and points to the base address of the DMA buffer
    auto buf = reinterpret_cast<uint8_t*>(info.base_rx_address);

    if (buf == nullptr)
    {
        const std::string msg = ": failed to map DMA buffer address from driver";
        log(LogLevel::Info, msg);
        abort();
        return OpStatus::Error;
    }

    // Populate `mappings` with buffer offsets
    for (size_t i = 0; i < info.dma_rx_buf_count; ++i)
    {
        mappings.push_back({ buf + info.dma_rx_buf_size * i, info.dma_rx_buf_size });
    }

    const uint8_t* buffer{ mappings.at(0).buffer };

    const FPGA_RxDataPacket* pkt = reinterpret_cast<const FPGA_RxDataPacket*>(buffer);
    
    isInitialized = true;
    return OpStatus::Success;
    

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
            PAGE_READWRITE, // Read-only protection
            0, // Maximum object size (high-order DWORD)
            info.dma_rx_buf_size * info.dma_rx_buf_count, // Maximum object size (low-order DWORD)
            NULL); // Name of the mapping object

        if (hMapFile == NULL)
        {
            const std::string msg = ": failed to create file mapping for Rx DMA buffer";
            log(LogLevel::Info, msg);
            abort();
            return OpStatus::Error;
        }

        auto buf = static_cast<uint8_t*>(MapViewOfFile(hMapFile, // Handle to the mapping object
            FILE_MAP_WRITE, // Read access
            0, // File offset (high-order DWORD)
            info.dma_rx_buf_offset, // File offset (low-order DWORD)
            info.dma_rx_buf_size * info.dma_rx_buf_count)); // Number of bytes to map

        if (buf == NULL)
        {
            const std::string msg = ": failed to map view of Rx DMA buffer";
            log(LogLevel::Info, msg);
            CloseHandle(hMapFile);
            abort();
            return OpStatus::Error;
        }

        // Populate `mappings` similarly
        for (size_t i = 0; i < info.dma_rx_buf_count; ++i)
        {
            mappings.push_back({ buf + info.dma_rx_buf_size * i, info.dma_rx_buf_size });
        }

        /* for (size_t i = 0; i < mappings.size(); ++i)
        {
            FPGA_RxDataPacket* packet = reinterpret_cast<FPGA_RxDataPacket*>(mappings[i].buffer);
            packet->header0 = 0xAB; // Dummy header value
            packet->payloadSizeLSB = 0x10; // Dummy payload size LSB
            packet->payloadSizeMSB = 0x00; // Dummy payload size MSB
            packet->counter = static_cast<int64_t>(i * 768); // Dummy counter value

            // Optionally populate the data with some dummy values
            //std::fill(std::begin(packet->data), std::end(packet->data), 0xFF); // Filling with dummy data
        }*/

        // Clean up
        //UnmapViewOfFile(buf);
        //CloseHandle(hMapFile);
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

        // Populate `mappings` similarly
        for (size_t i = 0; i < info.dma_rx_buf_count; ++i)
        {
            mappings.push_back({ buf + info.dma_rx_buf_size * i, info.dma_rx_buf_size });
        }

        for (size_t i = 0; i < mappings.size(); ++i)
        {
            // Get the buffer address and size from the mapping
            uint8_t* buffer = mappings[i].buffer; // Address
            size_t bufferSize = mappings[i].size;

            // Calculate the number of packets per buffer
            size_t packetCount = bufferSize / sizeof(FPGA_RxDataPacket);

            for (size_t j = 0; j < packetCount; ++j)
            {
                FPGA_RxDataPacket* packet = reinterpret_cast<FPGA_RxDataPacket*>(buffer + j * sizeof(FPGA_RxDataPacket));
                packet->header0 = 0xAB; // Dummy header value
                packet->payloadSizeLSB = 0x10; // Dummy payload size LSB
                packet->payloadSizeMSB = 0x00; // Dummy payload size MSB
                packet->counter = static_cast<int64_t>(i * 1000 + j); // Dummy counter value

                // Optionally populate the data with some dummy values
                std::fill(std::begin(packet->data), std::end(packet->data), 0xFF); // Filling with dummy data
            }
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
    //log(LogLevel::Info, "SubmitRequest %d", index);

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
