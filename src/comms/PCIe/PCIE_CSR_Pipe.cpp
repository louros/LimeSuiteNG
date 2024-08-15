#include "comms/PCIe/PCIE_CSR_Pipe.h"

using namespace lime;

PCIE_CSR_Pipe::PCIE_CSR_Pipe(std::shared_ptr<LimePCIe> port)
    : port(port)
{
}

int PCIE_CSR_Pipe::Write(const uint8_t* data, size_t length, int timeout_ms)
{
    return port->WriteControl(data, length, timeout_ms);
}
int PCIE_CSR_Pipe::Read(uint8_t* data, size_t length, int timeout_ms)
{
    return port->ReadControl(data, length, timeout_ms);
}
OpStatus PCIE_CSR_Pipe::RunControlCommand(uint8_t* data, size_t length, int timeout_ms)
{
    return port->RunControlCommand(data, length, timeout_ms);
}
OpStatus PCIE_CSR_Pipe::RunControlCommand(uint8_t* request, uint8_t* response, size_t length, int timeout_ms)
{
    return port->RunControlCommand(request, response, length, timeout_ms);
}