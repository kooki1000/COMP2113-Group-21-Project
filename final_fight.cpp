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

using namespace std;
