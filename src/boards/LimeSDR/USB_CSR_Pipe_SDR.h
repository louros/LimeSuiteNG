#ifndef LIME_USB_CSR_Pipe_SDR_H
#define LIME_USB_CSR_Pipe_SDR_H

#include "comms/USB/USB_CSR_Pipe.h"
#include "comms/USB/FX3/FX3.h"

#include <cstdint>

namespace lime {

/** @brief Class for interfacing with Control/Status registers (CSR) of LimeSDR-USB. */
class USB_CSR_Pipe_SDR : public USB_CSR_Pipe
{
  public:
    /**
      @brief Constructs a new USB_CSR_Pipe_SDR object
      @param port The FX3 communications port to use.
     */
    explicit USB_CSR_Pipe_SDR(FX3& port);

    int Write(const uint8_t* data, std::size_t length, int timeout_ms) override;
    int Read(uint8_t* data, std::size_t length, int timeout_ms) override;
    OpStatus RunControlCommand(uint8_t* data, size_t length, int timeout_ms) override;
    OpStatus RunControlCommand(uint8_t* request, uint8_t* response, size_t length, int timeout_ms) override;

  private:
    FX3& port;
};

} // namespace lime

#endif // LIME_USB_CSR_Pipe_SDR_H
