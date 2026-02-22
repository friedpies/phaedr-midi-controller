#ifndef mcu_protocol_h
#define mcu_protocol_h

#include <Arduino.h>

class MCUProtocol
{
public:
    MCUProtocol();

    // Call from setup() to register SysEx callback and start retry timer
    void begin();

    // Call from loop() to handle retry timer (sends unsolicited query if no response in 5s)
    void update();

    // SysEx callback — register with usbMIDI.setHandleSystemExclusive()
    // Must be called from main.cpp's static SysEx handler which forwards to this
    void handleSysEx(const uint8_t* data, uint16_t length, bool complete);

    bool isHandshakeComplete() const { return _handshakeComplete; }

private:
    void sendHostConnectionQuery();
    void sendConfirmation();
    bool validateChallengeResponse(const uint8_t response[4]) const;

    bool _handshakeComplete;
    unsigned long _lastRetryMs;
    static const unsigned long RETRY_INTERVAL_MS = 5000;  // retry every 5 seconds

    // Device identity — "PHAEDR\0" per CONTEXT.md locked decision
    static const uint8_t MCU_HEADER[5];   // F0 00 00 66 14
    static const uint8_t DEVICE_SERIAL[7]; // 0x50 0x48 0x41 0x45 0x44 0x52 0x00

    // Static challenge bytes — fixed for simplicity; could be randomized later
    // 4 non-zero 7-bit bytes
    static const uint8_t CHALLENGE[4];    // 0x7A 0x6B 0x5C 0x4D
};

extern MCUProtocol mcuProtocol;

#endif
