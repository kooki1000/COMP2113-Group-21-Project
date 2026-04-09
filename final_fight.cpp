// =============================================================================
// final_fight.cpp
// TermiCraft — Final Boss Fight Module Implementation
//
// Implements the Space Invaders-style dragon boss fight for TermiCraft.
// The dragon moves as a full ASCII block across the arena, fires fireballs
// in phase-dependent patterns, and the player shoots upward arrows with
// rapid-fire support. Three phases are triggered at 66% and 33% dragon HP,
// increasing speed, fireball spread, and score per hit.
//
// Rendering uses ncurses for flicker-free terminal output.
// usleep() controls the game tick rate (50ms per tick = 20 ticks/second).
//
// Compile flag required: -lncurses
// ncurses is pre-installed on Ubuntu (HKU CS academy server).
//
// Author:       Sohan
// Dependencies: ncurses, <unistd.h>, <fstream>, <ctime>, <cstring>
// =============================================================================
