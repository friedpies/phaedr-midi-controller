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

// MCU note numbers for transport mode state LEDs
// Loop/Cycle button state:
#define MCU_NOTE_LOOP           86
// Punch In (punch start) state:
#define MCU_NOTE_PUNCH_IN       85
// Metronome/Click toggle state:
#define MCU_NOTE_METRONOME      89

// Grid button indices (0-based, K1=index 0 ... K16=index 15)
// These define which physical grid button receives the LED feedback.
// Current Phase 1 placeholder assignments for K1-K12:
//   K1-K8  (indices 0-7):  MUTE Ch1-Ch8 (note 16-23)
//   K9-K12 (indices 8-11): SELECT Ch1-Ch4 (note 24-27)
// Phase 3 will reassign these when the full grid transport layout is implemented.
// For Phase 2, we wire Loop/Punch/Metronome to grid buttons that are not already
// assigned critical transport functions. K9-K11 (indices 8-10) are SELECT placeholders
// and are appropriate hosts for mode LEDs until Phase 3 finalizes grid layout.
//
// Change these indices in Phase 3 when grid button note assignments are confirmed.
#define MCU_LOOP_GRID_INDEX     8   // K9  — Loop/Cycle LED
#define MCU_PUNCH_GRID_INDEX    9   // K10 — Punch In LED
#define MCU_METRO_GRID_INDEX    10  // K11 — Metronome LED

#endif // mcu_config_h
