// =============================================================================
// final_fight.cpp
// TermiCraft — Final Boss Fight Module Implementation
//
// Full Space Invaders-style dragon boss fight.
//
// Rendering:  ANSI escape codes + colors.h (consistent with rest of codebase).
//             A static char buffer is rebuilt every tick and flushed in one
//             cout pass to prevent flicker.
// Input:      Non-blocking via fcntl(O_NONBLOCK) + read(). Saved/restored
//             around the fight loop so the rest of the game is unaffected.
// Scoring:    state.score incremented in real time.
//             Every add: state.score += (int)(rawPts * state.settings.scoreMultiplier)
// HP:         Local fight HP separate from state.player.health.
//             Formula: FF_BASE_HP_* (by difficulty) + flat armor bonus.
// High score: Uses fileio's addHighScore() and getTopHighScore().
//             Stores top 10. menu.h's getPlayerName() handles the name prompt.
// Phases:     Dragon phases change at 66 % and 33 % HP remaining.
//             Visual damage: char-swap in the art array at each threshold.
//             Dragon speed increases by 1 col/tick per phase above Phase 1.
// Fireballs:  Phase 1 = 1 straight, Phase 2 = 2 spread, Phase 3 = 3 spread.
//             Outer fireballs drift ±1 col/tick as they fall.
// Arrows:     Rapid fire — up to FF_MAX_ARROWS active simultaneously.
//
// Author:       Sohan
// Dependencies: final_fight.h, types.h, fileio.h, menu.h, colors.h
//               <fcntl.h>, <unistd.h>, <cstring>, <cstdlib>, <ctime>
// =============================================================================

#include "final_fight.h"
#include "score.h"        // addScore(), saveFinalScore() — centralised scoring
#include "fileio.h"       // getTopHighScore() used in showScoreBreakdown()
#include "menu.h"         // getPlayerName(), waitForKeypress()
#include "colors.h"       // COLOR_* defines, clearScreen()

#include <iostream>
#include <iomanip>        // std::setw
#include <sstream>
#include <cstring>        // strlen, snprintf
#include <cstdlib>        // rand, srand
#include <ctime>          // time
#include <algorithm>      // std::max, std::min
#include <fcntl.h>        // fcntl, F_GETFL, F_SETFL, O_NONBLOCK
#include <unistd.h>       // usleep, read, STDIN_FILENO

//using namespace std;
// =============================================================================
// Dragon ASCII art — 3 phase variants
//
// The dragon is drawn as a 12-line block. Only two lines differ between phases:
//   Line 3 (eyes):  "@ @" (P1) → "x @" (P2) → "x x" (P3)
//   Line 5 (mouth): "\VV/"     → "\VV/"      → "\XX/"
//
// The entire block shifts horizontally each tick to animate movement.
// Every backslash in the art is doubled (\\) as required by C strings.
// =============================================================================

// Phase 1 — full health, 100 % to 67 %: eyes @ @, mouth \VV/
static const char* DRAGON_P1[FF_DRAGON_ROWS] = {
    "        ,     \\    /      ,        ",
    "       / \\    )\\__/(     / \\       ",
    "      /   \\  (_\\  /_)   /   \\      ",
    " ____/_____\\__\\@ @/___/_____\\____  ",
    "|             |\\../|              |",
    "|              \\VV/               |",
    "|        ----------------         |",
    "|_________________________________|",
    " |    /\\ /      \\\\       \\ /\\    |",
    " |  /   V        ))       V   \\  |",
    " |/     `       //        '     \\|",
    " `              V                `"
};

// Phase 2 — damaged, 66 % to 34 %: one eye becomes 'x', mouth still \VV/
static const char* DRAGON_P2[FF_DRAGON_ROWS] = {
    "        ,     \\    /      ,        ",
    "       / \\    )\\__/(     / \\       ",
    "      /   \\  (_\\  /_)   /   \\      ",
    " ____/_____\\__\\x @/___/_____\\____  ",
    "|             |\\../|              |",
    "|              \\VV/               |",
    "|        ----------------         |",
    "|_________________________________|",
    " |    /\\ /      \\\\       \\ /\\    |",
    " |  /   V        ))       V   \\  |",
    " |/     `       //        '     \\|",
    " `              V                `"
};

// Phase 3 — near death, 33 % to 0 %: both eyes 'x', mouth breaks to \XX/
static const char* DRAGON_P3[FF_DRAGON_ROWS] = {
    "        ,     \\    /      ,        ",
    "       / \\    )\\__/(     / \\       ",
    "      /   \\  (_\\  /_)   /   \\      ",
    " ____/_____\\__\\x x/___/_____\\____  ",
    "|             |\\../|              |",
    "|              \\XX/               |",
    "|        ----------------         |",
    "|_________________________________|",
    " |    /\\ /      \\\\       \\ /\\    |",
    " |  /   V        ))       V   \\  |",
    " |/     `       //        '     \\|",
    " `              V                `"
};

// Lookup table: DRAGON_ART[phase-1][row] → the correct line string
static const char** DRAGON_ART[3] = {
    (const char**)DRAGON_P1,
    (const char**)DRAGON_P2,
    (const char**)DRAGON_P3
};

// =============================================================================
// Rendering buffer
// =============================================================================

// One fixed-width char buffer for the entire arena, rebuilt every tick.
// Declared static so it is not reallocated on the stack each frame.
static char screenBuf[FF_ARENA_HEIGHT][FF_ARENA_WIDTH + 1];

/*
 * safeSet
 * Sets one character in the screen buffer, bounds-checked.
 * Never overwrites the left border (col 0) or right border (col FF_ARENA_WIDTH-1).
 *
 * Inputs:  r - row index, c - column index, ch - character to place
 * Outputs: none (modifies screenBuf in place)
 */
static void safeSet(int r, int c, char ch) {
    if (r >= 0 && r < FF_ARENA_HEIGHT && c > 0 && c < FF_ARENA_WIDTH - 1)
        screenBuf[r][c] = ch;
}

/*
 * safeSetStr
 * Copies a C-string into the buffer starting at (r, c).
 * Stops at null terminator, right border, or end of buffer — whichever first.
 * Never overwrites border columns (col 0 or col FF_ARENA_WIDTH-1).
 *
 * Inputs:  r - row index, c - start column, str - null-terminated string
 * Outputs: none (modifies screenBuf in place)
 */
static void safeSetStr(int r, int c, const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        int col = c + i;
        if (col <= 0 || col >= FF_ARENA_WIDTH - 1) continue;
        screenBuf[r][col] = str[i];
    }
}

/*
 * buildBaseBuffer
 * Resets screenBuf to a blank arena shell: space-filled interior, side borders,
 * horizontal border rows, and the static controls text.
 * Must be called at the start of every renderArena() call.
 *
 * Inputs:  none
 * Outputs: none (rebuilds screenBuf from scratch)
 */
static void buildBaseBuffer() {
    // Fill every row: spaces interior, '|' borders, null terminator
    for (int r = 0; r < FF_ARENA_HEIGHT; r++) {
        for (int c = 0; c < FF_ARENA_WIDTH; c++)
            screenBuf[r][c] = ' ';
        screenBuf[r][0]               = '|';
        screenBuf[r][FF_ARENA_WIDTH-1] = '|';
        screenBuf[r][FF_ARENA_WIDTH]  = '\0';
    }

    // Horizontal border rows (overwrite the '|' borders with '+' corners)
    int borderRows[] = { 0, FF_HUD_SEP_ROW, FF_CTRL_SEP_ROW, FF_BOT_ROW };
    for (int br = 0; br < 4; br++) {
        int r = borderRows[br];
        for (int c = 0; c < FF_ARENA_WIDTH; c++)
            screenBuf[r][c] = '=';
        screenBuf[r][0]               = '+';
        screenBuf[r][FF_ARENA_WIDTH-1] = '+';
        screenBuf[r][FF_ARENA_WIDTH]  = '\0';
    }

    // Static controls hint (row FF_CTRL_ROW)
    safeSetStr(FF_CTRL_ROW, 2,
        "[A] move left   [D] move right   [SPACE] shoot   [Q] quit");

    // Blank safety row at the bottom
    for (int c = 0; c <= FF_ARENA_WIDTH; c++)
        screenBuf[FF_ARENA_HEIGHT-1][c] = '\0';
}

/*
 * buildHPBar
 * Builds an ASCII HP bar string, e.g. "[||||.....] 23/50".
 *
 * Inputs:  current  - current HP value
 *          maximum  - maximum HP value
 *          barWidth - number of characters inside the brackets
 * Outputs: std::string containing "[bar] current/maximum"
 */
static string buildHPBar(int current, int maximum, int barWidth) {
    if (maximum <= 0) maximum = 1;
    if (current < 0)  current = 0;
    int filled = current * barWidth / maximum;
    if (filled > barWidth) filled = barWidth;

    string bar = "[";
    for (int i = 0; i < barWidth; i++)
        bar += (i < filled ? '|' : '.');
    bar += "] ";

    char nums[24];
    snprintf(nums, sizeof(nums), "%d/%d", current, maximum);
    bar += nums;
    return bar;
}

/*
 * buildHUDRow
 * Writes the HUD content (score left, dragon HP bar right) into screenBuf row 1.
 *
 * Inputs:  currentScore - live score to display
 *          dragon       - current dragon state (for HP bar and percentage)
 * Outputs: none (modifies screenBuf[FF_HUD_ROW])
 */
static void buildHUDRow(int currentScore, const Dragon& dragon) {
    const int BAR_W = 18;
    int pct    = (dragon.maxHp > 0) ? (dragon.hp * 100 / dragon.maxHp) : 0;
    int filled = (dragon.maxHp > 0) ? (dragon.hp * BAR_W / dragon.maxHp) : 0;

    // Left: title + score
    char left[56];
    snprintf(left, sizeof(left), " TERMICRAFT: THE LAIR   SCORE: %06d", currentScore);

    // Right: dragon HP bar
    string barStr;
    for (int i = 0; i < BAR_W; i++) barStr += (i < filled ? '|' : '.');
    char right[40];
    snprintf(right, sizeof(right), "DRAGON:[%s]%3d%%", barStr.c_str(), pct);

    // Calculate padding so right section is flush with the right border
    int leftLen  = (int)strlen(left);
    int rightLen = (int)strlen(right);
    int innerW   = FF_ARENA_WIDTH - 2;      // cols 1-98
    int pad      = innerW - leftLen - rightLen - 1;
    if (pad < 1) pad = 1;

    safeSetStr(FF_HUD_ROW, 1, left);
    safeSetStr(FF_HUD_ROW, 1 + leftLen + pad, right);
}

/*
 * buildPlayerHPRow
 * Writes the player HP bar into screenBuf row FF_HP_ROW.
 *
 * Inputs:  playerHp    - current fight HP
 *          playerMaxHp - starting fight HP
 *          armorName   - display name for equipped armor
 * Outputs: none (modifies screenBuf[FF_HP_ROW])
 */
static void buildPlayerHPRow(int playerHp, int playerMaxHp, const char* armorName) {
    string bar = buildHPBar(playerHp, playerMaxHp, 28);
    char line[80];
    snprintf(line, sizeof(line), " PLAYER HP: %s  [%s]", bar.c_str(), armorName);
    safeSetStr(FF_HP_ROW, 1, line);
}

/*
 * renderArena
 * Rebuilds and flushes the full arena to the terminal each game tick.
 * Uses ANSI \033[H to overwrite the screen without clearing (no flicker).
 * Dragon rows are printed in COLOR_DRAGON; player sprite in COLOR_PLAYER.
 *
 * Inputs:  dragon, fireballs, arrows, playerX,
 *          playerHp, playerMaxHp, currentScore, armorName
 * Outputs: none (writes to stdout)
 */
static void renderArena(const Dragon&   dragon,
                        const Fireball* fireballs,
                        const Arrow*    arrows,
                        int             playerX,
                        int             playerHp,
                        int             playerMaxHp,
                        int             currentScore,
                        const char*     armorName) {
    buildBaseBuffer();

    // HUD
    buildHUDRow(currentScore, dragon);

    // Dragon art block: 12 lines placed at dragon.x offset
    const char** art = DRAGON_ART[dragon.phase - 1];
    for (int r = 0; r < FF_DRAGON_ROWS; r++)
        safeSetStr(FF_DRAGON_TOP_ROW + r, dragon.x, art[r]);

    // Fireballs: rendered as '*'
    for (int i = 0; i < FF_MAX_FIREBALLS; i++) {
        if (fireballs[i].active)
            safeSet(fireballs[i].y, fireballs[i].x, '*');
    }

    // Arrows: rendered as '|'
    for (int i = 0; i < FF_MAX_ARROWS; i++) {
        if (arrows[i].active)
            safeSet(arrows[i].y, arrows[i].x, '|');
    }

    // Player sprite "[/\ &&&&& /\]" (13 chars, centred on playerX)
    const char* sprite   = "[/\\ &&&&& /\\]";
    const int   spriteLen = 13;
    const int   spriteStart = playerX - spriteLen / 2;   // = playerX - 6

    for (int i = 0; i < spriteLen; i++)
        safeSet(FF_PLAYER_ROW, spriteStart + i, sprite[i]);

    // Player HP bar
    buildPlayerHPRow(playerHp, playerMaxHp, armorName);

    // --- Flush to terminal ---
    // Move cursor to top-left without clearing (prevents flicker)
    cout << CURSOR_HOME;

    for (int r = 0; r < FF_ARENA_HEIGHT; r++) {
        if (r >= FF_DRAGON_TOP_ROW && r < FF_DRAGON_TOP_ROW + FF_DRAGON_ROWS) {
            // Dragon rows: entire row in dragon red
            cout << COLOR_DRAGON << screenBuf[r] << COLOR_RESET << "\n";

        } else if (r == FF_PLAYER_ROW) {
            // Player row: colour only the sprite, leave rest of row normal
            string rowStr(screenBuf[r]);
            int sStart = max(1, spriteStart);
            int sEnd   = sStart + spriteLen;
            cout << rowStr.substr(0, sStart)
                 << COLOR_PLAYER
                 << rowStr.substr(sStart, sEnd - sStart)
                 << COLOR_RESET
                 << rowStr.substr(sEnd) << "\n";

        } else {
            cout << screenBuf[r] << "\n";
        }
    }
    cout.flush();
}

// =============================================================================
// Public helper functions
// =============================================================================

/*
 * initBossConfig — see final_fight.h for full documentation
 */
BossConfig initBossConfig(Difficulty diff) {
    BossConfig cfg;
    switch (diff) {
        case DIFF_EASY:
            cfg.dragonHp      = 50;
            cfg.fireballDmg   = 2;
            cfg.fireRateTicks = 40;    // ~2.0 s at 50 ms/tick
            cfg.dragonSpeed   = 1;
            break;
        case DIFF_NORMAL:
            cfg.dragonHp      = 80;
            cfg.fireballDmg   = 5;
            cfg.fireRateTicks = 24;    // ~1.2 s
            cfg.dragonSpeed   = 2;
            break;
        case DIFF_HARD:
        default:
            cfg.dragonHp      = 120;
            cfg.fireballDmg   = 10;
            cfg.fireRateTicks = 14;    // ~0.7 s
            cfg.dragonSpeed   = 3;
            break;
    }
    return cfg;
}

/*
 * calcFightHP — see final_fight.h for full documentation
 */
int calcFightHP(Difficulty diff, MaterialTier armor) {
    // Base HP by difficulty
    int base;
    switch (diff) {
        case DIFF_EASY:   base = FF_BASE_HP_EASY;   break;
        case DIFF_HARD:   base = FF_BASE_HP_HARD;   break;
        case DIFF_NORMAL:
        default:          base = FF_BASE_HP_NORMAL;  break;
    }

    // Flat armor bonus — identical on every difficulty
    int bonus;
    switch (armor) {
        case MATERIAL_STONE:   bonus = FF_HP_BONUS_STONE;   break;
        case MATERIAL_IRON:    bonus = FF_HP_BONUS_IRON;    break;
        case MATERIAL_GOLD:    bonus = FF_HP_BONUS_GOLD;    break;
        case MATERIAL_DIAMOND: bonus = FF_HP_BONUS_DIAMOND; break;
        default:               bonus = 0;                   break;
    }

    return base + bonus;
}

/*
 * calcArmorDamage — see final_fight.h for full documentation
 */
int calcArmorDamage(MaterialTier armor) {
    switch (armor) {
        case MATERIAL_IRON:    return FF_DMG_IRON;
        case MATERIAL_GOLD:    return FF_DMG_GOLD;
        case MATERIAL_DIAMOND: return FF_DMG_DIAMOND;
        default:               return FF_DMG_DEFAULT;  // NONE, WOOD, STONE
    }
}

// =============================================================================
// Private (static) helper functions
// =============================================================================

/*
 * getArmorName
 * Returns a short display string for a MaterialTier value.
 * Used in the arena HUD and intro screen.
 *
 * Inputs:  armor - MaterialTier enum value
 * Outputs: const char* — human-readable name
 */
static const char* getArmorName(MaterialTier armor) {
    switch (armor) {
        case MATERIAL_STONE:   return "Stone";
        case MATERIAL_IRON:    return "Iron";
        case MATERIAL_GOLD:    return "Gold";
        case MATERIAL_DIAMOND: return "Diamond";
        default:               return "None";
    }
}

/*
 * initDragon
 * Creates a Dragon centred at the top of the arena, moving right, Phase 1.
 *
 * Inputs:  config - BossConfig for this difficulty
 * Outputs: Dragon struct ready to enter the fight loop
 */
static Dragon initDragon(const BossConfig& config) {
    Dragon d;
    d.x         = (FF_ARENA_WIDTH - FF_DRAGON_COLS) / 2;   // ≈ col 32 (centred)
    d.y         = FF_DRAGON_TOP_ROW;
    d.hp        = config.dragonHp;
    d.maxHp     = config.dragonHp;
    d.speed     = config.dragonSpeed;
    d.direction = 1;       // start moving right
    d.phase     = FF_PHASE1;
    return d;
}

/*
 * updatePhase
 * Checks dragon HP against phase thresholds and upgrades phase if needed.
 * Speed increases by 1 col/tick at each phase transition above Phase 1.
 * Phase can only advance (1 → 2 → 3), never retreat.
 *
 * Inputs:  dragon    - current dragon state (modified in place)
 *          baseSpeed - config.dragonSpeed (reference for speed calculation)
 * Outputs: none
 */
static void updatePhase(Dragon& dragon, int baseSpeed) {
    if (dragon.hp <= 0) return;
    int pct = dragon.hp * 100 / dragon.maxHp;

    int newPhase;
    if      (pct <= FF_PHASE3_PCT) newPhase = FF_PHASE3;
    else if (pct <= FF_PHASE2_PCT) newPhase = FF_PHASE2;
    else                           newPhase = FF_PHASE1;

    if (newPhase > dragon.phase) {
        dragon.phase = newPhase;
        // Speed boost: +1 col/tick per phase above Phase 1
        dragon.speed = baseSpeed + (dragon.phase - 1);
    }
}

/*
 * spawnFireballs
 * Activates new fireballs from the first available slots in the pool.
 * Spawn position: horizontally centred on the dragon, just below its body.
 * Pattern per phase:
 *   Phase 1 — 1 fireball at centre, dx=0 (straight down)
 *   Phase 2 — 2 fireballs at centre ± FF_SPREAD_P2, dx = ∓1 (diverge as they fall)
 *   Phase 3 — 3 fireballs: centre-spread, centre, centre+spread; dx=-1, 0, +1
 *
 * Inputs:  fireballs - the projectile pool array (modified in place)
 *          dragon    - current dragon state (provides position and phase)
 * Outputs: none
 */
static void spawnFireballs(Fireball* fireballs, const Dragon& dragon) {
    // Horizontal centre of the dragon block
    int cx     = dragon.x + FF_DRAGON_COLS / 2;
    // Spawn just below the dragon body
    int spawnY = FF_DRAGON_TOP_ROW + FF_DRAGON_ROWS;

    // Build list of {x, dx} pairs for this volley
    int spawnX[3], spawnDX[3], spawnCount;

    if (dragon.phase == FF_PHASE1) {
        spawnX[0] = cx;               spawnDX[0] = 0;
        spawnCount = 1;
    } else if (dragon.phase == FF_PHASE2) {
        spawnX[0] = cx - FF_SPREAD_P2;  spawnDX[0] = -1;
        spawnX[1] = cx + FF_SPREAD_P2;  spawnDX[1] =  1;
        spawnCount = 2;
    } else {   // FF_PHASE3
        spawnX[0] = cx - FF_SPREAD_P3;  spawnDX[0] = -1;
        spawnX[1] = cx;                  spawnDX[1] =  0;
        spawnX[2] = cx + FF_SPREAD_P3;  spawnDX[2] =  1;
        spawnCount = 3;
    }

    // Find free slots and activate them
    int activated = 0;
    for (int i = 0; i < FF_MAX_FIREBALLS && activated < spawnCount; i++) {
        if (!fireballs[i].active) {
            fireballs[i].x      = spawnX[activated];
            fireballs[i].y      = spawnY;
            fireballs[i].dx     = spawnDX[activated];
            fireballs[i].active = true;
            activated++;
        }
    }
}

/*
 * fireArrow
 * Activates a new arrow from the first available slot in the pool.
 * Arrow spawns one row above the player sprite (FF_PLAYER_ROW - 1).
 * If all FF_MAX_ARROWS slots are already active, the shot is silently dropped.
 *
 * Inputs:  arrows  - the arrow pool array (modified in place)
 *          playerX - column to spawn the arrow at (the player's centre)
 * Outputs: none
 */
static void fireArrow(Arrow* arrows, int playerX) {
    for (int i = 0; i < FF_MAX_ARROWS; i++) {
        if (!arrows[i].active) {
            arrows[i].x      = playerX;
            arrows[i].y      = FF_PLAYER_ROW - 1;
            arrows[i].active = true;
            return;    // one arrow per SPACE press
        }
    }
    // All slots full — rapid fire cap reached, shot dropped silently
}

/*
 * showIntroScreen
 * Clears the screen and displays the cave entry lore + player stats.
 * Warns the player if they entered without diamond armor.
 * Calls waitForKeypress() before the fight begins.
 *
 * Inputs:  state       - game state (for armor and score)
 *          playerHp    - computed starting fight HP
 *          playerMaxHp - same as playerHp at call time (= full HP)
 * Outputs: none
 */
static void showIntroScreen(const GameState& state, int playerHp, int playerMaxHp) {
    clearScreen();
    cout << COLOR_DRAGON
         << "\n  ================================================================\n"
         << "                    ~  THE DRAGON CAVE  ~                       \n"
         << "  ================================================================\n"
         << COLOR_RESET << "\n"
         << "  The cavern floor trembles beneath your feet.\n"
         << "  A low rumble reverberates through the stone walls.\n"
         << "  Two burning eyes open in the darkness ahead...\n\n";

    cout << COLOR_SCORE << "  Your stats:\n" << COLOR_RESET
         << "    Armor:    " << COLOR_PLAYER
                             << getArmorName(state.player.equipment.armor)
                             << COLOR_RESET << "\n"
         << "    Fight HP: " << COLOR_HEALTH
                             << playerHp << " / " << playerMaxHp
                             << COLOR_RESET << "\n"
         << "    Score:    " << COLOR_SCORE
                             << state.score
                             << COLOR_RESET << "\n\n";

    if (state.player.equipment.armor < MATERIAL_DIAMOND) {
        cout << COLOR_WARNING
             << "  WARNING: Diamond armor is recommended for this fight.\n"
             << "           Entering with "
             << getArmorName(state.player.equipment.armor)
             << " armor — good luck.\n"
             << COLOR_RESET << "\n";
    } else {
        cout << COLOR_SUCCESS
             << "  You are fully equipped. The dragon awaits.\n"
             << COLOR_RESET << "\n";
    }

    cout << "  Controls: [A] left   [D] right   [SPACE] shoot   [Q] quit\n\n"
         << "  Press any key to begin...\n";

    waitForKeypress();
    clearScreen();
}

/*
 * showScoreBreakdown
 * Displays the end-of-fight score breakdown: mining score carried in,
 * boss hits per phase with multiplier applied, kill bonus, and final total.
 * Also signals a new high score if state.score beats the stored record.
 *
 * Inputs:  won           - true = dragon defeated, false = player died
 *          miningSnapshot - state.score at the moment the fight started
 *          p1/p2/p3Hits  - arrow hits landed in each phase
 *          killBonus     - true if dragon was defeated (500 pt bonus applied)
 *          totalScore    - state.score after the fight
 *          scoreMult     - state.settings.scoreMultiplier
 * Outputs: none
 */
static void showScoreBreakdown(bool won,
                                int  miningSnapshot,
                                int  p1Hits,
                                int  p2Hits,
                                int  p3Hits,
                                bool killBonus,
                                int  totalScore,
                                float scoreMult) {
    clearScreen();

    if (won) {
        cout << COLOR_SUCCESS
             << "\n  *** THE DRAGON FALLS!  YOU ARE VICTORIOUS! ***\n\n"
             << COLOR_RESET;
    } else {
        cout << COLOR_DANGER
             << "\n  *** YOU HAVE FALLEN IN THE DRAGON'S LAIR... ***\n\n"
             << COLOR_RESET;
    }

    cout << COLOR_SCORE
         << "  ============= SCORE BREAKDOWN =============\n\n"
         << COLOR_RESET;

    cout << "  Mining score (carried in):    " << setw(8) << miningSnapshot << "\n\n"
         << "  Boss fight:\n";

    // Show each phase's contribution: raw pts, then multiplied
    if (p1Hits > 0) {
        int raw = p1Hits * FF_SCORE_HIT_P1;
        cout << "    Phase 1 hits: " << setw(3) << p1Hits
             << " x " << setw(2) << FF_SCORE_HIT_P1
             << " pts = " << setw(5) << raw
             << "  (x" << scoreMult << " = "
             << (int)(raw * scoreMult) << ")\n";
    }
    if (p2Hits > 0) {
        int raw = p2Hits * FF_SCORE_HIT_P2;
        cout << "    Phase 2 hits: " << setw(3) << p2Hits
             << " x " << setw(2) << FF_SCORE_HIT_P2
             << " pts = " << setw(5) << raw
             << "  (x" << scoreMult << " = "
             << (int)(raw * scoreMult) << ")\n";
    }
    if (p3Hits > 0) {
        int raw = p3Hits * FF_SCORE_HIT_P3;
        cout << "    Phase 3 hits: " << setw(3) << p3Hits
             << " x " << setw(2) << FF_SCORE_HIT_P3
             << " pts = " << setw(5) << raw
             << "  (x" << scoreMult << " = "
             << (int)(raw * scoreMult) << ")\n";
    }
    if (killBonus) {
        cout << "    Dragon kill bonus:             " << setw(5) << FF_SCORE_KILL
             << "  (x" << scoreMult << " = "
             << (int)(FF_SCORE_KILL * scoreMult) << ")\n";
    }

    int bossScore = totalScore - miningSnapshot;
    cout << "\n    Boss fight total:         " << setw(8) << bossScore << "\n\n";

    cout << COLOR_SCORE
         << "  TOTAL SCORE:              " << setw(10) << totalScore << "\n"
         << COLOR_RESET;

    // New high score notice
    HighScore existing = getTopHighScore();
    if (totalScore > existing.score) {
        cout << COLOR_SUCCESS << "\n  ** NEW HIGH SCORE! **\n" << COLOR_RESET;
    }

    cout << "\n";
    waitForKeypress();
}

// score.cpp owns the high score saving functionality.


