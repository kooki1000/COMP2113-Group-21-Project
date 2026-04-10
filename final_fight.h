// =============================================================================
// final_fight.h
// TermiCraft — Final Boss Fight Module Header
//
// Declares all constants, structs, and functions for the Space Invaders-style
// dragon boss fight. Fully integrated with the team's shared types.h, fileio.h,
// menu.h, and colors.h. Does NOT redefine Difficulty, MaterialTier, HighScore,
// or HIGHSCORE_FILE — all are taken from the team's existing headers.
//
// Arena:     100 x 35 terminal columns/rows
// Rendering: ANSI escape codes via colors.h (no ncurses, no extra install)
// Input:     Non-blocking via fcntl(O_NONBLOCK) + read() from <fcntl.h>/<unistd.h>
// Score:     Added to state.score in real time, every increment multiplied by
//            state.settings.scoreMultiplier (from types.h DifficultySettings)
//
// Author:       Sohan
// Dependencies: types.h, fileio.h, menu.h, colors.h
//               <fcntl.h>, <unistd.h> — standard on Linux, no extra install
// =============================================================================

#ifndef FINAL_FIGHT_H
#define FINAL_FIGHT_H

#include "types.h"   // GameState, Difficulty, MaterialTier — use team's definitions

// -----------------------------------------------------------------------------
// Arena dimensions
// -----------------------------------------------------------------------------
#define FF_ARENA_WIDTH     100   // total terminal columns
#define FF_ARENA_HEIGHT     35   // total terminal rows

// -----------------------------------------------------------------------------
// Dragon art dimensions
// -----------------------------------------------------------------------------
#define FF_DRAGON_ROWS      12   // lines in the ASCII dragon block
#define FF_DRAGON_COLS      35   // width of the widest dragon line

// -----------------------------------------------------------------------------
// Dragon movement bounds
// Left edge min = 2  (clears left border at col 0)
// Left edge max = 63 (right edge = 63+35 = 98, border at col 99)
// -----------------------------------------------------------------------------
#define FF_DRAGON_MIN_X      2
#define FF_DRAGON_MAX_X     63

// -----------------------------------------------------------------------------
// Arena row layout  (rows 0-33 active; row 34 is a blank safety buffer)
// -----------------------------------------------------------------------------
#define FF_HUD_ROW           1   // score + dragon HP bar
#define FF_HUD_SEP_ROW       2   // separator under HUD
#define FF_DRAGON_TOP_ROW    3   // first row of dragon art
#define FF_PLAYER_ROW       29   // row the player sprite sits on
#define FF_HP_ROW           30   // player HP bar display
#define FF_CTRL_SEP_ROW     31   // separator above controls
#define FF_CTRL_ROW         32   // controls hint line
#define FF_BOT_ROW          33   // bottom border

// -----------------------------------------------------------------------------
// Projectile pool sizes
// -----------------------------------------------------------------------------
#define FF_MAX_FIREBALLS    12   // maximum simultaneous fireballs
#define FF_MAX_ARROWS       10   // maximum simultaneous arrows (rapid fire)

// -----------------------------------------------------------------------------
// Boss fight base HP (local to this fight, independent of state.player.health)
// Formula: calcFightHP(difficulty, armor) = baseHP + armorBonus
// Base HP per difficulty:
// -----------------------------------------------------------------------------
#define FF_BASE_HP_EASY     40
#define FF_BASE_HP_NORMAL   30
#define FF_BASE_HP_HARD     20

// Flat armor HP bonus added on top of base — same value on all difficulties
#define FF_HP_BONUS_STONE    8
#define FF_HP_BONUS_IRON    15
#define FF_HP_BONUS_GOLD    22
#define FF_HP_BONUS_DIAMOND 30

// -----------------------------------------------------------------------------
// Arrow damage to dragon per hit, by MaterialTier (from types.h)
// MATERIAL_NONE=0, MATERIAL_WOOD=1, MATERIAL_STONE=2 → 1 dmg
// MATERIAL_IRON=3 → 3, MATERIAL_GOLD=4 → 5, MATERIAL_DIAMOND=5 → 8
// -----------------------------------------------------------------------------
#define FF_DMG_DEFAULT   1
#define FF_DMG_IRON      3
#define FF_DMG_GOLD      5
#define FF_DMG_DIAMOND   8

// -----------------------------------------------------------------------------
// Phase thresholds (% of dragon.maxHp remaining when phase transition triggers)
// -----------------------------------------------------------------------------
#define FF_PHASE2_PCT    66   // Phase 2 triggers at or below 66 %
#define FF_PHASE3_PCT    33   // Phase 3 triggers at or below 33 %

#define FF_PHASE1         1
#define FF_PHASE2         2
#define FF_PHASE3         3

// -----------------------------------------------------------------------------
// Score per arrow hit per phase — RAW value before scoreMultiplier
// state.settings.scoreMultiplier (1.0 / 1.5 / 2.0) is applied on every add
// -----------------------------------------------------------------------------
#define FF_SCORE_HIT_P1  10
#define FF_SCORE_HIT_P2  20
#define FF_SCORE_HIT_P3  30
#define FF_SCORE_KILL   500   // bonus when dragon HP reaches 0

// Fireball horizontal spread offsets per phase
#define FF_SPREAD_P2      3   // phase 2: two fireballs at centre ± 3 cols
#define FF_SPREAD_P3      5   // phase 3: outer fireballs at centre ± 5 cols

// Game tick duration in microseconds (50 ms = 20 ticks per second)
#define FF_TICK_US     50000
#endif // FINAL_FIGHT_H
