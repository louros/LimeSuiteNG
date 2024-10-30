#include "DeviceFactoryPCIe.h"

#include <fcntl.h>

#include "limesuiteng/DeviceHandle.h"
#include "CommonFunctions.h"
#include "limesuiteng/Logger.h"
#include "comms/PCIe/LimePCIe.h"
#include "protocols/LMSBoards.h"
#include "comms/PCIe/LMS64C_FPGA_Over_PCIe.h"
#include "comms/PCIe/LMS64C_LMS7002M_Over_PCIe.h"
#include "MMX8/LMS64C_ADF_Over_PCIe_MMX8.h"
#include "MMX8/LMS64C_FPGA_Over_PCIe_MMX8.h"
#include "MMX8/LMS64C_LMS7002M_Over_PCIe_MMX8.h"

#include "boards/LimeSDR_XTRX/LimeSDR_XTRX.h"
#include "boards/LimeSDR_X3/LimeSDR_X3.h"
#include "boards/MMX8/MM_X8.h"
#include "boards/external/XSDR/XSDR.h"

#include <algorithm>
#include <windows.h>

#include <ioapiset.h>
#include "liblitepcie.h"
#include "csr.h"
#include "logger/LoggerInternal.h"

using namespace lime;
using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

void __loadDeviceFactoryPCIe(void) //TODO fixme replace with LoadLibrary/dlopen
{
    static DeviceFactoryPCIe limePCIeSupport; // self register on initialization
}

DeviceFactoryPCIe::DeviceFactoryPCIe()
    : DeviceRegistryEntry("LimePCIe"s)
{
}

std::vector<DeviceHandle> DeviceFactoryPCIe::enumerate(const DeviceHandle& hint)
{
    log(LogLevel::Info, "devicefactorypcie enumerate");

    /* file_t fd;
    fd = litepcie_open("\\CTRL", FILE_ATTRIBUTE_NORMAL);
    if (fd == INVALID_HANDLE_VALUE)
    {
        log(LogLevel::Info, "Could not init driver\n");
        exit(1);
    }
    */
    std::vector<DeviceHandle> handles;
    DeviceHandle handle;
    handle.media = "PCIe"s;

    if (!hint.media.empty() && hint.media != handle.media)
        return handles;

    // generate handles by probing devices
    std::vector<std::string> nodes = LimePCIe::GetPCIeDeviceList();
     for (const std::string& nodeName : nodes)
    {
        std::shared_ptr<LimePCIe> controlPort = std::make_shared<LimePCIe>();
        if (controlPort->Open("\\CTRL", FILE_ATTRIBUTE_NORMAL) != OpStatus::Success)
            continue;

        LMS64CProtocol::FirmwareInfo fw{};
        int subDeviceIndex = 0;
        auto controlPipe = std::make_shared<PCIE_CSR_Pipe>(controlPort);
        LMS64CProtocol::GetFirmwareInfo(*controlPipe, fw, subDeviceIndex);

        handle.name = GetDeviceName(static_cast<eLMS_DEV>(fw.deviceId));
        handle.serial = intToHex(fw.boardSerialNumber);

        // Add handle conditionally, filter by serial number
        if (handle.IsEqualIgnoringEmpty(hint))
            handles.push_back(handle);
    }
    return handles;
}

static bool ValidIdentifier(const char* identifier)
{
    if (strncmp("Lime", identifier, 4) == 0)
        return true;
    return false;

}

static bool ValidChar(char c)
{
    return isalnum(c) || c == '-' || c == '_' || c == ' ';
}

static void SanitizeIdentifier(const char* identifier, char* destination, const size_t dstSize)
{
    uint8_t size = 0;
    while (size < dstSize && ValidChar(identifier[size]))
        size++;

    while (size > 0 && identifier[size - 1] == ' ')
        size--;

    size = size <= dstSize ? size : dstSize;
    strncpy(destination, identifier, size);

    for (size_t i = 0; i < size; i++)
        if (destination[i] == ' ')
            destination[i] = '-';
}

SDRDevice* DeviceFactoryPCIe::make(const DeviceHandle& handle)
{
    file_t fd;
    fd = litepcie_open("\\CTRL", FILE_ATTRIBUTE_NORMAL);
    if (fd == INVALID_HANDLE_VALUE)
    {
        log(LogLevel::Info, "Could not init driver\n");
        exit(1);
    }

    /* uint32_t testValue = 0xC0DEAA55;

    litepcie_writel(fd, CSR_CNTRL_TEST_ADDR, testValue);
    uint32_t testReadback = litepcie_readl(fd, CSR_CNTRL_TEST_ADDR);
    if (testReadback != testValue)
    {
        log(LogLevel::Info, "Test register failed. Written (0x%08X), read (0x%08X).\n", testValue, testReadback);
    }
    else
    {
        log(LogLevel::Info, "Test register  success ");
    }*/

    char fpga_identifier[256];
    for (int i = 0; i < 256; i++)
        fpga_identifier[i] = litepcie_readl(fd, CSR_IDENTIFIER_MEM_BASE + i * 4);
    fpga_identifier[255] = '\0';

    if (fpga_identifier[0] != 0)
        log(LogLevel::Info, "Identifier: %s\n", fpga_identifier);
    
    /* if (ValidIdentifier(fpga_identifier))
        SanitizeIdentifier(fpga_identifier, myDevice->info.devName, sizeof(myDevice->info.devName));
    else if (ReadInfo(myDevice) != 0)
    {
        dev_err(&myDevice->pciContext->dev, "Failed to read device info from FPGA MCU\n");
        return -EIO;
    }*/


    /*
     struct device *sysDev = &myDevice->pciContext->dev;
    uint32_t testValue = 0xC0DEAA55;
    limepcie_writel(myDevice, CSR_CNTRL_TEST_ADDR, testValue);
    uint32_t testReadback = limepcie_readl(myDevice, CSR_CNTRL_TEST_ADDR);
    if (testReadback != testValue)
    {
        dev_err(sysDev, "Test register failed. Written (0x%08X), read (0x%08X).\n", testValue, testReadback);
        return -EIO;
    }
    return 0;*/


    // Data transmission layer
    /* std::shared_ptr<LimePCIe> controlPort = std::make_shared<LimePCIe>();
    std::vector<std::shared_ptr<LimePCIe>> streamPorts;

    std::string controlFile(handle.addr + "/control0");
    OpStatus connectionStatus = controlPort->Open(controlFile, O_RDWR);
    if (connectionStatus != OpStatus::Success)
    {
        lime::ReportError(connectionStatus, "Unable to connect to device using handle (%s)", handle.Serialize().c_str());
        return nullptr;
    }

    std::vector<std::string> streamEndpoints = LimePCIe::GetEndpointsWithPattern(handle.addr, "trx*"s);
    std::sort(
        streamEndpoints.begin(), streamEndpoints.end()); // TODO: Fix potential sorting problem if there would be trx1 and trx11
    for (const std::string& endpointPath : streamEndpoints)
    {
        streamPorts.push_back(std::make_shared<LimePCIe>());
        streamPorts.back()->SetPathName(endpointPath);
    }
    */

    std::shared_ptr<LimePCIe> controlPort = std::make_shared<LimePCIe>();

    // protocol layer
    auto route_lms7002m = std::make_shared<LMS64C_LMS7002M_Over_PCIe>(controlPort);
    auto route_fpga = std::make_shared<LMS64C_FPGA_Over_PCIe>(controlPort);

    LMS64CProtocol::FirmwareInfo fw{};
    int subDeviceIndex = 0;
    auto controlPipe = std::make_shared<PCIE_CSR_Pipe>(controlPort);
    LMS64CProtocol::GetFirmwareInfo(*controlPipe, fw, subDeviceIndex);

    log(LogLevel::Info, "fw.deviceId %d", fw.deviceId);

    switch (fw.deviceId)
    {
    case LMS_DEV_LIMESDR_XTRX:
        return new LimeSDR_XTRX(route_lms7002m, route_fpga, nullptr, controlPipe);
    default:
        lime::ReportError(OpStatus::InvalidValue, "Unrecognized device ID (%i)", fw.deviceId);
        return nullptr;
    }
}
