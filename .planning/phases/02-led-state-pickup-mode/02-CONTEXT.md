# Phase 2: LED State + Pickup Mode - Context

**Gathered:** 2026-02-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Implement pickup mode FSM for faders, pan pots, and sliders — suppressing MIDI output after a bank switch until the physical position crosses the DAW value. Provide lazy-reveal visual feedback using channel strip button LEDs and grid rows. Reflect loop, punch, and metronome state from Logic Pro on their respective grid button LEDs.

</domain>

<decisions>
## Implementation Decisions

### Pickup trigger (reveal condition)
- Out-of-sync state is NOT shown immediately on bank switch — no visual noise by default
- Out-of-sync indicators are revealed only when the user takes action on that channel strip: any button press, fader move, or pan pot move on that channel's physical controls
- Until triggered, the channel looks and behaves normally from the user's perspective

### Out-of-sync visual feedback
- Channel strip button (P1–P8 for that channel) pulses to indicate out-of-sync state
- Grid row 1: one LED per channel indicates pan pot sync state
- Grid rows 2–3: one LED per slider indicates slider sync state (8 LEDs total, physically aligned to horizontal position of each slider)
- Pulse rate: medium (~2–3 Hz), symmetric (50/50 duty cycle)
- Pulse speed encodes distance from target: faster = farther from DAW value, slows as physical approaches target — gives "warmer/colder" feel without directional encoding
- All indicators for a channel go dark the moment that control picks up; no confirmation animation, no "all clear" flash

### Pickup crossover detection
- Deadband: ±2–3 MIDI units around the DAW target value — compensates for ADC noise without feeling loose
- Either approach direction counts — physical does not need to cross from the "correct" side; entering the deadband from any direction triggers pickup
- Edge case: if physical position and DAW value are both at 0 or 127, pickup fires immediately without requiring sweep
- On pickup: suppress MIDI until crossover, then immediately send the current physical value so the DAW snaps to match

### Pickup scope
- Full pickup FSM (MIDI suppression until crossover) applies to: sliders (faders), pan pots, and all other assignable knobs after a bank switch
- Not visual-only — all three control types suppress output until they sync

### State LEDs (loop, punch, metronome)
- All three state LEDs are in scope: Loop (Cycle), Punch In/Out, Metronome (Click)
- Physical buttons are on the grid (K1–K16)
- Button-to-function mapping lives in a compile-time config header (e.g., `mcuConfig.h`) — not EEPROM, not hardcoded in logic
- LED responds immediately (instant snap) when Logic sends Note On/Off for that state — no fade
- Brightness: same as channel strip LEDs (LED_MAX_BRIGHTNESS)

### Claude's Discretion
- Exact config header name and structure for button mapping
- Which specific grid LEDs align to which rows for the sync indicators (researcher should verify against pinDefines.h and physical layout)
- ADC noise filtering for crossover detection (3-value threshold already exists on Potentiometer class — may be sufficient)
- How pickup FSM state is stored (per-control struct, bitfield, etc.)

</decisions>

<specifics>
## Specific Ideas

- The reveal-on-action model is intentional UX: the player isn't bombarded with 8 blinking LEDs on every bank switch. Only the channel they interact with shows its sync state.
- The grid rows as sync indicators leverage physical alignment — row 1 sits near the pan pots, rows 2–3 sit near the sliders, so the LED position visually maps to the physical control that needs attention.
- Pulse speed as distance encoding feels like a "radar ping" — fast when far, slowing as you approach, then silence when caught.

</specifics>

<deferred>
## Deferred Ideas

- None — discussion stayed within phase scope.

</deferred>

---

*Phase: 02-led-state-pickup-mode*
*Context gathered: 2026-02-25*
