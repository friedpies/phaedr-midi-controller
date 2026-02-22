#include "mcuProtocol.h"

// MCU SysEx header: F0 00 00 66 14 (Mackie Control Universal, model 0x14)
const uint8_t MCUProtocol::MCU_HEADER[5] = {0xF0, 0x00, 0x00, 0x66, 0x14};

// "PHAEDR\0" in ASCII — per CONTEXT.md locked decision
const uint8_t MCUProtocol::DEVICE_SERIAL[7] = {0x50, 0x48, 0x41, 0x45, 0x44, 0x52, 0x00};

// Static challenge bytes — 4 non-zero 7-bit values
const uint8_t MCUProtocol::CHALLENGE[4] = {0x7A, 0x6B, 0x5C, 0x4D};

MCUProtocol mcuProtocol;

MCUProtocol::MCUProtocol()
    : _handshakeComplete(false), _lastRetryMs(0)
{
}

void MCUProtocol::begin()
{
    // Send an unsolicited Host Connection Query on startup.
    // If Logic is already running with this surface configured, it responds with a reply.
    // If Logic hasn't started yet, the retry timer in update() resends every 5 seconds.
    sendHostConnectionQuery();
    _lastRetryMs = millis();
}

void MCUProtocol::update()
{
    if (_handshakeComplete) return;

    // Retry unsolicited query if Logic hasn't responded
    if (millis() - _lastRetryMs >= RETRY_INTERVAL_MS) {
        sendHostConnectionQuery();
        _lastRetryMs = millis();
    }
}

void MCUProtocol::handleSysEx(const uint8_t* data, uint16_t length, bool complete)
{
    // Only process complete messages
    if (!complete) return;

    // Minimum length: 7 bytes (header 5 + msgType 1 + F7 1)
    if (length < 7) return;

    // Verify MCU header: F0 00 00 66 14
    if (memcmp(data, MCU_HEADER, 5) != 0) return;

    uint8_t msgType = data[5];

    if (msgType == 0x00) {
        // Step 1: Device Query received from Logic
        // Respond with Host Connection Query (Step 2)
        sendHostConnectionQuery();
        _lastRetryMs = millis();  // reset retry timer

    } else if (msgType == 0x02) {
        // Step 3: Host Connection Reply from Logic
        // Message layout: F0 00 00 66 14 02 [serial 7] [response 4] F7
        // response bytes start at data[13]
        if (length < 18) return;  // too short to contain serial + response

        const uint8_t* response = data + 13;
        if (validateChallengeResponse(response)) {
            sendConfirmation();
            _handshakeComplete = true;
        }
        // If validation fails, do not confirm. Logic will retry the Device Query.
    }
}

void MCUProtocol::sendHostConnectionQuery()
{
    // Step 2: Host Connection Query
    // Format: F0 00 00 66 14 01 [serial 7 bytes] [challenge 4 bytes] F7
    // Total: 18 bytes
    uint8_t msg[18];
    msg[0] = 0xF0;
    msg[1] = 0x00;
    msg[2] = 0x00;
    msg[3] = 0x66;
    msg[4] = 0x14;
    msg[5] = 0x01;                          // message type: Host Connection Query
    memcpy(msg + 6, DEVICE_SERIAL, 7);             // device serial "PHAEDR\0"
    memcpy(msg + 13, CHALLENGE, 4);         // challenge bytes
    msg[17] = 0xF7;
    usbMIDI.sendSysEx(18, msg, true);       // hasTerm=true: msg already has F0 and F7
}

void MCUProtocol::sendConfirmation()
{
    // Step 4: Host Connection Confirmation
    // Format: F0 00 00 66 14 03 [serial 7 bytes] F7
    // Total: 14 bytes
    uint8_t confirm[14];
    confirm[0] = 0xF0;
    confirm[1] = 0x00;
    confirm[2] = 0x00;
    confirm[3] = 0x66;
    confirm[4] = 0x14;
    confirm[5] = 0x03;                       // message type: Host Connection Confirmation
    memcpy(confirm + 6, DEVICE_SERIAL, 7);          // device serial "PHAEDR\0"
    confirm[13] = 0xF7;
    usbMIDI.sendSysEx(14, confirm, true);    // hasTerm=true
}

bool MCUProtocol::validateChallengeResponse(const uint8_t response[4]) const
{
    // Challenge-response algorithm from Ardour source.cc (open source, verified):
    // https://github.com/ardour/ardour/blob/master/libs/surfaces/mackie/surface.cc
    // c = CHALLENGE (4 bytes sent by device in Step 2)
    // r = response (4 bytes received from Logic in Step 3)
    const uint8_t* c = CHALLENGE;
    uint8_t expected[4];
    expected[0] = 0x7F & (c[0] + (c[1] ^ 0x0A) - c[3]);
    expected[1] = 0x7F & ((c[2] >> 4) ^ (c[0] + c[3]));
    expected[2] = 0x7F & ((c[3] - (c[2] << 2)) ^ (c[0] | c[1]));
    expected[3] = 0x7F & (c[1] - c[2] + (0xF0 ^ (c[3] << 4)));
    return memcmp(expected, response, 4) == 0;
}
