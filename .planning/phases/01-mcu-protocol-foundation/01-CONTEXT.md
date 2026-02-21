# Phase 1: MCU Protocol Foundation - Context

**Gathered:** 2026-02-21
**Status:** Ready for planning

<domain>
## Phase Boundary

Establish the MCU protocol foundation: complete the SysEx handshake so Logic Pro recognizes this device as a Mackie Control surface, replace CC-based button sends with MCU Note Bang messages, convert slider output to 14-bit Pitch Bend on per-fader MIDI channels, and wire bidirectional LED feedback through new MCU-aware classes. Bug fixes (copy-by-value Button, Serial debug output) are prerequisites before any MCU work.

</domain>

<decisions>
## Implementation Decisions

### Device Identity
- Device name: **"Phaedr"** — this is what Logic Pro displays in its Control Surfaces panel
- SysEx serial bytes: ASCII "PHAEDR" + null pad — `0x50 0x48 0x41 0x45 0x44 0x52 0x00`
- These 7 bytes appear in the Host Connection Query and confirm messages

### Button LED Confirmation
- **Wait for Logic** — all LEDs with LED hardware update only after Logic sends a Note On/Off confirmation; no optimistic local toggling
- Play (btn 16) and Stop (btn 15) have no LEDs — the wait-for-Logic rule is irrelevant for them
- Record (btn 14) has an LED — waits for Logic to confirm record arm state
- Channel strip buttons (REC/SOLO/MUTE/SELECT) all wait for Logic
- This means the Button class must NOT toggle LED state on press in MCU mode

### Legacy CC Compatibility
- **Clean break** — all CC output removed. Firmware speaks MCU only after Phase 1
- The existing CC-based ButtonRegistry can be replaced entirely by a Note-to-Button lookup
- No multi-protocol support, no compile-time mode switch

### Class Architecture
- **New MCUButton class** — handles MCU-mode buttons (sends Note Bang, no local LED toggle, LED updates only via `setLedState()` from DAW feedback)
- **New Fader class** — handles MCU-mode sliders (sends 14-bit Pitch Bend on per-fader MIDI channel, not CC); separate from the Potentiometer class which stays for knobs
- Existing `Button` and `Potentiometer` classes can remain as dead code or be removed — Claude's discretion on cleanup
- Existing `ButtonRegistry` can be replaced or extended — Claude's discretion, given MCU-only direction

### Channel Button Send Behavior
- **Pure MCU Note Bang**: send Note On followed immediately by Note Off; don't track toggle state locally
- Logic Pro owns all channel button state; hardware just sends bangs and displays whatever Logic says

### Pre-Handshake Behavior
- If Logic doesn't respond to the handshake — Claude's discretion on retry strategy (retry periodically is preferred over silent wait indefinitely)

### Claude's Discretion
- Whether to keep or delete `Button` and `Potentiometer` classes after new MCU classes are introduced
- ButtonRegistry replacement architecture (new class vs. extend existing)
- Pre-handshake retry interval/count
- Debug Serial output removal strategy (define vs. delete)

</decisions>

<specifics>
## Specific Ideas

- "Phaedr" should appear as the device name in Logic Pro's Control Surfaces → Mackie Control panel
- The SysEx serial is `PHAEDR\0` — readable, branded, unique enough for a single-device setup

</specifics>

<deferred>
## Deferred Ideas

- **Pre-handshake idle LED animation** — User wants "a cool idle LED animation" while waiting for Logic to connect. This belongs in Phase 4 (Animation Manager), not Phase 1. Phase 4 should implement a pre-connection idle pattern (e.g., slow pulse or sweep) that plays until the MCU handshake completes.

</deferred>

---

*Phase: 01-mcu-protocol-foundation*
*Context gathered: 2026-02-21*
