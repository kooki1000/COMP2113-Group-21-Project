/*
 * day_night.cpp
 *
 * Day/Night cycle system for TermiCraft.
 * Manages time progression and sky rendering with sun, moon, and stars.
 *
 * Author: Mohit
 *
 * =========================================================================
 * MODULE OVERVIEW
 * =========================================================================
 * This file implements the full day/night cycle for TermiCraft.  It is the
 * single authoritative source for the global tick counter and for every
 * piece of rendering that belongs to the sky region of the viewport.
 *
 * The module is organised into six logical sections:
 *
 *   1. SHARED STATE        — the global tickCount variable that all other
 *                            modules read (but must not write).
 *
 *   2. TIME OF DAY         — getTimeOfDay(), getTimeLabel(), tickDayCycle().
 *                            These three functions expose the cycle state to
 *                            the rest of the game.
 *
 *   3. SKY BACKGROUND      — getSkyBg() maps the current phase to an ANSI
 *                            256-colour background escape string.
 *
 *   4. SUN / MOON ART      — static ASCII art arrays and their dimensions,
 *                            plus the STAR_CHARS palette used for the star
 *                            field.
 *
 *   5. POSITION HELPERS    — getSunCol() and getMoonCol() compute the
 *                            current horizontal pixel column for each body
 *                            as a linear interpolation across the viewport.
 *
 *   6. ART LOOKUP + RENDER — getSunChar(), getMoonChar(), isStarAt(), and
 *                            the public renderSkyCell() that assembles them
 *                            all into a single ANSI-escaped character.
 *
 * =========================================================================
 * FUNCTION REFERENCE
 * =========================================================================
 *
 *   getTimeOfDay()
 *       Determines which of the four cycle phases is currently active by
 *       taking (tickCount % CYCLE_LENGTH) and comparing it to the boundary
 *       constants DUSK_START, NIGHT_START, and DAWN_START.
 *       Returns a TimeOfDay enum: TIME_DAY, TIME_DUSK, TIME_NIGHT, TIME_DAWN.
 *
 *   getTimeLabel()
 *       Thin wrapper around getTimeOfDay().  Converts the enum to a
 *       human-readable C-string for display in the HUD.
 *       Returns: "DAY" | "DUSK" | "NIGHT" | "DAWN" | "?" (unreachable).
 *
 *   tickDayCycle()
 *       Increments the global tickCount by one.  Must be called exactly
 *       once per game tick in the main update loop.
 *
 *   getSkyBg()
 *       Returns the ANSI background escape string for the current phase.
 *       Used by renderSkyCell() and by any other code that needs to paint
 *       sky-coloured terminal cells.
 *
 *   getSunCol(vpWidth)
 *       Computes the viewport-space column at which the centre of the sun
 *       should be drawn this tick.  During DAY the sun moves from 15% to
 *       85% of the viewport width.  During DUSK it continues off the right
 *       edge.  During DAWN it rises from the left edge.  Returns -100 at
 *       NIGHT so the sun art is safely off-screen.
 *       The result is clamped so the 5-wide sun art never clips the edge.
 *
 *   getMoonCol(vpWidth)
 *       Same logic as getSunCol() but for the moon.  The moon is visible
 *       during TIME_NIGHT and TIME_DAWN, and is off-screen (-100) during
 *       TIME_DAY.  During TIME_DUSK the moon is just beginning to rise
 *       from the left edge.
 *
 *   getSunChar(skyRow, vx, sunCx)
 *       Maps a (skyRow, viewport-x) coordinate to the corresponding
 *       character in the SUN_ART array, given the current sun centre column
 *       sunCx.  Returns 0 if the coordinate is outside the 5×3 art bounding
 *       box, so the caller can skip to the next priority test.
 *
 *   getMoonChar(skyRow, vx, moonCx)
 *       Same as getSunChar() but for the 3×3 MOON_ART array.
 *
 *   isStarAt(row, col)
 *       Deterministic hash function that returns true for roughly 1 in 12
 *       sky cells, producing a stable star field that does not flicker
 *       between frames.  The same (row, col) pair always yields the same
 *       result regardless of tickCount.
 *
 *   renderSkyCell(skyRow, vx, vpWidth, buf)
 *       Public entry point called by fog_of_war.cpp for every sky cell in
 *       the viewport each frame.  Consults the sun, moon, star, and plain-
 *       sky layers in priority order and writes the winning ANSI sequence
 *       into buf.  Returns the number of bytes written.
 *
 * =========================================================================
 * DEPENDENCIES
 * =========================================================================
 *   day_night.h — owns the public interface declarations and the constants
 *                 CYCLE_LENGTH, DAY_START, DUSK_START, NIGHT_START, DAWN_START
 *   <cstdio>    — for sprintf(), used to compose ANSI escape sequences
 *   <cstring>   — pulled in transitively; not directly used in this file
 */

#include "day_night.h"
#include <cstdio>
#include <cstring>

// =========================================================================
// SECTION 1 — SHARED STATE
// =========================================================================

// tickCount is the heartbeat of the entire day/night system.
// It is incremented once per game tick by tickDayCycle() and must never
// be written by any other function.  All phase queries are derived from
// (tickCount % CYCLE_LENGTH), so the cycle repeats automatically forever
// without any explicit reset logic.
int tickCount = 0;

// =========================================================================
// SECTION 2 — TIME OF DAY
// =========================================================================

// -------------------------------------------------------------------------
// getTimeOfDay
//
// Folds tickCount into the [0, CYCLE_LENGTH) window via modulus, then
// performs three sequential comparisons to determine the active phase.
// The comparisons mirror the boundary constants exactly:
//
//   t < DUSK_START   → TIME_DAY   (early in the cycle)
//   t < NIGHT_START  → TIME_DUSK  (sun is setting)
//   t < DAWN_START   → TIME_NIGHT (full dark, moon up)
//   else             → TIME_DAWN  (sky brightening, moon setting)
//
// This function is called many times per frame (once per sky cell rendered)
// so it is kept intentionally branchless and arithmetic-light.
// -------------------------------------------------------------------------
TimeOfDay getTimeOfDay() {
    // Wrap the ever-increasing tick counter into one cycle window
    int t = tickCount % CYCLE_LENGTH;

    // Check each phase boundary in chronological order
    if (t < DUSK_START)  return TIME_DAY;    // Bright daytime
    if (t < NIGHT_START) return TIME_DUSK;   // Transitioning to night
    if (t < DAWN_START)  return TIME_NIGHT;  // Full night
    return TIME_DAWN;                         // Transitioning back to day
}

// -------------------------------------------------------------------------
// getTimeLabel
//
// Returns a short uppercase string label for the HUD.  The label is printed
// next to the depth/direction indicator so the player always knows what
// time of day it is without needing to watch the sky.
// -------------------------------------------------------------------------
const char* getTimeLabel() {
    switch (getTimeOfDay()) {
        case TIME_DAY:   return "DAY";    // Full daylight
        case TIME_DUSK:  return "DUSK";   // Evening transition
        case TIME_NIGHT: return "NIGHT";  // Full darkness
        case TIME_DAWN:  return "DAWN";   // Morning transition
    }
    return "?"; // Unreachable; satisfies the compiler
}

// -------------------------------------------------------------------------
// tickDayCycle
//
// The sole writer of tickCount.  Called once per game tick by the main
// loop.  Because getTimeOfDay() always uses modular arithmetic, there is no
// need to clamp or reset this counter — it can grow indefinitely.
// -------------------------------------------------------------------------
void tickDayCycle() {
    tickCount++;
}

// =========================================================================
// SECTION 3 — SKY BACKGROUND
// =========================================================================

// -------------------------------------------------------------------------
// getSkyBg
//
// Maps the current TimeOfDay to a terminal background colour escape code.
// The colours are chosen from the xterm-256 palette to give a visually
// distinct feel for each phase:
//
//   TIME_DAY   → colour 39  (bright azure blue)
//   TIME_DUSK  → colour 130 (burnt orange / amber)
//   TIME_NIGHT → colour 17  (deep navy, almost black)
//   TIME_DAWN  → colour 95  (dusty rose / muted pink)
//
// The trailing 'm' closes the SGR escape.  The caller must emit \033[0m
// after the character to reset the background for non-sky content.
// -------------------------------------------------------------------------
const char* getSkyBg() {
    switch (getTimeOfDay()) {
        case TIME_DAY:   return "\033[48;5;39m";   // bright blue — midday sky
        case TIME_DUSK:  return "\033[48;5;130m";  // warm orange — setting sun
        case TIME_NIGHT: return "\033[48;5;17m";   // dark navy   — starry night
        case TIME_DAWN:  return "\033[48;5;95m";   // muted pink  — sunrise glow
    }
    return ""; // Fallback: no background colour applied
}

// =========================================================================
// SECTION 4 — SUN / MOON ART
// =========================================================================

// Three-line ASCII art for the sun.
// Each string is exactly SUN_W (5) characters wide.
// The art is designed so the central 'O' sits at row index 1 (0-based),
// meaning the art occupies sky rows [centreRow+1 .. centreRow+3].
static const char* SUN_ART[] = {
    " \\|/ ",   // Row 0: rays above the disc
    "- O -",   // Row 1: horizontal rays and the disc itself
    " /|\\ "   // Row 2: rays below the disc
};
static const int SUN_H = 3;   // Height of the sun art in rows
static const int SUN_W = 5;   // Width  of the sun art in columns

// Three-line ASCII art for the moon.
// Each string is exactly MOON_W (3) characters wide.
// The crescent shape is approximated with parentheses and a tilde.
static const char* MOON_ART[] = {
    " _ ",   // Row 0: top of the crescent silhouette
    "( )",   // Row 1: body of the crescent
    " ~ "    // Row 2: tail / reflection shimmer below
};
static const int MOON_H = 3;   // Height of the moon art in rows
static const int MOON_W = 3;   // Width  of the moon art in columns

// Star character palette — four glyphs of increasing visual weight.
// isStarAt() selects from this array using a hash of the cell coordinates,
// so each star position always has the same glyph across all frames.
static const char STAR_CHARS[] = {'.', '+', '*', '`'};

// =========================================================================
// SECTION 5 — POSITION HELPERS
// =========================================================================

// -------------------------------------------------------------------------
// getSunCol
//
// Computes the viewport column at which the LEFT EDGE of the sun art
// should be drawn.  The sun centre moves as a linear fraction of vpWidth:
//
//   TIME_DAY:   from 15% vpWidth to 85% vpWidth across the day duration.
//               This maps the full daytime span onto most of the viewport.
//
//   TIME_DUSK:  from 85% vpWidth to 100% vpWidth — the sun slides off the
//               right edge as dusk progresses into full night.
//
//   TIME_DAWN:  from 0% vpWidth to 15% vpWidth — the sun has just risen
//               and is climbing from the left edge toward its daytime start.
//
//   TIME_NIGHT: returns -100 so the 5-wide art is completely off-screen.
//
// The result is clamped to [0, vpWidth - SUN_W] so that even at extreme
// progress values the art never partially overflows the viewport buffer.
// -------------------------------------------------------------------------
static int getSunCol(int vpWidth) {
    TimeOfDay tod = getTimeOfDay();
    int t = tickCount % CYCLE_LENGTH; // Current tick within the cycle
    int col;

    switch (tod) {
        case TIME_DAY: {
            // Progress through the daytime window: 0.0 at DAY_START, 1.0 at DUSK_START
            float p = (float)(t - DAY_START) / (DUSK_START - DAY_START);
            // Map to viewport range [15%, 85%]
            col = (int)(vpWidth * 0.15f + p * vpWidth * 0.7f);
            break;
        }
        case TIME_DUSK: {
            // Progress through dusk: 0.0 at DUSK_START, 1.0 at NIGHT_START
            float p = (float)(t - DUSK_START) / (NIGHT_START - DUSK_START);
            // Sun moves from 85% to 100% of viewport — sliding off the right
            col = (int)(vpWidth * 0.85f + p * vpWidth * 0.15f);
            break;
        }
        case TIME_DAWN: {
            // Progress through dawn: 0.0 at DAWN_START, 1.0 at CYCLE_LENGTH
            float p = (float)(t - DAWN_START) / (CYCLE_LENGTH - DAWN_START);
            // Sun climbs from left edge to 15% of viewport
            col = (int)(p * vpWidth * 0.15f);
            break;
        }
        default:
            return -100; // Sun is completely off-screen during nighttime
    }

    // Clamp so sun art (SUN_W = 5 chars wide) never renders past the viewport edge
    if (col < 0)           col = 0;
    if (col > vpWidth - 5) col = vpWidth - 5;
    return col;
}

// -------------------------------------------------------------------------
// getMoonCol
//
// Computes the viewport column for the moon, using the same linear
// interpolation approach as getSunCol() but with a different phase schedule:
//
//   TIME_NIGHT: moon rises from 15% to 85% of vpWidth across the night.
//
//   TIME_DAWN:  moon continues from 85% to 100% — setting toward the right.
//
//   TIME_DUSK:  moon is just beginning to appear at the left edge (0–15%).
//               This ensures a seamless transition as day becomes night.
//
//   TIME_DAY:   returns -100 — the moon is fully off-screen.
//
// Unlike getSunCol(), the moon column is not clamped here because the
// 3-wide moon art is less likely to overflow, and the dusk/dawn edge cases
// are handled by the art lookup returning 0 for out-of-range coordinates.
// -------------------------------------------------------------------------
static int getMoonCol(int vpWidth) {
    TimeOfDay tod = getTimeOfDay();
    int t = tickCount % CYCLE_LENGTH; // Current tick within the cycle

    switch (tod) {
        case TIME_NIGHT: {
            // Progress through full night: 0.0 at NIGHT_START, 1.0 at DAWN_START
            float p = (float)(t - NIGHT_START) / (DAWN_START - NIGHT_START);
            // Moon travels across most of the viewport
            return (int)(vpWidth * 0.15f + p * vpWidth * 0.7f);
        }
        case TIME_DAWN: {
            // Progress through dawn: moon continues setting toward the right
            float p = (float)(t - DAWN_START) / (CYCLE_LENGTH - DAWN_START);
            return (int)(vpWidth * 0.85f + p * vpWidth * 0.15f);
        }
        case TIME_DUSK: {
            // Moon is just beginning to rise at dusk — starts near the left edge
            float p = (float)(t - DUSK_START) / (NIGHT_START - DUSK_START);
            return (int)(p * vpWidth * 0.15f);
        }
        default:
            return -100; // Moon is completely off-screen during daytime
    }
}

// =========================================================================
// SECTION 6 — ART LOOKUP + RENDER
// =========================================================================

// -------------------------------------------------------------------------
// getSunChar
//
// Given the current sky row (skyRow), the viewport column being rendered
// (vx), and the computed sun centre column (sunCx), determines which
// character from SUN_ART should appear at this cell.
//
// The sun art is 5 wide × 3 tall and is centred horizontally on sunCx.
// Vertically it occupies sky rows 1–3 (ly = skyRow - 1, so ly is 0-based
// within the art).
//
// Returns 0 if the cell is outside the bounding box, indicating the caller
// should fall through to the next rendering layer (moon / star / plain sky).
// -------------------------------------------------------------------------
static char getSunChar(int skyRow, int vx, int sunCx) {
    // Translate viewport-x into an x index local to the sun art
    // The art is centred on sunCx, so its left edge is at (sunCx - SUN_W/2)
    int lx = vx - (sunCx - SUN_W / 2);

    // Translate skyRow into a y index local to the sun art
    // The art occupies rows 1-3, so subtract 1 to get 0-based art row
    int ly = skyRow - 1;

    // Bounds-check: return 0 (no character) if outside the art rectangle
    if (ly < 0 || ly >= SUN_H || lx < 0 || lx >= SUN_W) return 0;

    // Look up and return the character in the art array at (ly, lx)
    return SUN_ART[ly][lx];
}

// -------------------------------------------------------------------------
// getMoonChar
//
// Same logic as getSunChar() but for the 3×3 MOON_ART array.
// Returns 0 if the cell is outside the moon's bounding box.
// -------------------------------------------------------------------------
static char getMoonChar(int skyRow, int vx, int moonCx) {
    // Translate viewport-x into a local art x index
    int lx = vx - (moonCx - MOON_W / 2);

    // Translate skyRow into a local art y index
    int ly = skyRow - 1;

    // Return 0 if the coordinate is outside the 3×3 art rectangle
    if (ly < 0 || ly >= MOON_H || lx < 0 || lx >= MOON_W) return 0;

    // Look up and return the moon art character at (ly, lx)
    return MOON_ART[ly][lx];
}

// -------------------------------------------------------------------------
// isStarAt
//
// Deterministic per-cell hash that decides whether a star should appear at
// (row, col).  Approximately 1 in 12 cells will be a star (the hash maps to
// a 32-bit integer and returns true only when (h % 12) == 0).
//
// The hash is seeded purely by (row, col), so the same sky cell always
// produces the same result regardless of tickCount.  This keeps the star
// field perfectly stable — stars do not twinkle or flicker between frames.
//
// The two XOR-shift steps and the multiplicative mix (0x5bd1e995 is the
// constant from the MurmurHash2 algorithm) ensure good bit avalanche so
// that nearby cells produce very different outputs.
// -------------------------------------------------------------------------
static bool isStarAt(int row, int col) {
    // Combine row and col with large primes to avoid alignment patterns
    unsigned int h = (unsigned int)(row * 7919 + col * 6271 + 1031);
    // Avalanche bits downward to mix high-bit differences into low bits
    h ^= h >> 13;
    // Multiply by a near-prime to spread bits across the full 32-bit range
    h *= 0x5bd1e995;
    // Final avalanche pass to remove any remaining low-bit bias
    h ^= h >> 15;
    // One in twelve cells is a star; the modulus gives a ~8.3% density
    return (h % 12) == 0;
}

// -------------------------------------------------------------------------
// renderSkyCell  (PUBLIC)
//
// The main sky rendering function, called by fog_of_war.cpp once for every
// sky cell in the viewport each frame.  Composes the full ANSI escape
// sequence for the cell and writes it into the caller's buffer.
//
// Parameters:
//   skyRow  — world-space Y row (used for sun/moon art row lookup)
//   vx      — viewport X column (used for sun/moon art column lookup and stars)
//   vpWidth — total viewport width (used to compute sun/moon X positions)
//   buf     — write destination inside the caller's pre-allocated render buffer
//
// Returns:
//   The number of bytes written to buf.  The caller adds this to its
//   running buffer position to advance past the bytes just written.
//
// Priority order (first match wins and the function returns immediately):
//   1. Sun glyph   — bright yellow foreground on sky background
//   2. Moon glyph  — bright white  foreground on sky background
//   3. Star        — white (night) or dim grey (dawn/dusk) on sky background
//   4. Plain sky   — single space with sky background colour only
//
// All branches end with \033[0m to reset colours so that the next cell
// (which may be a solid terrain block) is not tinted by the sky background.
// -------------------------------------------------------------------------
int renderSkyCell(int skyRow, int vx, int vpWidth, char* buf) {
    // Cache the current phase to avoid repeated getTimeOfDay() calls
    TimeOfDay tod = getTimeOfDay();

    // Retrieve the ANSI background escape for the current phase
    const char* bg = getSkyBg();

    // Compute the current horizontal column for both celestial bodies
    int sunCx  = getSunCol(vpWidth);
    int moonCx = getMoonCol(vpWidth);

    // ── Priority 1: Sun ──────────────────────────────────────────────────
    // Look up the sun art character for this (skyRow, vx) coordinate.
    // Space characters are transparent — they fall through to the next layer.
    char sc = getSunChar(skyRow, vx, sunCx);
    if (sc && sc != ' ') {
        // Emit: sky background + bright yellow foreground + sun glyph + reset
        return sprintf(buf, "%s\033[38;5;226m%c\033[0m", bg, sc);
    }

    // ── Priority 2: Moon ─────────────────────────────────────────────────
    // Look up the moon art character; same space-transparency rule applies.
    char mc = getMoonChar(skyRow, vx, moonCx);
    if (mc && mc != ' ') {
        // Emit: sky background + bright white foreground + moon glyph + reset
        return sprintf(buf, "%s\033[38;5;255m%c\033[0m", bg, mc);
    }

    // ── Priority 3: Stars ────────────────────────────────────────────────
    // Stars are only shown outside of full daytime, and only at cells where
    // the deterministic hash says a star should appear.
    if (tod != TIME_DAY && isStarAt(skyRow, vx)) {
        // Derive the star glyph from a simple hash of the coordinates so
        // each star position always shows the same glyph (no flickering).
        unsigned int h = (unsigned int)(skyRow * 31 + vx * 17);
        char star = STAR_CHARS[h % 4]; // One of: . + * `

        // Stars are brighter at night and dimmer (grey) during dawn/dusk
        const char* col = (tod == TIME_NIGHT) ? "\033[38;5;255m"   // bright white
                                              : "\033[38;5;244m";  // medium grey
        // Emit: sky background + chosen star colour + glyph + reset
        return sprintf(buf, "%s%s%c\033[0m", bg, col, star);
    }

    // ── Priority 4: Empty sky ────────────────────────────────────────────
    // No celestial body or star at this cell — just fill with the sky colour.
    return sprintf(buf, "%s \033[0m", bg);
}
