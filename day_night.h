/*
 * day_night.h
 *
 * Day/Night cycle system for TermiCraft.
 * Handles time progression, sky rendering (sun, moon, stars),
 * and sky background colors across four phases: day, dusk, night, dawn.
 *
 * Author: Mohit
 *
 * =========================================================================
 * OVERVIEW
 * =========================================================================
 * This module owns the global tick counter (tickCount) and everything
 * that derives from it: which phase of the day/night cycle we are in,
 * what ANSI background color the sky should show, and how to paint a
 * single sky cell (sun glyph / moon glyph / star / plain sky) into a
 * caller-supplied output buffer.
 *
 * The cycle is divided into four named phases:
 *
 *   TIME_DAY   — ticks [0,       DUSK_START)   bright blue sky, sun visible
 *   TIME_DUSK  — ticks [DUSK_START, NIGHT_START) warm orange sky, sun sets
 *   TIME_NIGHT — ticks [NIGHT_START, DAWN_START)  dark navy sky, moon + stars
 *   TIME_DAWN  — ticks [DAWN_START, CYCLE_LENGTH) muted pink sky, moon sets
 *
 * =========================================================================
 * CONSTANTS
 * =========================================================================
 *
 *   CYCLE_LENGTH  — total ticks for one full day; at 20 ticks/sec this is
 *                   exactly 5 minutes.  Raise it to lengthen the in-game day.
 *
 *   DAY_START     — always 0; the cycle wraps with (tickCount % CYCLE_LENGTH)
 *
 *   DUSK_START    — 40% through the cycle; sun begins to set
 *
 *   NIGHT_START   — 50% through; the sky goes dark and the moon appears
 *
 *   DAWN_START    — 85% through; the sky lightens and the moon begins to set
 *
 * =========================================================================
 * FUNCTION REFERENCE
 * =========================================================================
 *
 *   getTimeOfDay()
 *     Returns the TimeOfDay enum value for the current tick.
 *     Computes (tickCount % CYCLE_LENGTH) and checks which phase bracket
 *     the result falls into.  Used internally and by callers that need to
 *     branch on day vs. night (e.g., enemy spawn rate, mob behaviour).
 *
 *   getTimeLabel()
 *     Returns a short C-string label for the current phase — "DAY", "DUSK",
 *     "NIGHT", or "DAWN".  Used by the HUD renderer to display the current
 *     time of day to the player.
 *
 *   getSkyBg()
 *     Returns an ANSI 256-colour background escape sequence appropriate for
 *     the current time of day.  The caller prepends this to any sky cell it
 *     prints so that the terminal background colour matches the phase:
 *       DAY   → bright blue  (\033[48;5;39m)
 *       DUSK  → warm orange  (\033[48;5;130m)
 *       NIGHT → dark navy    (\033[48;5;17m)
 *       DAWN  → muted pink   (\033[48;5;95m)
 *
 *   tickDayCycle()
 *     Increments the global tickCount by one.  Should be called once per
 *     game tick (i.e., inside the main update loop) to advance time.
 *     The counter wraps naturally through modular arithmetic; it is never
 *     reset directly.
 *
 *   renderSkyCell(skyRow, vx, vpWidth, buf)
 *     The primary rendering entry point for sky cells.  Given a world-space
 *     row index (skyRow), a viewport x coordinate (vx), the total viewport
 *     width (vpWidth), and a writable buffer pointer (buf), this function
 *     composes the full ANSI escape sequence for one sky character and
 *     writes it into buf.  The return value is the number of bytes written,
 *     so the caller can advance its buffer pointer by that amount.
 *
 *     Rendering priority (first match wins):
 *       1. Sun glyph  — bright yellow, visible during DAY and transition phases
 *       2. Moon glyph — bright white,  visible during NIGHT and DAWN/DUSK
 *       3. Star       — dot/cross/asterisk, only rendered outside TIME_DAY,
 *                       drawn at deterministic positions via a hash so that
 *                       the star field is stable across frames
 *       4. Plain sky  — just the background colour with a space character
 *
 * =========================================================================
 * SHARED STATE
 * =========================================================================
 *
 *   extern int tickCount
 *     The single source of truth for game time.  Declared extern here so
 *     that any translation unit which includes this header can read the
 *     current tick.  The actual storage is in day_night.cpp.
 *     Only tickDayCycle() should write to this variable.
 *
 * =========================================================================
 * DEPENDENCIES
 * =========================================================================
 *   types.h  — for GameState, Block, BlockType, and the depth-level constants
 *              (SURFACE_LEVEL, STONE_LEVEL, DEEP_LEVEL) used implicitly by
 *              the fog-of-war caller.
 */

#ifndef DAY_NIGHT_H
#define DAY_NIGHT_H

#include "types.h"

// -------------------------------------------------------------------------
// Cycle timing (ticks). At 20 ticks/sec, 2400 = 2 min full cycle.
// Adjust CYCLE_LENGTH to change how long a day lasts.
// -------------------------------------------------------------------------

// Total number of ticks in one complete day/night cycle.
// Increase this value to make days feel longer in real time.
const int CYCLE_LENGTH = 6000;

// Tick index at which the DAY phase begins — always 0 (start of cycle).
const int DAY_START    = 0;

// Tick index at which DUSK begins: 40% of the way through the cycle.
// The sun starts moving toward the horizon and the sky warms to orange.
const int DUSK_START   = (int)(CYCLE_LENGTH * 0.40);

// Tick index at which NIGHT begins: 50% of the way through the cycle.
// The sky goes dark, the sun is off-screen, and the moon appears.
const int NIGHT_START  = (int)(CYCLE_LENGTH * 0.50);

// Tick index at which DAWN begins: 85% of the way through the cycle.
// The sky shifts to muted pink and the moon begins to set.
const int DAWN_START   = (int)(CYCLE_LENGTH * 0.85);

// -------------------------------------------------------------------------
// TimeOfDay enum — represents which named phase the current tick falls into.
// Used as the return type of getTimeOfDay() and as a switch key throughout
// the day_night module.
// -------------------------------------------------------------------------
enum TimeOfDay {
    TIME_DAY,    // Daytime  — blue sky, sun traverses left to right
    TIME_DUSK,   // Dusk     — orange sky, sun sets toward the right edge
    TIME_NIGHT,  // Nighttime — dark sky, moon + stars visible
    TIME_DAWN    // Dawn     — pink sky, moon sets, sun about to rise
};

// -------------------------------------------------------------------------
// Shared tick counter — declared here, defined in day_night.cpp.
// All other modules should treat this as read-only; advance it only via
// tickDayCycle().
// -------------------------------------------------------------------------
extern int tickCount;

// -------------------------------------------------------------------------
// getTimeOfDay
//
// Computes (tickCount % CYCLE_LENGTH) and returns the matching TimeOfDay
// enum value based on the four phase brackets defined by the constants above.
//
// Return values:
//   TIME_DAY   if t < DUSK_START
//   TIME_DUSK  if t < NIGHT_START
//   TIME_NIGHT if t < DAWN_START
//   TIME_DAWN  otherwise (t < CYCLE_LENGTH)
// -------------------------------------------------------------------------
TimeOfDay getTimeOfDay();

// -------------------------------------------------------------------------
// getTimeLabel
//
// Wraps getTimeOfDay() and converts the result to a human-readable string.
// Returns one of: "DAY", "DUSK", "NIGHT", "DAWN".
// Intended for the HUD line printed above the viewport each frame.
// -------------------------------------------------------------------------
const char* getTimeLabel();

// -------------------------------------------------------------------------
// getSkyBg
//
// Returns an ANSI 256-colour background escape sequence for the current
// time of day.  Should be prepended to every sky character printed so the
// terminal cell background colour matches the phase.
//
// Sequences returned:
//   TIME_DAY   → "\033[48;5;39m"   (bright blue)
//   TIME_DUSK  → "\033[48;5;130m"  (warm orange)
//   TIME_NIGHT → "\033[48;5;17m"   (dark navy)
//   TIME_DAWN  → "\033[48;5;95m"   (muted pink)
// -------------------------------------------------------------------------
const char* getSkyBg();

// -------------------------------------------------------------------------
// tickDayCycle
//
// Advances the global tickCount by one.  Call once per game update tick
// (i.e., once per iteration of the main loop) to move time forward.
// The counter is never reset; getTimeOfDay() uses modular arithmetic so
// the cycle repeats automatically.
// -------------------------------------------------------------------------
void tickDayCycle();

// -------------------------------------------------------------------------
// renderSkyCell
//
// Renders a single sky cell into the caller's output buffer and returns the
// number of bytes written.  The caller should advance its write pointer by
// this amount before rendering the next cell.
//
// Parameters:
//   skyRow  — world Y coordinate of the row being rendered.
//             Used to vertically position sun/moon art (art spans 3 rows).
//   vx      — viewport X coordinate (0-based column within the viewport).
//             Used to horizontally position sun/moon art and to test stars.
//   vpWidth — total width of the viewport in columns.
//             Used to compute the sun/moon horizontal position as a fraction
//             of the viewport so they scale with different terminal widths.
//   buf     — pointer into the caller's pre-allocated output buffer.
//             The function writes a complete ANSI escape sequence + one
//             visible character + a reset sequence here.
//
// Rendering logic (first match wins):
//   1. If (skyRow, vx) falls inside the sun art for the current sun column
//      and the character is not a space, emit the sun glyph in bright yellow.
//   2. Else if (skyRow, vx) falls inside the moon art and the char != space,
//      emit the moon glyph in bright white.
//   3. Else if the current phase is not TIME_DAY and isStarAt(skyRow, vx)
//      returns true, emit a pseudo-random star character (. + * `) whose
//      brightness depends on whether it is night or dawn.
//   4. Else emit a plain space with only the sky background colour.
//
// All output includes an ANSI reset (\033[0m) after the character so that
// subsequent non-sky cells are not affected by the background colour.
// -------------------------------------------------------------------------
int renderSkyCell(int skyRow, int vx, int vpWidth, char* buf);

#endif
