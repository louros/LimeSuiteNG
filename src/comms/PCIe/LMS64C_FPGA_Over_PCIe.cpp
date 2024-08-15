#include "LMS64C_FPGA_Over_PCIe.h"
#include "protocols/LMS64CProtocol.h"

using namespace lime;

LMS64C_FPGA_Over_PCIe::LMS64C_FPGA_Over_PCIe(std::shared_ptr<LimePCIe> dataPort)
    : pipe(dataPort)
{
}

OpStatus LMS64C_FPGA_Over_PCIe::SPI(const uint32_t* MOSI, uint32_t* MISO, uint32_t count)
{
    return SPI(0, MOSI, MISO, count);
}

OpStatus LMS64C_FPGA_Over_PCIe::SPI(uint32_t spiBusAddress, const uint32_t* MOSI, uint32_t* MISO, uint32_t count)
{
    return LMS64CProtocol::FPGA_SPI(pipe, MOSI, MISO, count);
}

OpStatus LMS64C_FPGA_Over_PCIe::CustomParameterWrite(const std::vector<CustomParameterIO>& parameters)
{
    return LMS64CProtocol::CustomParameterWrite(pipe, parameters);
}

OpStatus LMS64C_FPGA_Over_PCIe::CustomParameterRead(std::vector<CustomParameterIO>& parameters)
{
    return LMS64CProtocol::CustomParameterRead(pipe, parameters);
}

OpStatus LMS64C_FPGA_Over_PCIe::ProgramWrite(const char* data, size_t length, int prog_mode, int target, ProgressCallback callback)
{
    return LMS64CProtocol::ProgramWrite(
        pipe, data, length, prog_mode, static_cast<LMS64CProtocol::ProgramWriteTarget>(target), callback);
}

OpStatus LMS64C_FPGA_Over_PCIe::MemoryWrite(uint32_t address, const void* data, uint32_t dataLength)
{
    return LMS64CProtocol::MemoryWrite(pipe, address, data, dataLength);
}

OpStatus LMS64C_FPGA_Over_PCIe::MemoryRead(uint32_t address, void* data, uint32_t dataLength)
{
    return LMS64CProtocol::MemoryRead(pipe, address, data, dataLength);
}
