// =============================================================================
// final_fight.cpp
// TermiCraft — Final Boss Fight Module Implementation
//
// Full Space Invaders-style dragon boss fight rendered entirely in the terminal
// using ncurses for flicker-free output and dynamic terminal size adaptation.
//
// Rendering:   ncurses (initscr/endwin). Layout adapts to terminal size via
//              computeLayout() which is re-run on every KEY_RESIZE event.
//              All rendering uses ncurses color pairs (CP_* constants) defined
//              at the top of this file and registered by initFightColors().
// Input:       ncurses nodelay(TRUE) so getch() never blocks the game loop.
//              The full key buffer is drained each tick so move + shoot can
//              happen in the same frame. KEY_RESIZE is handled inline.
// Scoring:     state.score incremented in real time via addScore() (score.h).
//              Every add: (int)(rawPts * state.settings.scoreMultiplier).
//              Phase hits tracked separately (p1h/p2h/p3h) for the breakdown.
// HP:          Local fight HP — separate from state.player.health.
//              Formula: FF_BASE_HP_* (by difficulty) + flat armor bonus.
//              Incoming fireball damage is also reduced by armor percentage
//              (10/20/30/45 % for Stone/Iron/Gold/Diamond).
// Opening:     50-tick locked animation with expanding shockwave and roar
//              damage applied on tick 1. Player can flee with Q.
// Phases:      Dragon transitions at 66 % and 33 % HP remaining.
//              Visual damage: char-swap in the art array at each threshold.
//              Speed increases by 1 col/tick per phase above Phase 1.
//              Hard mode adds an enrage trigger at 50 % HP (speed + fire boost).
// Fireballs:   Phase 1 = 1 straight, Phase 2 = 2 spread, Phase 3 = 3 spread.
//              Outer fireballs drift ±1 col/tick as they fall.
// Arrows:      Rapid fire — up to FF_MAX_ARROWS active simultaneously.
// High score:  saveFinalScore() in score.cpp handles name + leaderboard save.
//              showScoreBreakdown() uses ANSI output (ncurses torn down first).
//
// Author:       Sohan
// Dependencies: final_fight.h, score.h, fileio.h, colors.h
//               <ncurses.h> — pre-installed on Ubuntu/HKU CS academy server
//               <unistd.h>, <locale.h> — standard Linux headers, no install
// =============================================================================

#include "final_fight.h"    // structs, constants, public declarations
#include "score.h"           // addScore(), saveFinalScore() — centralised scoring
#include "fileio.h"          // getTopHighScore() — used in showScoreBreakdown()
#include "colors.h"          // ANSI color defines — used in showScoreBreakdown() (post-ncurses)

// ncurses must come BEFORE any header that declares getch() with a different signature
#include <ncurses.h>         // full ncurses API — rendering, input, color pairs
#include <algorithm>         // std::max, std::min
#include <string>            // std::string
#include <cstring>           // strlen — for centring strings in ncurses rows
#include <cstdlib>           // (reserved for future rand use)
#include <cstdio>            // std::snprintf — safe formatted string building
#include <sstream>           // std::ostringstream — score breakdown formatting
#include <iomanip>           // std::setw — score table column alignment
#include <unistd.h>          // usleep() — tick timing, read() — post-fight keypress
#include <locale.h>          // setlocale(LC_ALL,"") — enables UTF-8 in ncurses

// ── Color pair IDs ────────────────────────────────────────────────────────
#define CP_BORDER   1
#define CP_TITLE    2
#define CP_HUD      3
#define CP_DRAG_P1  4
#define CP_DRAG_P2  5
#define CP_DRAG_P3  6
#define CP_PLAYER   7
#define CP_FIRE     8
#define CP_ARROW    9
#define CP_HP_G    10
#define CP_HP_Y    11
#define CP_HP_R    12
#define CP_DIM     13
#define CP_WARN    14

/*
 * initFightColors
 * Registers all ncurses color pairs used by the boss fight renderer.
 * Must be called once after initscr() and before any rendering function.
 * Uses use_default_colors() so the terminal background is preserved (-1).
 *
 * Inputs:  none
 * Outputs: none (side effect: 14 ncurses color pairs registered globally)
 *
 * Color pair IDs are defined as CP_* constants at the top of this file.
 * Pairs CP_HP_G / CP_HP_Y / CP_HP_R are used by drawBar() to colour-shift
 * the HP bar from green → yellow → red as HP falls below 50 % and 20 %.
 */
static void initFightColors() {
    start_color();
    use_default_colors();
    init_pair(CP_BORDER,  COLOR_CYAN,    -1);
    init_pair(CP_TITLE,   COLOR_WHITE,   -1);
    init_pair(CP_HUD,     COLOR_YELLOW,  -1);
    init_pair(CP_DRAG_P1, COLOR_GREEN,   -1);
    init_pair(CP_DRAG_P2, COLOR_YELLOW,  -1);
    init_pair(CP_DRAG_P3, COLOR_RED,     -1);
    init_pair(CP_PLAYER,  COLOR_CYAN,    -1);
    init_pair(CP_FIRE,    COLOR_RED,     -1);
    init_pair(CP_ARROW,   COLOR_WHITE,   -1);
    init_pair(CP_HP_G,    COLOR_GREEN,   -1);
    init_pair(CP_HP_Y,    COLOR_YELLOW,  -1);
    init_pair(CP_HP_R,    COLOR_RED,     -1);
    init_pair(CP_DIM,     COLOR_WHITE,   -1);
    init_pair(CP_WARN,    COLOR_RED,     -1);
}

// =============================================================================
// Dragon ASCII art — 3 phase variants
//
// The dragon is drawn as a 12-line block. Only two lines differ per phase:
//   Line 3 (eyes):  "@ @" (Phase 1) → "x @" (Phase 2) → "x x" (Phase 3)
//   Line 5 (mouth): "\VV/"           → "\VV/"           → "\XX/"
//
// The entire block shifts horizontally each tick to animate movement —
// no wing animation, the body moves as one rigid unit.
// Every backslash in the art is doubled (\\) as required by C strings.
//
// DRAGON_ART[phase-1][row] gives the correct line for any phase and row.
// Phase transition is handled by renderDragon() which reads dragon.phase
// and selects the corresponding array automatically.
// =============================================================================
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
static const char** DRAGON_ART[3] = {
    (const char**)DRAGON_P1,
    (const char**)DRAGON_P2,
    (const char**)DRAGON_P3
};

// =============================================================================
// Layout globals — computed by computeLayout() from live terminal dimensions
//
// All render functions read these globals instead of hardcoded row/col values
// so the arena adapts to any terminal size. computeLayout() is called once at
// fight start and again on KEY_RESIZE events during the game loop.
// NC_ROWS / NC_COLS mirror ncurses getmaxyx(stdscr) results.
// =============================================================================
static int NC_ROWS, NC_COLS;
static int ROW_HUD, ROW_DRAG_HP, ROW_TOP_SEP;
static int ROW_DRAG_START, ROW_DRAG_END;
static int ROW_PLAYER, ROW_PLR_HP, ROW_BOT_SEP, ROW_CTRL;
static int DRAG_MIN_X, DRAG_MAX_X;

/*
 * computeLayout
 * Reads the current terminal dimensions via ncurses getmaxyx() and calculates
 * all dynamic row/column layout globals used by every render function.
 * Called once at fight start and again on KEY_RESIZE so the arena adapts
 * if the player resizes the terminal mid-fight.
 *
 * Inputs:  none (reads stdscr dimensions internally)
 * Outputs: none (side effect: sets NC_ROWS, NC_COLS, ROW_HUD, ROW_DRAG_HP,
 *          ROW_TOP_SEP, ROW_DRAG_START, ROW_DRAG_END, ROW_CTRL, ROW_BOT_SEP,
 *          ROW_PLR_HP, ROW_PLAYER, DRAG_MIN_X, DRAG_MAX_X)
 *
 * Layout anchors from the bottom so the player row and HP bar are always
 * visible regardless of terminal height.
 */
static void computeLayout() {
    getmaxyx(stdscr, NC_ROWS, NC_COLS);
    ROW_HUD        = 1;
    ROW_DRAG_HP    = 2;
    ROW_TOP_SEP    = 3;
    ROW_DRAG_START = 4;
    ROW_DRAG_END   = ROW_DRAG_START + FF_DRAGON_ROWS - 1;
    ROW_CTRL       = NC_ROWS - 2;
    ROW_BOT_SEP    = NC_ROWS - 3;
    ROW_PLR_HP     = NC_ROWS - 4;
    ROW_PLAYER     = NC_ROWS - 5;
    DRAG_MIN_X     = 1;
    DRAG_MAX_X     = NC_COLS - FF_DRAGON_COLS - 3;
    if (DRAG_MAX_X < DRAG_MIN_X + 4) DRAG_MAX_X = DRAG_MIN_X + 4;
}

// ── Drawing primitives ────────────────────────────────────────────────────

/*
 * drawFrame
 * Draws the arena border and the two internal horizontal separators using
 * ncurses box-drawing characters (ACS_HLINE, ACS_LTEE, ACS_RTEE).
 * The top separator divides the HUD from the combat zone.
 * The bottom separator divides the combat zone from the controls row.
 *
 * Inputs:  none (uses NC_COLS, ROW_TOP_SEP, ROW_BOT_SEP layout globals)
 * Outputs: none (side effect: draws to stdscr, requires refresh() to display)
 */
static void drawFrame() {
    attron(COLOR_PAIR(CP_BORDER) | A_BOLD);
    box(stdscr, 0, 0);
    mvhline(ROW_TOP_SEP, 1, ACS_HLINE, NC_COLS - 2);
    mvaddch(ROW_TOP_SEP, 0,          ACS_LTEE);
    mvaddch(ROW_TOP_SEP, NC_COLS-1,  ACS_RTEE);
    mvhline(ROW_BOT_SEP, 1, ACS_HLINE, NC_COLS - 2);
    mvaddch(ROW_BOT_SEP, 0,          ACS_LTEE);
    mvaddch(ROW_BOT_SEP, NC_COLS-1,  ACS_RTEE);
    attroff(COLOR_PAIR(CP_BORDER) | A_BOLD);
}

/*
 * drawBar
 * Draws a filled HP bar of a given width at a specific screen position.
 * Filled cells use A_REVERSE (solid block appearance); empty cells show '-'.
 * Color shifts automatically: green > 50 %, yellow > 20 %, red <= 20 %.
 *
 * Inputs:
 *   row   - ncurses row to draw on
 *   col   - ncurses column of the bar's left edge
 *   width - total number of characters the bar spans
 *   cur   - current value (HP remaining)
 *   maxV  - maximum value (starting HP); clamped to 1 if 0 to avoid division
 * Outputs: none (side effect: draws to stdscr, requires refresh() to display)
 */
static void drawBar(int row, int col, int width, int cur, int maxV) {
    if (maxV <= 0) maxV = 1;
    if (cur < 0)  cur  = 0;
    int filled = cur * width / maxV;
    int pct    = cur * 100 / maxV;
    int cpair  = (pct > 50) ? CP_HP_G : (pct > 20) ? CP_HP_Y : CP_HP_R;
    for (int i = 0; i < width; i++) {
        if (i < filled) {
            attron(COLOR_PAIR(cpair) | A_BOLD | A_REVERSE);
            mvaddch(row, col + i, ' ');
            attroff(COLOR_PAIR(cpair) | A_BOLD | A_REVERSE);
        } else {
            attron(COLOR_PAIR(CP_DIM) | A_DIM);
            mvaddch(row, col + i, '-');
            attroff(COLOR_PAIR(CP_DIM) | A_DIM);
        }
    }
}

// ── HUD ───────────────────────────────────────────────────────────────────

/*
 * renderHUD
 * Renders the top two rows of the arena HUD:
 *   Row 1 — title (left), current phase label (centre), live score (right)
 *   Row 2 — "DRAGON" label, dragon HP bar, percentage + exact HP numbers
 * Phase label colour changes per phase: green (I), yellow (II), red (III).
 * Dragon HP bar width scales with terminal width (NC_COLS / 2 - 16).
 *
 * Inputs:
 *   score  - current state.score to display
 *   dragon - current Dragon struct (reads hp, maxHp, phase)
 * Outputs: none (side effect: draws to stdscr rows ROW_HUD and ROW_DRAG_HP)
 */
static void renderHUD(int score, const Dragon& dragon) {
    // Left: title
    attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvprintw(ROW_HUD, 2, " TERMICRAFT: THE LAIR ");
    attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

    // Centre: phase
    const char* phaseStr;
    int phaseCp;
    if      (dragon.phase == FF_PHASE1) { phaseStr = "[ PHASE I ]";   phaseCp = CP_DRAG_P1; }
    else if (dragon.phase == FF_PHASE2) { phaseStr = "[ PHASE II ]";  phaseCp = CP_DRAG_P2; }
    else                                { phaseStr = "[ PHASE III ]"; phaseCp = CP_DRAG_P3; }
    int midCol = (NC_COLS - (int)strlen(phaseStr)) / 2;
    attron(COLOR_PAIR(phaseCp) | A_BOLD);
    mvprintw(ROW_HUD, midCol, "%s", phaseStr);
    attroff(COLOR_PAIR(phaseCp) | A_BOLD);

    // Right: score
    char scoreBuf[24];
    std::snprintf(scoreBuf, sizeof(scoreBuf), "SCORE: %06d ", score);
    attron(COLOR_PAIR(CP_HUD) | A_BOLD);
    mvprintw(ROW_HUD, NC_COLS - (int)strlen(scoreBuf) - 1, "%s", scoreBuf);
    attroff(COLOR_PAIR(CP_HUD) | A_BOLD);

    // Dragon HP bar
    int barW = std::max(8, NC_COLS / 2 - 16);
    attron(COLOR_PAIR(CP_HUD) | A_BOLD);
    mvprintw(ROW_DRAG_HP, 2, "DRAGON");
    attroff(COLOR_PAIR(CP_HUD) | A_BOLD);
    drawBar(ROW_DRAG_HP, 9, barW, dragon.hp, dragon.maxHp);
    int pct = (dragon.maxHp > 0) ? (dragon.hp * 100 / dragon.maxHp) : 0;
    attron(COLOR_PAIR(CP_HUD));
    mvprintw(ROW_DRAG_HP, 9 + barW + 1, " %3d%%  HP: %d/%d", pct, dragon.hp, dragon.maxHp);
    attroff(COLOR_PAIR(CP_HUD));
}

// ── Dragon ────────────────────────────────────────────────────────────────

/*
 * renderDragon
 * Renders the 12-line dragon ASCII art block at its current x position.
 * Selects the art array for the current phase via DRAGON_ART[phase-1].
 * Color shifts per phase: green (I), yellow (II), red (III).
 * Phase 3 adds A_BLINK on alternating tick pairs for a frantic visual.
 * On a hit (hitFlash=true) the entire block is rendered in A_REVERSE for
 * 4 ticks to give clear hit feedback to the player.
 * Lines that would overlap the player row are clipped silently.
 *
 * Inputs:
 *   dragon   - current Dragon struct (reads x, phase)
 *   tick     - current game tick counter (used for blink timing)
 *   hitFlash - true for 4 ticks after an arrow hits the dragon
 * Outputs: none (side effect: draws to stdscr rows ROW_DRAG_START onward)
 */
static void renderDragon(const Dragon& dragon, int tick, bool hitFlash) {
    const char** art = DRAGON_ART[dragon.phase - 1];
    int cpair = (dragon.phase == FF_PHASE1) ? CP_DRAG_P1 :
                (dragon.phase == FF_PHASE2) ? CP_DRAG_P2 : CP_DRAG_P3;
    int attrs = A_BOLD;
    if (dragon.phase == FF_PHASE3 && (tick / 2) % 2 == 0) attrs |= A_BLINK;

    for (int r = 0; r < FF_DRAGON_ROWS; r++) {
        int srow = ROW_DRAG_START + r;
        if (srow >= ROW_PLAYER - 2) break;
        int scol = 1 + dragon.x;
        if (hitFlash) attron(A_REVERSE | A_BOLD);
        else          attron(COLOR_PAIR(cpair) | attrs);
        const char* line = art[r];
        for (int c = 0; line[c] && scol + c < NC_COLS - 1; c++)
            mvaddch(srow, scol + c, (unsigned char)line[c]);
        if (hitFlash) attroff(A_REVERSE | A_BOLD);
        else          attroff(COLOR_PAIR(cpair) | attrs);
    }
}

// ── Projectiles ───────────────────────────────────────────────────────────

/*
 * renderProjectiles
 * Renders all active fireballs ('*', red) and arrows ('^', white).
 * Projectiles are skipped if they sit on or above ROW_TOP_SEP or on/below
 * ROW_PLAYER, keeping them inside the combat zone only.
 * Column offset of +1 is applied to account for the left border character.
 *
 * Inputs:
 *   fbs    - fireball pool array (FF_MAX_FIREBALLS entries)
 *   arrows - arrow pool array (FF_MAX_ARROWS entries)
 * Outputs: none (side effect: draws to stdscr combat zone rows)
 */
static void renderProjectiles(const Fireball* fbs, const Arrow* arrows) {
    attron(COLOR_PAIR(CP_FIRE) | A_BOLD);
    for (int i = 0; i < FF_MAX_FIREBALLS; i++) {
        if (!fbs[i].active) continue;
        if (fbs[i].y <= ROW_TOP_SEP || fbs[i].y >= ROW_PLAYER) continue;
        if (fbs[i].x > 0 && fbs[i].x < NC_COLS - 2)
            mvaddch(fbs[i].y, 1 + fbs[i].x, '*');
    }
    attroff(COLOR_PAIR(CP_FIRE) | A_BOLD);

    attron(COLOR_PAIR(CP_ARROW) | A_BOLD);
    for (int i = 0; i < FF_MAX_ARROWS; i++) {
        if (!arrows[i].active) continue;
        if (arrows[i].y <= ROW_TOP_SEP || arrows[i].y >= ROW_PLAYER) continue;
        if (arrows[i].x > 0 && arrows[i].x < NC_COLS - 2)
            mvaddch(arrows[i].y, 1 + arrows[i].x, '^');
    }
    attroff(COLOR_PAIR(CP_ARROW) | A_BOLD);
}

// ── Player ────────────────────────────────────────────────────────────────

/*
 * getArmorName
 * Returns a short display string for a MaterialTier value.
 * Used in the player HP bar label and the intro stats line.
 *
 * Inputs:  armor - MaterialTier enum value (from types.h)
 * Outputs: const char* — "Stone", "Iron", "Gold", "Diamond", or "None"
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
 * renderPlayer
 * Renders the player sprite "[/\=====/\]" at ROW_PLAYER and the player HP
 * bar + armor label at ROW_PLR_HP.
 * Sprite colour shifts to match HP percentage: cyan > 50 %, yellow > 20 %,
 * red <= 20 % — giving the player a visual cue that they are in danger.
 * The sprite is clamped so it never overlaps the left or right arena borders.
 *
 * Inputs:
 *   playerX    - centre column of the player sprite
 *   playerHp   - current fight HP
 *   playerMaxHp - starting fight HP (used for bar and percentage)
 *   armor       - MaterialTier (for color and label in HP row)
 * Outputs: none (side effect: draws to stdscr rows ROW_PLAYER and ROW_PLR_HP)
 */
static void renderPlayer(int playerX, int playerHp, int playerMaxHp, MaterialTier armor) {
    const char* sprite = "[/\\=====/\\]";
    int slen = (int)strlen(sprite);
    int scol = 1 + playerX - slen / 2;
    if (scol < 1)             scol = 1;
    if (scol + slen > NC_COLS - 1) scol = NC_COLS - 1 - slen;

    int pct   = (playerMaxHp > 0) ? (playerHp * 100 / playerMaxHp) : 0;
    int cpair = (pct > 50) ? CP_PLAYER : (pct > 20) ? CP_HP_Y : CP_HP_R;
    attron(COLOR_PAIR(cpair) | A_BOLD);
    mvprintw(ROW_PLAYER, scol, "%s", sprite);
    attroff(COLOR_PAIR(cpair) | A_BOLD);

    int barW = std::max(8, NC_COLS / 3);
    attron(COLOR_PAIR(CP_HUD) | A_BOLD);
    mvprintw(ROW_PLR_HP, 2, "PLAYER");
    attroff(COLOR_PAIR(CP_HUD) | A_BOLD);
    drawBar(ROW_PLR_HP, 9, barW, playerHp, playerMaxHp);
    attron(COLOR_PAIR(CP_HUD));
    mvprintw(ROW_PLR_HP, 9 + barW + 1, " %3d%%  [%s armor]", pct, getArmorName(armor));
    attroff(COLOR_PAIR(CP_HUD));
}

// ── Controls ──────────────────────────────────────────────────────────────

/*
 * renderControls
 * Renders the static controls hint on ROW_CTRL (below the bottom separator).
 * Displayed in dimmed white so it does not compete with the combat visuals.
 *
 * Inputs:  none (uses ROW_CTRL layout global)
 * Outputs: none (side effect: draws to stdscr row ROW_CTRL)
 */
static void renderControls() {
    attron(COLOR_PAIR(CP_DIM) | A_DIM);
    mvprintw(ROW_CTRL, 2,
        "[A] move left    [D] move right    [SPACE] shoot    [Q] flee");
    attroff(COLOR_PAIR(CP_DIM) | A_DIM);
}

// ── Full frame ────────────────────────────────────────────────────────────

/*
 * renderBanners
 * Renders a centred phase-change or enrage announcement banner in the combat
 * zone for dragon.announceTicks frames. The banner blinks every 5 ticks by
 * rendering only when (announceTicks / 5) % 2 == 0.
 * Content: "PHASE II", "PHASE III", or "DRAGON ENRAGED" depending on state.
 * Enrage banner uses CP_WARN (red); phase banners use CP_HUD (yellow).
 * Called from renderFrame() every tick; does nothing if announceTicks == 0.
 *
 * Inputs:  dragon - current Dragon struct (reads announceTicks, phase, enraged)
 * Outputs: none (side effect: may draw one banner row to stdscr combat zone)
 */
static void renderBanners(const Dragon& dragon) {
    // Phase / enrage announcement — flash for announceTicks frames
    if (dragon.announceTicks > 0) {
        bool blink = (dragon.announceTicks / 5) % 2 == 0;
        if (blink) {
            const char* msg = dragon.enraged      ? "  !! DRAGON ENRAGED !!  " :
                              (dragon.phase == FF_PHASE3) ? "  -- PHASE III --  "  :
                                                            "  -- PHASE II --  ";
            int cpair = dragon.enraged ? CP_WARN : CP_HUD;
            int msgRow = ROW_TOP_SEP + 1;
            int msgCol = (NC_COLS - (int)strlen(msg)) / 2;
            attron(COLOR_PAIR(cpair) | A_BOLD | A_REVERSE);
            mvprintw(msgRow, msgCol, "%s", msg);
            attroff(COLOR_PAIR(cpair) | A_BOLD | A_REVERSE);
        }
    }
}

/*
 * renderOpening
 * Renders one frame of the 50-tick locked entry animation shown before the
 * fight loop begins. Each frame shows an expanding shockwave ring of '*'
 * characters radiating outward from the arena centre, a dense 'W'/'*' cluster
 * at the centre for the first 10 frames, and a blinking "ROOAARRR!" message
 * showing the opening roar damage dealt to the player.
 * The player sprite and HP bar are always rendered so the HP drop is visible.
 * Player can press Q during this sequence to flee (sets running = false).
 *
 * Inputs:
 *   playerX      - player's current centre column
 *   playerHp     - player's current HP after roar damage
 *   playerMaxHp  - player's starting fight HP (for bar rendering)
 *   score        - current state.score (passed to renderHUD)
 *   dragon       - current Dragon struct (passed to renderHUD)
 *   armor        - player's armor tier (for renderPlayer label)
 *   openingTicks - remaining ticks in the opening sequence (0-49)
 *   roarDmg      - HP taken from opening roar (shown in message)
 * Outputs: none (side effect: draws one full frame to stdscr and calls refresh)
 */
static void renderOpening(int playerX, int playerHp, int playerMaxHp,
                          int score, const Dragon& dragon,
                          MaterialTier armor, int openingTicks, int roarDmg) {
    erase();
    drawFrame();
    renderHUD(score, dragon);

    int frame    = 50 - openingTicks;  // 0..49
    int combatTop = ROW_TOP_SEP + 1;
    int combatBot = ROW_PLAYER - 1;
    int centreRow = (combatTop + combatBot) / 2;
    int centreCol = NC_COLS / 2;

    // Expanding shockwave ring
    int radius = frame / 5;
    int maxR   = (combatBot - combatTop) / 2;
    if (radius > maxR) radius = maxR;

    if (radius > 0) {
        attron(COLOR_PAIR(CP_FIRE) | A_BOLD);
        int rTop  = centreRow - radius, rBot  = centreRow + radius;
        int cLeft = centreCol - radius * 2, cRight = centreCol + radius * 2;
        for (int c = cLeft; c <= cRight; c++) {
            if (rTop > ROW_TOP_SEP && rTop < ROW_PLAYER && c > 0 && c < NC_COLS-1)
                mvaddch(rTop, c, '*');
            if (rBot > ROW_TOP_SEP && rBot < ROW_PLAYER && c > 0 && c < NC_COLS-1)
                mvaddch(rBot, c, '*');
        }
        for (int r = rTop+1; r < rBot; r++) {
            if (r > ROW_TOP_SEP && r < ROW_PLAYER) {
                if (cLeft  > 0 && cLeft  < NC_COLS-1) mvaddch(r, cLeft,  '*');
                if (cRight > 0 && cRight < NC_COLS-1) mvaddch(r, cRight, '*');
            }
        }
        // Dense central cluster for first 10 frames
        if (frame < 10) {
            for (int dr = -1; dr <= 1; dr++)
                for (int dc = -2; dc <= 2; dc++) {
                    int rr = centreRow + dr, cc = centreCol + dc;
                    if (rr > ROW_TOP_SEP && rr < ROW_PLAYER && cc > 0 && cc < NC_COLS-1)
                        mvaddch(rr, cc, frame % 2 == 0 ? 'W' : '*');
                }
        }
        attroff(COLOR_PAIR(CP_FIRE) | A_BOLD);
    }

    // ROOAARRR message — blinks every 5 frames
    if ((frame / 5) % 2 == 0) {
        char roarMsg[64];
        std::snprintf(roarMsg, sizeof(roarMsg),
            "  ROOAARRR!  Dragon breathes fire!  -%d HP  ", roarDmg);
        int msgLen = (int)strlen(roarMsg);
        int msgRow = centreRow;
        int msgCol = std::max(1, (NC_COLS - msgLen) / 2);
        attron(COLOR_PAIR(CP_WARN) | A_BOLD | A_REVERSE);
        mvprintw(msgRow, msgCol, "%s", roarMsg);
        attroff(COLOR_PAIR(CP_WARN) | A_BOLD | A_REVERSE);

        const char* sub = "  BRACE FOR IMPACT  ";
        int subCol = std::max(1, (NC_COLS - (int)strlen(sub)) / 2);
        attron(COLOR_PAIR(CP_FIRE) | A_BOLD | A_BLINK);
        mvprintw(msgRow + 1, subCol, "%s", sub);
        attroff(COLOR_PAIR(CP_FIRE) | A_BOLD | A_BLINK);
    }

    renderPlayer(playerX, playerHp, playerMaxHp, armor);
    renderControls();
    attron(COLOR_PAIR(CP_DIM) | A_DIM);
    mvprintw(ROW_CTRL, 2, "[Q] flee");
    attroff(COLOR_PAIR(CP_DIM) | A_DIM);
    refresh();
}

/*
 * renderFrame
 * Composes and renders one complete game frame by calling all sub-renderers
 * in the correct z-order: frame → HUD → dragon → projectiles → player →
 * controls → banners. Calls ncurses erase() first to clear the previous frame,
 * then refresh() at the end to flush everything to the terminal in one pass.
 *
 * Inputs:
 *   dragon      - current Dragon struct
 *   fbs         - fireball pool array (FF_MAX_FIREBALLS entries)
 *   arrows      - arrow pool array (FF_MAX_ARROWS entries)
 *   playerX     - player centre column
 *   playerHp    - current player fight HP
 *   playerMaxHp - starting player fight HP
 *   score       - current state.score
 *   armor       - player's armor tier (for renderPlayer)
 *   tick        - current game tick (for renderDragon blink timing)
 *   hitFlash    - true for 4 ticks after an arrow hits (for renderDragon flash)
 * Outputs: none (side effect: full terminal repaint via ncurses refresh())
 */
static void renderFrame(const Dragon& dragon, const Fireball* fbs, const Arrow* arrows,
                        int playerX, int playerHp, int playerMaxHp,
                        int score, MaterialTier armor, int tick, bool hitFlash) {
    erase();
    drawFrame();
    renderHUD(score, dragon);
    renderDragon(dragon, tick, hitFlash);
    renderProjectiles(fbs, arrows);
    renderPlayer(playerX, playerHp, playerMaxHp, armor);
    renderControls();
    renderBanners(dragon);
    refresh();
}

// ── Intro screen ──────────────────────────────────────────────────────────

/*
 * showIntro
 * Displays a full-screen ncurses intro before the fight begins.
 * Shows: centred dragon art (Phase 1 colours), cave title, lore text,
 * player stats (armor, HP, score), and a "DIAMOND ARMOR RECOMMENDED" warning
 * if the player entered with less than MATERIAL_DIAMOND.
 * Blocks until the player presses any key (nodelay disabled for this call).
 *
 * Inputs:  state - full GameState (reads player.equipment.armor, player.health,
 *                  player.maxHealth, score)
 * Outputs: none (side effect: blocks on getch(), draws to stdscr)
 */
static void showIntro(const GameState& state) {
    erase();
    int artStart = (NC_ROWS / 2) - 9;
    if (artStart < 1) artStart = 1;
    int artWidth = (int)strlen(DRAGON_P1[0]);
    int artCol   = (NC_COLS - artWidth) / 2;
    if (artCol < 1) artCol = 1;

    attron(COLOR_PAIR(CP_DRAG_P3) | A_BOLD);
    for (int r = 0; r < FF_DRAGON_ROWS && artStart + r < NC_ROWS - 6; r++)
        mvprintw(artStart + r, artCol, "%s", DRAGON_P1[r]);
    attroff(COLOR_PAIR(CP_DRAG_P3) | A_BOLD);

    const char* title = "~ THE DRAGON CAVE ~";
    attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvprintw(artStart - 2, (NC_COLS - (int)strlen(title)) / 2, "%s", title);
    attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

    int loreRow = artStart + FF_DRAGON_ROWS + 1;
    const char* lore = "The cavern trembles. Two burning eyes open in the darkness...";
    attron(COLOR_PAIR(CP_HUD));
    mvprintw(loreRow, (NC_COLS - (int)strlen(lore)) / 2, "%s", lore);
    attroff(COLOR_PAIR(CP_HUD));

    char statBuf[80];
    std::snprintf(statBuf, sizeof(statBuf),
        "Armor: %s    HP: %d/%d    Score: %d",
        getArmorName(state.player.equipment.armor),
        state.player.health, state.player.maxHealth, state.score);
    attron(COLOR_PAIR(CP_PLAYER) | A_BOLD);
    mvprintw(loreRow + 2, (NC_COLS - (int)strlen(statBuf)) / 2, "%s", statBuf);
    attroff(COLOR_PAIR(CP_PLAYER) | A_BOLD);

    if (state.player.equipment.armor < MATERIAL_DIAMOND) {
        const char* warn = "DIAMOND ARMOR RECOMMENDED. GOOD LUCK SURVIVING.";
        attron(COLOR_PAIR(CP_WARN) | A_BOLD | A_REVERSE);
        mvprintw(loreRow + 5, (NC_COLS - (int)strlen(warn)) / 2, "%s", warn);
        attroff(COLOR_PAIR(CP_WARN) | A_BOLD | A_REVERSE);
    }

    const char* prompt = "Press any key to begin...";
    attron(COLOR_PAIR(CP_DIM) | A_DIM);
    mvprintw(NC_ROWS - 2, (NC_COLS - (int)strlen(prompt)) / 2, "%s", prompt);
    attroff(COLOR_PAIR(CP_DIM) | A_DIM);

    refresh();
    nodelay(stdscr, FALSE);
    getch();
    nodelay(stdscr, TRUE);
}

// ── Score breakdown (post-fight, ANSI — ncurses already torn down) ────────

/*
 * showScoreBreakdown
 * Displays the post-fight score breakdown screen using ANSI escape codes
 * (ncurses has already been torn down by the time this is called).
 * Shows a large ASCII art "YOU WIN" or "GAME OVER" banner, then a bordered
 * table with: mining score carried in, hits per phase with multiplied totals,
 * kill bonus (if applicable), and the final total. Flags a new high score
 * by comparing total against the current stored best (pre-save snapshot from
 * runBossFight — see race condition note in score.cpp).
 * Blocks on read() until the player presses any key.
 *
 * Inputs:
 *   won       - true = dragon defeated, false = player died
 *   miningSnap - state.score captured before the fight started
 *   p1h/p2h/p3h - arrow hits landed in each phase
 *   killBonus  - true if dragon was defeated (500 pt bonus was applied)
 *   total      - final state.score after the fight
 *   mult       - state.settings.scoreMultiplier (1.0 / 1.5 / 2.0)
 * Outputs: none (side effect: prints to stdout, blocks on keypress)
 */
static void showScoreBreakdown(bool won, int miningSnap, int p1h, int p2h, int p3h,
                                bool killBonus, int total, float mult) {
    std::cout << "\033[2J\033[H";  // clear screen
    if (won) {
        std::cout << "\033[1;32m\n"
            "   ██╗   ██╗ ██████╗ ██╗   ██╗    ██╗    ██╗██╗███╗   ██╗██╗\n"
            "   ╚██╗ ██╔╝██╔═══██╗██║   ██║    ██║    ██║██║████╗  ██║██║\n"
            "    ╚████╔╝ ██║   ██║██║   ██║    ██║ █╗ ██║██║██╔██╗ ██║██║\n"
            "     ╚██╔╝  ██║   ██║██║   ██║    ██║███╗██║██║██║╚██╗██║██║\n"
            "      ██║   ╚██████╔╝╚██████╔╝    ╚███╔███╔╝██║██║ ╚████║██║\n"
            "      ╚═╝    ╚═════╝  ╚═════╝      ╚══╝╚══╝ ╚═╝╚═╝  ╚═══╝╚═╝\n"
            "\033[0m\n";
    } else {
        std::cout << "\033[1;31m\n"
            "   ██████╗  █████╗ ███╗   ███╗███████╗     ██████╗ ██╗   ██╗███████╗██████╗\n"
            "  ██╔════╝ ██╔══██╗████╗ ████║██╔════╝    ██╔═══██╗██║   ██║██╔════╝██╔══██╗\n"
            "  ██║  ███╗███████║██╔████╔██║█████╗      ██║   ██║██║   ██║█████╗  ██████╔╝\n"
            "  ██║   ██║██╔══██║██║╚██╔╝██║██╔══╝      ██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██╗\n"
            "  ╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗    ╚██████╔╝ ╚████╔╝ ███████╗██║  ██║\n"
            "   ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝     ╚═════╝   ╚═══╝  ╚══════╝╚═╝  ╚═╝\n"
            "\033[0m\n";
    }

    std::cout << "\033[1;36m"
        "  ╔═══════════════════════════════════════╗\n"
        "  ║          SCORE  BREAKDOWN             ║\n"
        "  ╠═══════════════════════════════════════╣\n"
        "\033[0m";

    char buf[64];
    auto row = [&](const char* label, int val) {
        std::snprintf(buf, sizeof(buf), "  ║  %-28s %6d  ║\n", label, val);
        std::cout << buf;
    };
    row("Mining score (carried in):", miningSnap);
    if (p1h > 0) {
        char lbl[40];
        std::snprintf(lbl, sizeof(lbl), "Phase I   hits: %3d x %2d =", p1h, FF_SCORE_HIT_P1);
        row(lbl, (int)(p1h * FF_SCORE_HIT_P1 * mult));
    }
    if (p2h > 0) {
        char lbl[40];
        std::snprintf(lbl, sizeof(lbl), "Phase II  hits: %3d x %2d =", p2h, FF_SCORE_HIT_P2);
        row(lbl, (int)(p2h * FF_SCORE_HIT_P2 * mult));
    }
    if (p3h > 0) {
        char lbl[40];
        std::snprintf(lbl, sizeof(lbl), "Phase III hits: %3d x %2d =", p3h, FF_SCORE_HIT_P3);
        row(lbl, (int)(p3h * FF_SCORE_HIT_P3 * mult));
    }
    if (killBonus)
        row("Dragon kill bonus:", (int)(FF_SCORE_KILL * mult));

    std::cout << "\033[1;36m"
        "  ╠═══════════════════════════════════════╣\n"
        "\033[0m";
    std::snprintf(buf, sizeof(buf), "  ║  %-28s %6d  ║\n", "TOTAL SCORE:", total);
    std::cout << "\033[1;33m" << buf << "\033[0m";

    HighScore existing = getTopHighScore();
    if (total > existing.score)
        std::cout << "\033[1;32m  ║           ** NEW HIGH SCORE! **        ║\n\033[0m";

    std::cout << "\033[1;36m"
        "  ╚═══════════════════════════════════════╝\n"
        "\033[0m\n";

    std::cout << "\033[2m  Press any key to continue...\033[0m";
    std::cout.flush();
    char dummy; read(STDIN_FILENO, &dummy, 1);
}

// ── Game logic helpers ────────────────────────────────────────────────────

/*
 * initBossConfig
 * Builds the difficulty-specific BossConfig from a Difficulty enum value.
 * Hard mode enables hasEnrage (dragon speed + fire rate boost at 50 % HP)
 * and uses a wider Phase 3 fireball spread (spread3 = 8 vs FF_SPREAD_P3).
 *
 * Inputs:  diff — DIFF_EASY, DIFF_NORMAL, or DIFF_HARD (from types.h)
 * Outputs: BossConfig fully populated for that difficulty
 *
 * Tick timing reference (FF_TICK_US = 50 ms per tick):
 *   Easy:   fireRateTicks=35 ≈ 1.75 s, dragonSpeed=1, no enrage
 *   Normal: fireRateTicks=18 ≈ 0.9 s,  dragonSpeed=2, no enrage
 *   Hard:   fireRateTicks=12 ≈ 0.6 s,  dragonSpeed=3, enrage at 50 %
 */
BossConfig initBossConfig(Difficulty diff) {
    BossConfig cfg = {};
    switch (diff) {
        case DIFF_EASY:
            cfg.dragonHp = 50;  cfg.fireballDmg = 4;
            cfg.fireRateTicks = 35; cfg.dragonSpeed = 1;
            cfg.hasEnrage = false; cfg.spread3 = FF_SPREAD_P3;
            break;
        case DIFF_NORMAL:
            cfg.dragonHp = 80;  cfg.fireballDmg = 9;
            cfg.fireRateTicks = 18; cfg.dragonSpeed = 2;
            cfg.hasEnrage = false; cfg.spread3 = FF_SPREAD_P3;
            break;
        default:  // HARD
            cfg.dragonHp = 140; cfg.fireballDmg = 15;
            cfg.fireRateTicks = 12; cfg.dragonSpeed = 3;
            cfg.hasEnrage = true; cfg.spread3 = 8;
            break;
    }
    return cfg;
}

/*
 * calcFightHP
 * Computes the player's starting HP for the boss fight.
 * Formula: baseHP (from difficulty) + flat armorBonus.
 * There is no cap — armor always adds on top regardless of difficulty.
 * Base HP values are set low so entering without good armor is punishing.
 *
 * Inputs:
 *   diff  - DIFF_EASY / DIFF_NORMAL / DIFF_HARD
 *           (base HP: FF_BASE_HP_EASY / FF_BASE_HP_NORMAL / FF_BASE_HP_HARD)
 *   armor - MaterialTier from types.h (MATERIAL_NONE through MATERIAL_DIAMOND)
 * Outputs: int — total starting HP
 *
 * Examples:
 *   calcFightHP(DIFF_EASY,   MATERIAL_DIAMOND) → 20 + 80 = 100
 *   calcFightHP(DIFF_NORMAL, MATERIAL_GOLD)    → 15 + 50 = 65
 *   calcFightHP(DIFF_HARD,   MATERIAL_NONE)    → 10 +  0 = 10
 */
int calcFightHP(Difficulty diff, MaterialTier armor) {
    int base = (diff == DIFF_EASY) ? FF_BASE_HP_EASY :
               (diff == DIFF_HARD) ? FF_BASE_HP_HARD : FF_BASE_HP_NORMAL;
    int bonus = 0;
    switch (armor) {
        case MATERIAL_STONE:   bonus = FF_HP_BONUS_STONE;   break;
        case MATERIAL_IRON:    bonus = FF_HP_BONUS_IRON;    break;
        case MATERIAL_GOLD:    bonus = FF_HP_BONUS_GOLD;    break;
        case MATERIAL_DIAMOND: bonus = FF_HP_BONUS_DIAMOND; break;
        default: break;
    }
    return base + bonus;
}

/*
 * calcArmorDamage
 * Returns the damage dealt to the dragon per arrow hit, based on armor tier.
 * Better armor = stronger weapon. None/Wood/Stone deal the minimum 1 per hit,
 * making fights with lower-tier armor significantly longer.
 *
 * Inputs:  armor — MaterialTier (MATERIAL_NONE through MATERIAL_DIAMOND)
 * Outputs: int — damage per hit
 *            NONE / WOOD / STONE → FF_DMG_DEFAULT (1)
 *            IRON                → FF_DMG_IRON    (4)
 *            GOLD                → FF_DMG_GOLD    (7)
 *            DIAMOND             → FF_DMG_DIAMOND (12)
 */
int calcArmorDamage(MaterialTier armor) {
    switch (armor) {
        case MATERIAL_IRON:    return FF_DMG_IRON;
        case MATERIAL_GOLD:    return FF_DMG_GOLD;
        case MATERIAL_DIAMOND: return FF_DMG_DIAMOND;
        default:               return FF_DMG_DEFAULT;
    }
}

/*
 * initDragon
 * Creates and returns a Dragon struct ready to enter the fight loop.
 * Starting position: x = FF_DRAGON_COLS (one block-width from the left border),
 * y = ROW_DRAG_START (computed by computeLayout). Starts in Phase 1,
 * moving right, not enraged, with announceTicks = 0.
 *
 * Inputs:  cfg - BossConfig (reads dragonHp and dragonSpeed)
 * Outputs: Dragon struct with all fields initialised
 */
static Dragon initDragon(const BossConfig& cfg) {
    Dragon d = {};
    d.x = FF_DRAGON_COLS;
    d.y = ROW_DRAG_START;
    d.hp = cfg.dragonHp; d.maxHp = cfg.dragonHp;
    d.speed = cfg.dragonSpeed; d.direction = 1;
    d.phase = FF_PHASE1;
    d.enraged = false;
    d.announceTicks = 0;
    return d;
}

/*
 * updatePhase
 * Checks the dragon's current HP percentage against the phase thresholds
 * and advances the phase if a threshold has been crossed.
 * Phase can only increase (1 → 2 → 3), never retreat.
 * On each phase advance, dragon.speed is increased by 1 col/tick above base.
 * Does nothing if dragon.hp <= 0 (prevents spurious phase changes at death).
 *
 * Inputs:
 *   dragon    - Dragon struct (reads hp, maxHp, phase; writes phase, speed)
 *   baseSpeed - config.dragonSpeed from BossConfig (reference for speed calc)
 * Outputs: none (modifies dragon in place)
 */
static void updatePhase(Dragon& dragon, int baseSpeed) {
    if (dragon.hp <= 0) return;
    int pct = dragon.hp * 100 / dragon.maxHp;
    int newPhase = (pct <= FF_PHASE3_PCT) ? FF_PHASE3 :
                   (pct <= FF_PHASE2_PCT) ? FF_PHASE2 : FF_PHASE1;
    if (newPhase > dragon.phase) {
        dragon.phase = newPhase;
        dragon.speed = baseSpeed + (dragon.phase - 1);
    }
}

/*
 * spawnFireballs
 * Activates new fireballs from the first available inactive slots in the pool.
 * Spawn position: horizontally centred on the dragon block, one row below
 * the dragon's bottom edge (ROW_DRAG_END + 1).
 *
 * Pattern per phase:
 *   Phase 1 — 1 fireball at centre, dx=0 (straight down)
 *   Phase 2 — 2 fireballs at centre ± FF_SPREAD_P2, dx = ∓1 (diverging)
 *   Phase 3 — 3 fireballs: centre ± spread3, centre; dx = -1, 0, +1
 *             spread3 = FF_SPREAD_P3 on Easy/Normal, wider (8) on Hard
 *
 * If all FF_MAX_FIREBALLS slots are active, the volley is silently dropped.
 *
 * Inputs:
 *   fbs     - fireball pool array (modified in place)
 *   dragon  - current Dragon struct (reads x, phase)
 *   spread3 - outer spread columns for Phase 3 (from BossConfig.spread3)
 * Outputs: none (modifies fbs in place)
 */
static void spawnFireballs(Fireball* fbs, const Dragon& dragon, int spread3) {
    int cx     = dragon.x + FF_DRAGON_COLS / 2;
    int spawnY = ROW_DRAG_END + 1;
    int spawnX[3], spawnDX[3], cnt;

    if (dragon.phase == FF_PHASE1) {
        spawnX[0] = cx; spawnDX[0] = 0; cnt = 1;
    } else if (dragon.phase == FF_PHASE2) {
        spawnX[0] = cx - FF_SPREAD_P2; spawnDX[0] = -1;
        spawnX[1] = cx + FF_SPREAD_P2; spawnDX[1] =  1; cnt = 2;
    } else {
        spawnX[0] = cx - spread3; spawnDX[0] = -1;
        spawnX[1] = cx;           spawnDX[1] =  0;
        spawnX[2] = cx + spread3; spawnDX[2] =  1; cnt = 3;
    }
    int activated = 0;
    for (int i = 0; i < FF_MAX_FIREBALLS && activated < cnt; i++) {
        if (!fbs[i].active) {
            fbs[i] = {spawnX[activated], spawnY, true, spawnDX[activated]};
            activated++;
        }
    }
}

/*
 * fireArrow
 * Activates a new arrow from the first available inactive slot in the pool.
 * Arrow spawns one row above the player sprite (ROW_PLAYER - 1), centred
 * on playerX. Rapid fire is supported — up to FF_MAX_ARROWS simultaneously.
 * If all slots are active, the shot is silently dropped (no feedback needed
 * as the player can see arrows on screen).
 *
 * Inputs:
 *   arrows  - arrow pool array (modified in place)
 *   playerX - column to spawn the arrow at (player's centre)
 * Outputs: none (modifies arrows in place)
 */
static void fireArrow(Arrow* arrows, int playerX) {
    for (int i = 0; i < FF_MAX_ARROWS; i++) {
        if (!arrows[i].active) {
            arrows[i] = {playerX, ROW_PLAYER - 1, true};
            return;
        }
    }
}

// ── runBossFight — main entry point ───────────────────────────────────────

/*
 * runBossFight
 * Main entry point for the entire boss fight sequence. Called from main.cpp
 * when the player enters the dragon cave portal.
 *
 * Sequence:
 *   1. Initialise ncurses (initscr, colors, nodelay, curs_set)
 *   2. Compute dynamic layout (computeLayout)
 *   3. Check minimum terminal size — returns false immediately if too small
 *   4. Build BossConfig, Dragon, projectile pools from GameState
 *   5. Calculate opening roar damage (scales with difficulty, reduced by armor)
 *   6. Run 50-tick locked opening animation (renderOpening)
 *   7. Run the main fight loop until dragon.hp == 0 or playerHp == 0
 *      — Input: drains full key buffer each tick (move + shoot same tick)
 *      — Dragon movement: bounces off DRAG_MIN_X / DRAG_MAX_X walls
 *      — Fireballs: spawn on interval, move 1 row down + dx drift per tick
 *      — Arrows: move 1 row up per tick, deactivated above ROW_TOP_SEP
 *      — Collisions: arrows vs dragon hitbox, fireballs vs player hitbox
 *      — Score: addScore() called on every hit and on kill bonus
 *      — Phase: updatePhase() called every tick
 *      — Enrage (Hard only): triggers once at 50 % HP — speed + fire rate boost
 *   8. endwin() — restore terminal before ANSI output
 *   9. Set state.dragonDefeated and state.phase before returning
 *  10. showScoreBreakdown() — post-fight ANSI score table
 *  11. saveFinalScore() — name + leaderboard save via score.cpp / fileio.cpp
 *
 * Fireball damage to player is reduced by armorPct (10/20/30/45 % per tier).
 * Opening roar deals difficulty-scaled damage also reduced by armorPct.
 * Score is added to state.score in real time via addScore() (score.h).
 *
 * Inputs:  state — full GameState
 *            reads: difficulty, settings, player.equipment.armor,
 *                   player.health, player.maxHealth, score
 *            writes: score, dragonDefeated, phase
 * Outputs: true  = dragon defeated → state.phase = PHASE_VICTORY
 *          false = player died, fled, or terminal too small
 *                → state.phase = PHASE_GAMEOVER
 */
bool runBossFight(GameState& state) {
    setlocale(LC_ALL, "");  // enable UTF-8 in ncurses
    // Init ncurses
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(0);
    initFightColors();
    computeLayout();

    // Minimum terminal size check
    if (NC_ROWS < 24 || NC_COLS < 60) {
        endwin();
        std::cout << "\n\033[1;31m  Terminal too small!\033[0m  Please resize to at least 60 cols x 24 rows.\n"
                  << "  Current: " << NC_COLS << " x " << NC_ROWS << "\n"
                  << "  Press any key...\n";
        std::cout.flush();
        char dummy; read(STDIN_FILENO, &dummy, 1);
        return false;
    }

    BossConfig config     = initBossConfig(state.difficulty);
    int        playerMaxHp = state.player.maxHealth;
    int        playerHp    = state.player.health;
    int        arrowDmg    = calcArmorDamage(state.player.equipment.armor);
    int        miningSnap  = state.score;
    int        p1h = 0, p2h = 0, p3h = 0;
    bool       killBonus   = false;
    bool       won         = false;

    // Armor damage reduction (%) for incoming fireballs
    int armorPct = 0;
    switch (state.player.equipment.armor) {
        case MATERIAL_STONE:   armorPct = 10; break;
        case MATERIAL_IRON:    armorPct = 20; break;
        case MATERIAL_GOLD:    armorPct = 30; break;
        case MATERIAL_DIAMOND: armorPct = 45; break;
        default: break;
    }

    Dragon   dragon = initDragon(config);
    Fireball fbs[FF_MAX_FIREBALLS];
    Arrow    arrows[FF_MAX_ARROWS];
    for (int i = 0; i < FF_MAX_FIREBALLS; i++) fbs[i]    = {0,0,false,0};
    for (int i = 0; i < FF_MAX_ARROWS;    i++) arrows[i] = {0,0,false};

    int playerX   = NC_COLS / 2;
    const int HALF    = 6;
    const int PLR_MIN = 1 + HALF + 1;
    int       PLR_MAX = NC_COLS - 2 - HALF - 1;

    // Entry roar — calculated here, applied visibly on tick 1
    int roarDmg = (state.difficulty == DIFF_EASY) ? 20 :
                  (state.difficulty == DIFF_HARD) ? 45 : 30;
    roarDmg = roarDmg * (100 - armorPct) / 100;
    if (roarDmg < 5) roarDmg = 5;

    int  fireballTick    = -20;  // 20-tick grace after opening before first volley
    int  currentFireRate = config.fireRateTicks;
    int  tick            = 0;
    int  flashTicks      = 0;
    int  openingTicks    = 50;   // locked dramatic entry sequence
    bool running         = true;

    showIntro(state);

    while (running) {
        tick++;

        // Apply roar damage on the very first tick (visible in opening sequence)
        if (tick == 1) {
            playerHp -= roarDmg;
            if (playerHp < 1) playerHp = 1;
        }

        // ── Opening sequence: 50 ticks of locked fire-blast animation ─────────
        if (openingTicks > 0) {
            openingTicks--;
            int ch;
            while ((ch = getch()) != ERR)
                if (ch == 'q' || ch == 'Q') { running = false; won = false; }
            renderOpening(playerX, playerHp, playerMaxHp, state.score, dragon,
                          state.player.equipment.armor, openingTicks, roarDmg);
            usleep(FF_TICK_US);
            continue;
        }

        // Drain the full key buffer so move + shoot can happen in the same frame
        {
            bool wantLeft = false, wantRight = false, wantShoot = false;
            int ch;
            while ((ch = getch()) != ERR) {
                if      (ch == 'a' || ch == 'A') wantLeft  = true;
                else if (ch == 'd' || ch == 'D') wantRight = true;
                else if (ch == ' ')              wantShoot = true;
                else if (ch == 'q' || ch == 'Q') { running = false; won = false; break; }
                else if (ch == KEY_RESIZE)       { computeLayout(); PLR_MAX = NC_COLS - 2 - HALF - 1; }
            }
            // Apply movement — allow both shoot + move in the same tick
            if (wantLeft)  playerX = std::max(PLR_MIN, playerX - 4);
            if (wantRight) playerX = std::min(PLR_MAX, playerX + 4);
            if (wantShoot) fireArrow(arrows, playerX);
        }

        // Move dragon
        dragon.x += dragon.speed * dragon.direction;
        if      (dragon.x <= DRAG_MIN_X) { dragon.x = DRAG_MIN_X; dragon.direction =  1; }
        else if (dragon.x >= DRAG_MAX_X) { dragon.x = DRAG_MAX_X; dragon.direction = -1; }

        // Spawn fireballs
        if (++fireballTick >= currentFireRate) {
            fireballTick = 0;
            spawnFireballs(fbs, dragon, config.spread3);
        }

        // Move fireballs
        for (int i = 0; i < FF_MAX_FIREBALLS; i++) {
            if (!fbs[i].active) continue;
            fbs[i].y++;
            fbs[i].x += fbs[i].dx;
            if (fbs[i].y > ROW_PLAYER || fbs[i].x <= 0 || fbs[i].x >= NC_COLS - 2)
                fbs[i].active = false;
        }

        // Move arrows
        for (int i = 0; i < FF_MAX_ARROWS; i++) {
            if (!arrows[i].active) continue;
            arrows[i].y--;
            if (arrows[i].y < ROW_TOP_SEP) arrows[i].active = false;
        }

        // Arrow-dragon collision
        bool hitFlash = (flashTicks > 0);
        if (flashTicks > 0) flashTicks--;
        for (int i = 0; i < FF_MAX_ARROWS; i++) {
            if (!arrows[i].active) continue;
            bool hitRow = (arrows[i].y >= ROW_DRAG_START && arrows[i].y <= ROW_DRAG_END);
            bool hitCol = (arrows[i].x >= dragon.x && arrows[i].x < dragon.x + FF_DRAGON_COLS);
            if (hitRow && hitCol) {
                arrows[i].active = false;
                dragon.hp -= arrowDmg;
                if (dragon.hp < 0) dragon.hp = 0;
                int pts = (dragon.phase == FF_PHASE1) ? FF_SCORE_HIT_P1 :
                          (dragon.phase == FF_PHASE2) ? FF_SCORE_HIT_P2 : FF_SCORE_HIT_P3;
                addScore(state, pts);
                if      (dragon.phase == FF_PHASE1) p1h++;
                else if (dragon.phase == FF_PHASE2) p2h++;
                else                                p3h++;
                flashTicks = 4;
                hitFlash   = true;
            }
        }

        // Fireball-player collision
        int pLeft = playerX - HALF, pRight = playerX + HALF;
        for (int i = 0; i < FF_MAX_FIREBALLS; i++) {
            if (!fbs[i].active) continue;
            if (fbs[i].y == ROW_PLAYER && fbs[i].x >= pLeft && fbs[i].x <= pRight) {
                fbs[i].active = false;
                int dmg = config.fireballDmg * (100 - armorPct) / 100;
                if (dmg < 1) dmg = 1;
                playerHp -= dmg;
                if (playerHp < 0) playerHp = 0;
            }
        }

        {
            int prevPhase = dragon.phase;
            updatePhase(dragon, config.dragonSpeed);
            // Phase change announcement
            if (dragon.phase > prevPhase)
                dragon.announceTicks = std::max(dragon.announceTicks, 25);
        }

        // Hard-mode enrage: triggers once at or below 50% HP
        if (config.hasEnrage && !dragon.enraged && dragon.hp > 0 &&
            dragon.hp * 100 / dragon.maxHp <= 50) {
            dragon.enraged    = true;
            dragon.speed     += 3;
            currentFireRate   = std::max(5, currentFireRate / 2);
            dragon.announceTicks = 40;  // 2 seconds of banner
        }
        if (dragon.announceTicks > 0) dragon.announceTicks--;

        if (dragon.hp <= 0) {
            addScore(state, FF_SCORE_KILL);
            killBonus = true; won = true; running = false;
        }
        if (playerHp <= 0 && running) { won = false; running = false; }

        renderFrame(dragon, fbs, arrows, playerX, playerHp, playerMaxHp,
                    state.score, state.player.equipment.armor, tick, hitFlash);

        usleep(FF_TICK_US);
    }

    endwin();  // restore terminal before ANSI output

    state.dragonDefeated = won;
    state.phase = won ? PHASE_VICTORY : PHASE_GAMEOVER;

    showScoreBreakdown(won, miningSnap, p1h, p2h, p3h,
                       killBonus, state.score, state.settings.scoreMultiplier);
    saveFinalScore(state, won);
    return won;
}
