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

// MCU note numbers for transport mode state LEDs
// Loop/Cycle button state:
#define MCU_NOTE_LOOP           86
// Punch In (punch start) state:
#define MCU_NOTE_PUNCH_IN       85
// Metronome/Click toggle state:
#define MCU_NOTE_METRONOME      89

// Track SELECT notes (track selection buttons)
#define MCU_NOTE_SELECT_BASE    24  // SELECT Ch1-Ch8: notes 24-31

// Transport button notes
#define MCU_NOTE_STOP           93  // Transport: Stop
#define MCU_NOTE_PLAY           94  // Transport: Play
#define MCU_NOTE_RECORD         95  // Transport: Record

// Session action notes (Logic Pro MCU function buttons)
#define MCU_NOTE_SAVE           50  // Save project
#define MCU_NOTE_UNDO           51  // Undo

// User-assignable function keys — bind in Logic Key Commands → Mackie Control
#define MCU_NOTE_F1             54
#define MCU_NOTE_F2             55
#define MCU_NOTE_F3             56
#define MCU_NOTE_F4             57
#define MCU_NOTE_F5             58
#define MCU_NOTE_F6             59
#define MCU_NOTE_F7             60
#define MCU_NOTE_F8             61

// Grid button indices (0-based) for mode LED feedback from Logic.
// K13 (index 12) is Loop — registered directly in init(), no constant needed.
#define MCU_METRO_GRID_INDEX    0   // K1 — Metronome LED
#define MCU_PUNCH_GRID_INDEX    1   // K2 — Punch In LED

#endif // mcu_config_h
