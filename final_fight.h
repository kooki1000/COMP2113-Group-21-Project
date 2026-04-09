// =============================================================================
// final_fight.h
// TermiCraft — Final Boss Fight Module Header
//
// Declares all constants, structs, and functions for the Space Invaders-style
// dragon boss fight. This module handles the full fight sequence: arena
// rendering, dragon movement, fireball and arrow mechanics, phase transitions,
// scoring, name entry, and high score persistence.
//
// Author:       Sohan
// Dependencies: ncurses (compile with -lncurses), <unistd.h> for usleep()
// =============================================================================

#ifndef FINAL_FIGHT_H
#define FINAL_FIGHT_H

#include <string>
 
// Arena dimensions
#define ARENA_WIDTH       100   // total terminal columns
#define ARENA_HEIGHT       35   // total terminal rows
#define ARENA_INNER_W      98   // width inside the border walls
#define PLAYER_ROW         29   // row where the player sprite sits (0-indexed)
#define DRAGON_START_ROW    4   // top row of dragon art (below HUD border)

// Dragon art dimensions
#define DRAGON_ROWS        12   // number of lines in the dragon ASCII block
#define DRAGON_COLS        36   // rendered width of the widest dragon line
#define DRAGON_MIN_X        1   // leftmost column the dragon left-edge can reach
#define DRAGON_MAX_X       63   // rightmost (ARENA_WIDTH - DRAGON_COLS - 1)

// Projectile limits
#define MAX_FIREBALLS      12   // maximum simultaneous fireballs on screen
#define MAX_ARROWS         10   // maximum simultaneous player arrows on screen
 

// High score
#define MAX_NAME_LEN       12   // maximum player name characters
#define HIGHSCORE_FILE     "highscore.txt"
 
// Armor level codes
// Passed in from player.cpp (Koki's module) as armorLevel int
#define ARMOR_NONE         0
#define ARMOR_STONE        1
#define ARMOR_IRON         2
#define ARMOR_GOLD         3
#define ARMOR_DIAMOND      4
 
// HP added to base difficulty HP per armor level (same on all difficulties)
#define HP_BONUS_NONE       0
#define HP_BONUS_STONE      8
#define HP_BONUS_IRON      15
#define HP_BONUS_GOLD      22
#define HP_BONUS_DIAMOND   30
 
// Damage dealt to the dragon per arrow hit, per armor level
#define ARROW_DMG_NONE      1
#define ARROW_DMG_STONE     1
#define ARROW_DMG_IRON      3
#define ARROW_DMG_GOLD      5
#define ARROW_DMG_DIAMOND   8

// Difficulty codes
// Passed in from main.cpp (Saarim's module)
#define DIFF_EASY          0
#define DIFF_NORMAL        1
#define DIFF_HARD          2
 
// Base player HP per difficulty (before armor bonus is applied)
#define BASE_HP_EASY       40
#define BASE_HP_NORMAL     30
#define BASE_HP_HARD       20

#endif // FINAL_FIGHT_H
