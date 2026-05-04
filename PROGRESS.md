# Progress — Button UX redesign

Last activity: 2026-04-11. Firmware built and uploaded to Teensy 3.5 successfully.

## What changed

Redesigned the button layout to drop redundant track SELECT, drop bank navigation (device is now locked to the first 8 Logic tracks), and fill the free grid with useful Logic macros.

### New layout

```
Row 1:  K1 Metronome | K2 Punch In | K3 Save   | K4 Undo
Row 2:  K5 F1        | K6 F2       | K7 F3     | K8 F4
Row 3:  K9 F5        | K10 F6      | K11 F7    | K12 F8
Row 4:  K13 Cycle    | K14 Record  | K15 Play  | K16 Stop

P1–P8: Mute toggles (notes 16–23), LED mirrors Logic mute state
```

- **Track buttons (P1–P8)** — press toggles mute, LED reflects Logic. Pickup-distance blink on fader/pan movement is preserved.
- **Fader/knob auto-focus** — moving a fader or pan knob still sends a SELECT note so Logic's inspector and Smart Controls follow your active hand.
- **Metronome / Punch In** are now bidirectional (press to toggle, LED mirrors).
- **F1–F8** are user-assignable — bind them in Logic → Key Commands → Mackie Control → Function Key 1–8.

## Files modified

- `src/mcuConfig.h` — added `MCU_NOTE_SAVE`, `MCU_NOTE_UNDO`, `MCU_NOTE_F1`–`F8`; removed `MCU_NOTE_BANK_LEFT`/`BANK_RIGHT`/`INPUT_MON_BASE`; moved `MCU_METRO_GRID_INDEX` to 0 and `MCU_PUNCH_GRID_INDEX` to 1.
- `src/inputManager.cpp` — rewrote grid button setup (K1–K12), switched track buttons to `MCU_NOTE_MUTE_BASE`, updated NoteRegistry loop to Mute range, replaced track button `poll()` + manual SELECT send in `readAll()` with `read()` (auto-sends mute), kept fader/knob SELECT-on-move.

Nothing is committed yet — the working tree has the above changes uncommitted.

## Build/upload status

- `pio run` — SUCCESS (Flash 4.4% / RAM 3.1%)
- `pio run --target upload` — SUCCESS after opening Teensy Loader

## Left to verify (pick up here after break)

The Save/Undo/F1–F8 note numbers are from community MCU docs with the same MEDIUM-confidence caveat called out in `src/mcuConfig.h:10–16`. **Verify before trusting the LEDs or behavior.**

1. Open Logic Pro and `pio device monitor`.
2. In Logic go to **Logic Pro → Key Commands → Mackie Control → Function Key 1–8** and watch which notes Logic expects. Correct any mismatches in `mcuConfig.h`.
3. Run the test checklist from the plan:
   - **Track button mute** — press P1 → Track 1 mutes, P1 LED on. Press again → unmute. Toggle mute in Logic's UI → LED should follow.
   - **Fader auto-focus preservation** — move fader 3 → Logic selects Track 3 (inspector updates) but P3's mute LED state must NOT change. Confirms we kept SELECT-on-move and stripped SELECT-on-press.
   - **Pickup blink regression** — edit a fader in Logic's UI so hardware is out of sync, then slowly push the hardware fader. Matching P button should blink, stop blinking on crossover pickup.
   - **Grid macros** — K1 Metronome, K2 Punch In, K3 Save, K4 Undo. Then bind F1–F8 in Logic Key Commands (e.g., F1 = Go to Beginning, F2 = Toggle Mixer) and press K5–K12 to verify.
   - **Transport regression** — K13 Cycle, K14 Record, K15 Play, K16 Stop still work.
   - **No bank nav** — verify nothing in Logic tries to shift the 8-track window.

## References

- Approved plan: `/Users/kenmarut/.claude/plans/harmonic-munching-ullman.md`
- Architecture notes: `CLAUDE.md`
