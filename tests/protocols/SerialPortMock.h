#ifndef LIME_SERIALPORTMOCK_H
#define LIME_SERIALPORTMOCK_H

#include "protocols/ISerialPort.h"
#include "protocols/LMS64CProtocol.h"

namespace lime::testing {

class SerialPortMock : public ISerialPort
{
  public:
    SerialPortMock()
    {
        ON_CALL(*this, Write).WillByDefault([](const uint8_t* data, size_t length, int timeout_ms) -> int { return length; });
        ON_CALL(*this, Read).WillByDefault([](uint8_t* data, size_t length, int timeout_ms) -> int { return length; });
    }

    MOCK_METHOD(int, Write, (const uint8_t* data, size_t length, int timeout_ms), (override));
    MOCK_METHOD(int, Read, (uint8_t * data, size_t length, int timeout_ms), (override));

    MOCK_METHOD(OpStatus, RunControlCommand, (uint8_t * data, size_t length, int timeout_ms), (override));
    MOCK_METHOD(OpStatus, RunControlCommand, (uint8_t * request, uint8_t* response, size_t length, int timeout_ms), (override));
};

} // namespace lime::testing

#endif // LIME_SERIALPORTMOCK_H
