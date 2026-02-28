#ifndef mcu_config_h
#define mcu_config_h

// ============================================================
// MCU Mode Button Note Numbers — Logic Pro LED State Feedback
// ============================================================
// Logic Pro sends Note On (velocity 127 = on, 0 = off) for these functions.
// NoteRegistry routes these notes to the grid button LEDs defined below.
//
// IMPORTANT: These values are from community MCU documentation (MEDIUM confidence).
// Verify with a MIDI monitor or temporary Serial.print in handleNoteOn() before
// relying on them. See RESEARCH.md Pitfall 5 for verification procedure.
//
// To verify: open Logic, enable Serial monitor (pio device monitor), toggle
// Loop/Punch/Metronome in Logic, observe which note numbers appear in Serial output
// (add temporary Serial.print(note) in handleNoteOn if needed), then update below.

// Track strip function note bases (MCU protocol, channel 1)
#define MCU_NOTE_ARM_BASE         0   // Arm  Ch1-Ch8: notes 0-7
#define MCU_NOTE_SOLO_BASE        8   // Solo Ch1-Ch8: notes 8-15
#define MCU_NOTE_MUTE_BASE       16   // Mute Ch1-Ch8: notes 16-23
// Input monitoring — not standard MCU, UNVERIFIED. Verify with Logic MIDI monitor
// same procedure as mode button notes in RESEARCH.md Pitfall 5.
#define MCU_NOTE_INPUT_MON_BASE  47

// MCU note numbers for transport mode state LEDs
// Loop/Cycle button state:
#define MCU_NOTE_LOOP           86
// Punch In (punch start) state:
#define MCU_NOTE_PUNCH_IN       85
// Metronome/Click toggle state:
#define MCU_NOTE_METRONOME      89

// Grid button indices (0-based) for mode LED feedback from Logic.
// K13 (index 12) is Loop — registered directly in init(), no constant needed.
// K10 (index 9) and K11 (index 10) host Punch/Metronome until reassigned.
#define MCU_PUNCH_GRID_INDEX    9   // K10 — Punch In LED
#define MCU_METRO_GRID_INDEX    10  // K11 — Metronome LED

#endif // mcu_config_h
