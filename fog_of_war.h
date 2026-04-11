/*
 * fog_of_war.h
 *
 * Fog of War visibility system for TermiCraft.
 * Handles block reveal radius by depth zone and world rendering
 * with buffered output (no flicker).
 *
 * Surface/dirt: always visible in viewport.
 * Underground (stone layer): 3-block circular radius, permanent reveal.
 * Deep layer: 2-block circular radius, permanent reveal.
 *
 * Author: Mohit
 */

#ifndef FOG_OF_WAR_H
#define FOG_OF_WAR_H

#include "types.h"

// Get the visibility radius for a given world row.
// Returns -1 if that row is always visible (surface/dirt).
int getVisibilityRadius(int worldY);

// Update which blocks are visible based on player position.
// Call once per tick or whenever the player moves.
void updateWorldVisibility(GameState& state);

// Render the entire world viewport into the terminal.
// Uses buffered output (single write call) to prevent flicker.
// Calls day_night functions for sky cells.
void renderWorld(const GameState& state);

#endif
