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


#endif // FINAL_FIGHT_H
