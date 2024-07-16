#ifndef LIME_LMS64C_ADF_OVER_PCIE_MMX8_H
#define LIME_LMS64C_ADF_OVER_PCIE_MMX8_H

#include "comms/ISPI.h"
#include "LitePCIe.h"
#include "PCIE_CSR_Pipe.h"

#include <cstdint>
#include <memory>

namespace lime {

/** @brief A class for communicating with MMX8's subdevice's ADF4002 chips. */
class LMS64C_ADF_Over_PCIe_MMX8 : public ISPI
{
  public:
    /**
      @brief Constructs a new LMS64C_ADF_Over_PCIe_MMX8 object
      @param dataPort The PCIe data bus to use.
      @param subdeviceIndex The subdevice index for which this class is created.
     */
    LMS64C_ADF_Over_PCIe_MMX8(std::shared_ptr<LitePCIe> dataPort, uint32_t subdeviceIndex);

    OpStatus SPI(const uint32_t* MOSI, uint32_t* MISO, uint32_t count) override;
    OpStatus SPI(uint32_t spiBusAddress, const uint32_t* MOSI, uint32_t* MISO, uint32_t count) override;

  private:
    PCIE_CSR_Pipe pipe;
    uint32_t subdeviceIndex;
};

} // namespace lime

#endif // LIME_LMS64C_ADF_OVER_PCIE_MMX8_H
