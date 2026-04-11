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


