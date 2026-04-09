/* TEAMNOTE START::
 * colors.h
 * 
 * ANSI escape codes for terminal colors and cursor control.
 * Include this if you want to make text colorful or move the cursor around.
 * 
 * Usage: cout << COLOR_RED << "Error!" << COLOR_RESET << endl;
 *
 *TEAMNOTE END
 */

#ifndef COLORS_H
#define COLORS_H

#include <iostream>
#include "types.h"

// ----- RESET -----
#define COLOR_RESET   "\033[0m"

// ----- REGULAR COLORS -----
#define COLOR_BLACK   "\033[0;30m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_YELLOW  "\033[0;33m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_MAGENTA "\033[0;35m"
#define COLOR_CYAN    "\033[0;36m"
#define COLOR_WHITE   "\033[0;37m"

// ----- BOLD COLORS -----
#define COLOR_BOLD_BLACK   "\033[1;30m"
#define COLOR_BOLD_RED     "\033[1;31m"
#define COLOR_BOLD_GREEN   "\033[1;32m"
#define COLOR_BOLD_YELLOW  "\033[1;33m"
#define COLOR_BOLD_BLUE    "\033[1;34m"
#define COLOR_BOLD_MAGENTA "\033[1;35m"
#define COLOR_BOLD_CYAN    "\033[1;36m"
#define COLOR_BOLD_WHITE   "\033[1;37m"

// ----- BACKGROUND COLORS -----
#define BG_BLACK   "\033[40m"
#define BG_RED     "\033[41m"
#define BG_GREEN   "\033[42m"
#define BG_YELLOW  "\033[43m"
#define BG_BLUE    "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN    "\033[46m"
#define BG_WHITE   "\033[47m"

// ----- TEXT STYLES -----
#define STYLE_BOLD      "\033[1m"
#define STYLE_DIM       "\033[2m"
#define COLOR_DIM       "\033[2m"
#define STYLE_ITALIC    "\033[3m"
#define STYLE_UNDERLINE "\033[4m"
#define STYLE_BLINK     "\033[5m"
#define STYLE_REVERSE   "\033[7m"

// ----- SCREEN CONTROL -----
#define CLEAR_SCREEN    "\033[2J"
#define CURSOR_HOME     "\033[H"
#define CURSOR_HIDE     "\033[?25l"
#define CURSOR_SHOW     "\033[?25h"

// ----- GAME-SPECIFIC COLORS -----
// These use 256-color mode for nicer looking blocks

// Block colors
#define COLOR_SKY       "\033[38;5;117m"   // light blue
#define COLOR_GRASS     "\033[38;5;34m"    // bright green
#define COLOR_DIRT      "\033[38;5;94m"    // brown
#define COLOR_STONE     "\033[38;5;245m"   // gray
#define COLOR_COAL      "\033[38;5;236m"   // dark gray
#define COLOR_IRON      "\033[38;5;252m"   // silver
#define COLOR_GOLD_ORE  "\033[38;5;220m"   // gold
#define COLOR_DIAMOND   "\033[38;5;51m"    // cyan/diamond blue
#define COLOR_WOOD      "\033[38;5;130m"   // brown wood
#define COLOR_LEAVES    "\033[38;5;28m"    // dark green
#define COLOR_BEDROCK   "\033[38;5;232m"   // almost black
#define COLOR_DRAGON    "\033[38;5;196m"   // bright red

// UI colors
#define COLOR_TITLE     "\033[38;5;208m"   // orange
#define COLOR_MENU      "\033[38;5;39m"    // blue
#define COLOR_SELECTED  "\033[38;5;226m"   // yellow
#define COLOR_HEALTH    "\033[38;5;196m"   // red
#define COLOR_MANA      "\033[38;5;33m"    // blue
#define COLOR_SCORE     "\033[38;5;226m"   // yellow
#define COLOR_WARNING   "\033[38;5;208m"   // orange
#define COLOR_SUCCESS   "\033[38;5;46m"    // green
#define COLOR_DANGER    "\033[38;5;196m"   // red

// Player character
#define COLOR_PLAYER    "\033[38;5;226m"   // yellow

// ----- HELPER FUNCTIONS -----

// Get the right color for a block type
inline const char* getBlockColor(BlockType type) {
    switch (type) {
        case BLOCK_AIR:         return COLOR_RESET;
        case BLOCK_SKY:         return COLOR_SKY;
        case BLOCK_GRASS:       return COLOR_GRASS;
        case BLOCK_DIRT:        return COLOR_DIRT;
        case BLOCK_STONE:       return COLOR_STONE;
        case BLOCK_COAL:        return COLOR_COAL;
        case BLOCK_IRON:        return COLOR_IRON;
        case BLOCK_GOLD:        return COLOR_GOLD_ORE;
        case BLOCK_DIAMOND:     return COLOR_DIAMOND;
        case BLOCK_WOOD:        return COLOR_WOOD;
        case BLOCK_LEAVES:      return COLOR_LEAVES;
        case BLOCK_BEDROCK:     return COLOR_BEDROCK;
        case BLOCK_DRAGON_CAVE: return COLOR_DRAGON;
        default:                return COLOR_RESET;
    }
}

// Get color for material tier (tools/armor)
inline const char* getMaterialColor(MaterialTier tier) {
    switch (tier) {
        case MATERIAL_NONE:    return COLOR_WHITE;
        case MATERIAL_WOOD:    return COLOR_WOOD;
        case MATERIAL_STONE:   return COLOR_STONE;
        case MATERIAL_IRON:    return COLOR_IRON;
        case MATERIAL_GOLD:    return COLOR_GOLD_ORE;
        case MATERIAL_DIAMOND: return COLOR_DIAMOND;
        default:               return COLOR_WHITE;
    }
}

// color for difficulty text
inline const char* getDifficultyColor(Difficulty diff) {
    switch (diff) {
        case DIFF_EASY:   return COLOR_GREEN;
        case DIFF_NORMAL: return COLOR_YELLOW;
        case DIFF_HARD:   return COLOR_RED;
        default:          return COLOR_WHITE;
    }
}

// Move cursor to specific position (row and col are 1-indexed)
inline void moveCursor(int row, int col) {
    std::cout << "\033[" << row << ";" << col << "H";
}

// Clear screen and put cursor at top-left
inline void clearScreen() {
    std::cout << CLEAR_SCREEN << CURSOR_HOME;
}

#endif
