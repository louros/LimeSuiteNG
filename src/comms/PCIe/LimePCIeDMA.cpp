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
    if (isInitialized)
        return OpStatus::Success;

    return OpStatus::Error;
}

LimePCIeDMA::~LimePCIeDMA()
{
    if (!port->IsOpen() || !isInitialized)
        return;

    Enable(false);

    if (mappings.empty())
        return;
}

OpStatus LimePCIeDMA::Enable(bool enabled)
{
    if (!isInitialized)
        return OpStatus::Error;
    return OpStatus::Error;
}

OpStatus LimePCIeDMA::EnableContinuous(bool enabled, uint32_t maxTransferSize, uint8_t irqPeriod)
{
    if (!isInitialized)
        return OpStatus::Error;
    assert(port->IsOpen());

    return OpStatus::Error;
}

IDMA::State LimePCIeDMA::GetCounters()
{
    IDMA::State dma{};
    if (!isInitialized)
        return dma;

    return dma;
}

OpStatus LimePCIeDMA::SubmitRequest(uint64_t index, uint32_t bytesCount, DataTransferDirection direction, bool generateIRQ)
{
    if (!isInitialized)
        return OpStatus::Error;
    assert(port->IsOpen());

    return OpStatus::Error;
}

OpStatus LimePCIeDMA::Wait()
{
    if (!isInitialized)
        return OpStatus::Error;
    assert(port->IsOpen());

    return OpStatus::Error;
}

void LimePCIeDMA::BufferOwnership(uint16_t index, DataTransferDirection bufferDirection)
{
    if (!isInitialized)
        return;

}

std::vector<IDMA::Buffer> LimePCIeDMA::GetBuffers() const
{
    return mappings;
}

std::string LimePCIeDMA::GetName() const
{
    return port->GetPathName();
}

} // namespace lime
