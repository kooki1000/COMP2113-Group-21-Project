// =============================================================================
// final_fight.h
// TermiCraft — Final Boss Fight Module Header
//
// Declares the boss-fight constants, structs, and functions used by the
// ncurses-based dragon encounter implemented in final_fight.cpp.
//
// Rendering: ncurses in the .cpp implementation (this header only declares API)
// Input:     read by the fight loop in final_fight.cpp using ncurses getch()
// Score:     Added to state.score in real time via score.cpp's addScore()
//
// Author:       Sohan
// Dependencies: types.h, fileio.h, colors.h
// =============================================================================

#ifndef FINAL_FIGHT_H
#define FINAL_FIGHT_H

#include "types.h"   // GameState, Difficulty, MaterialTier — use team's definitions

// -----------------------------------------------------------------------------
// Arena dimensions used by the fight logic and layout comments
// -----------------------------------------------------------------------------
#define FF_ARENA_WIDTH     100   // total terminal columns
#define FF_ARENA_HEIGHT     35   // total terminal rows

// -----------------------------------------------------------------------------
// Dragon art dimensions
// -----------------------------------------------------------------------------
#define FF_DRAGON_ROWS      12   // lines in the ASCII dragon block
#define FF_DRAGON_COLS      35   // width of the widest dragon line

// -----------------------------------------------------------------------------
// Dragon movement bounds used by the ncurses renderer
// Left edge min = 2  (clears the left border)
// Left edge max = 63 (keeps the 35-col art inside the 100-col arena)
// -----------------------------------------------------------------------------
#define FF_DRAGON_MIN_X      2
#define FF_DRAGON_MAX_X     63

// -----------------------------------------------------------------------------
// Arena row layout used by the ncurses fight screen
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
// -----------------------------------------------------------------------------
#define FF_BASE_HP_EASY     20
#define FF_BASE_HP_NORMAL   15
#define FF_BASE_HP_HARD     10

// Armor HP bonuses — higher tiers give the player more breathing room
#define FF_HP_BONUS_STONE   15
#define FF_HP_BONUS_IRON    30
#define FF_HP_BONUS_GOLD    50
#define FF_HP_BONUS_DIAMOND 80

// Arrow damage per hit — better armor = bigger payoff
#define FF_DMG_DEFAULT   1
#define FF_DMG_IRON      4
#define FF_DMG_GOLD      7
#define FF_DMG_DIAMOND  12

// -----------------------------------------------------------------------------
// Phase thresholds (% of dragon.maxHp remaining when phase transition triggers)
// -----------------------------------------------------------------------------
#define FF_PHASE2_PCT    66   // Phase 2 triggers at or below 66 %
#define FF_PHASE3_PCT    33   // Phase 3 triggers at or below 33 %

#define FF_PHASE1         1
#define FF_PHASE2         2
#define FF_PHASE3         3

// -----------------------------------------------------------------------------
// Score per arrow hit per phase — raw values before scoreMultiplier
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

// -----------------------------------------------------------------------------
// Structs  (HighScore, Difficulty, MaterialTier already in types.h)
// -----------------------------------------------------------------------------

/*
 * BossConfig
 *
 * Difficulty-specific parameters for the boss fight.
 * Built once by initBossConfig() at fight start and then treated as read-only.
 */
struct BossConfig {
    int  dragonHp;        // dragon starting HP
    int  fireballDmg;     // HP removed from player per fireball hit
    int  fireRateTicks;   // ticks between fireball volleys
    int  dragonSpeed;     // base columns dragon moves per tick
    bool hasEnrage;       // hard: speed + rate boost when HP drops to 50%
    int  spread3;         // phase-3 fireball outer spread (wider on hard)
};

/*
 * Dragon
 *
 * Live state of the dragon enemy throughout the fight.
 * The ASCII art moves as one block; x changes per tick and y stays anchored
 * at the top of the dragon art area.
 */
struct Dragon {
    int  x, y;
    int  hp, maxHp;
    int  speed, direction, phase;
    bool enraged;       // hard: triggers at 50% HP — speed + fire rate boost
    int  announceTicks; // ticks left to flash a phase-change / enrage banner
};

/*
 * Fireball
 *
 * One downward projectile fired by the dragon.
 * The fireballs[] pool recycles inactive slots without reallocating.
 */
struct Fireball {
    int  x;          // current column
    int  y;          // current row
    bool active;     // true = in flight and should be rendered + checked
    int  dx;         // horizontal drift per tick: 0 (straight), -1 or +1 (spread)
};

/*
 * Arrow
 *
 * One upward projectile fired by the player.
 * Rapid fire: multiple arrows can be active simultaneously (up to FF_MAX_ARROWS).
 */
struct Arrow {
    int  x;          // current column
    int  y;          // current row
    bool active;     // true = in flight
};

// -----------------------------------------------------------------------------
// Public function declarations
// -----------------------------------------------------------------------------

/*
 * runBossFight
 *
 * Main entry point. Called from main.cpp when the player enters the dragon cave.
 * Manages the intro screen, the full ncurses fight loop, the score breakdown
 * screen, and the final score save step.
 *
 * Score is added to state.score in real time throughout the fight via addScore().
 *
 * Inputs:  state — full GameState (reads: score, difficulty, settings, armor)
 *                                 (writes: score, phase, dragonDefeated)
 * Outputs: true  = dragon defeated → state.phase set to PHASE_VICTORY
 *          false = player died, fled, or the terminal was too small
 *                  → state.phase set to PHASE_GAMEOVER
 */
bool runBossFight(GameState& state);

/*
 * initBossConfig
 *
 * Builds the difficulty-specific BossConfig from a Difficulty enum value.
 *
 * Inputs:  diff — DIFF_EASY, DIFF_NORMAL, or DIFF_HARD (from types.h)
 * Outputs: BossConfig fully populated for that difficulty
 *
 * Tick timing reference (FF_TICK_US = 50 ms per tick):
 *   Easy:   fireRateTicks=35 ≈ 1.75 s between volleys, dragonSpeed=1
 *   Normal: fireRateTicks=18 ≈ 0.9 s,                  dragonSpeed=2
 *   Hard:   fireRateTicks=12 ≈ 0.6 s,                  dragonSpeed=3
 */
BossConfig initBossConfig(Difficulty diff);

/*
 * calcFightHP
 *
 * Computes the player's starting HP for the boss fight.
 * Formula: baseHP (from difficulty) + flat armorBonus.
 * No cap — armor always adds on top regardless of difficulty.
 *
 * Inputs:
 *   diff  — DIFF_EASY / DIFF_NORMAL / DIFF_HARD (base: 20 / 15 / 10)
 *   armor — MaterialTier from types.h (MATERIAL_NONE through MATERIAL_DIAMOND)
 * Outputs: int — total starting HP
 *
 * Examples:
 *   calcFightHP(DIFF_EASY,   MATERIAL_DIAMOND) → 20 + 80 = 100
 *   calcFightHP(DIFF_NORMAL, MATERIAL_GOLD)    → 15 + 50 = 65
 *   calcFightHP(DIFF_HARD,   MATERIAL_IRON)     → 10 + 30 = 40
 *   calcFightHP(DIFF_HARD,   MATERIAL_NONE)     → 10 +  0 = 10
 */
int calcFightHP(Difficulty diff, MaterialTier armor);

/*
 * calcArmorDamage
 *
 * Returns damage dealt to the dragon per arrow hit, based on armor tier.
 *
 * Inputs:  armor — MaterialTier (MATERIAL_NONE through MATERIAL_DIAMOND)
 * Outputs: int — damage per hit
 *            NONE / WOOD / STONE → 1
 *            IRON                → 4
 *            GOLD                → 7
 *            DIAMOND             → 12
 */
int calcArmorDamage(MaterialTier armor);

#endif // FINAL_FIGHT_H
