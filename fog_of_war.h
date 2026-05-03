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
 *
 * =========================================================================
 * MODULE OVERVIEW
 * =========================================================================
 * The fog-of-war system controls which blocks in the world the player can
 * see.  It enforces a simple depth-based rule:
 *
 *   Surface / dirt rows (y < STONE_LEVEL):
 *       All blocks in these rows are permanently visible across the entire
 *       viewport.  The player is assumed to be able to see the full
 *       landscape at ground level without any fog.
 *
 *   Underground / stone rows (STONE_LEVEL <= y < DEEP_LEVEL):
 *       Blocks are hidden by default and are only revealed when the player
 *       moves within 3 cells of them (Euclidean distance ≤ 3).  Once a
 *       block is revealed it stays visible permanently — the player's
 *       exploration is recorded in the Block.visible flag.
 *
 *   Deep rows (y >= DEEP_LEVEL):
 *       Same as underground but with a tighter reveal radius of 2 cells,
 *       making the deepest layer feel claustrophobic and dangerous.
 *
 * This module also owns the world rendering pipeline.  Every frame it
 * builds the full terminal output — viewport cells, HUD bar, and status
 * message — into a single large static buffer and flushes it with one
 * write() system call.  This eliminates flicker caused by partial redraws.
 *
 * =========================================================================
 * FUNCTION REFERENCE
 * =========================================================================
 *
 *   getVisibilityRadius(worldY)
 *       Translates a world-space row index into the reveal radius that
 *       applies to blocks in that row.
 *
 *       worldY < STONE_LEVEL  → returns -1  (surface: always visible, no
 *                                            radius calculation needed)
 *       worldY < DEEP_LEVEL   → returns  3  (underground stone layer)
 *       worldY >= DEEP_LEVEL  → returns  2  (deep layer, tighter fog)
 *
 *       The return value is used by updateWorldVisibility() to determine
 *       how far around the player to set Block.visible = true each tick.
 *       A return of -1 signals that the whole surface sweep should handle
 *       that row instead of the circular reveal loop.
 *
 *   updateWorldVisibility(state)
 *       The main fog-of-war update function.  Should be called once per
 *       tick (or at least once every time the player moves) to refresh
 *       which blocks are visible.
 *
 *       It performs two passes over the world:
 *
 *         Pass 1 — Surface sweep:
 *           Iterates every viewport cell whose world Y is < STONE_LEVEL and
 *           sets Block.visible = true unconditionally.  This ensures the
 *           player always sees the full terrain silhouette and sky.
 *
 *         Pass 2 — Circular reveal:
 *           Calls getVisibilityRadius() for the player's current Y position.
 *           If the radius > 0, iterates all cells within a square of side
 *           (2*radius+1) centred on the player, then discards cells whose
 *           Euclidean distance to the player exceeds the radius (turning the
 *           square into a circle).  All surviving cells are permanently
 *           marked visible.
 *
 *       Blocks that have been revealed in previous ticks retain their
 *       visible == true flag forever — the fog is "permanent reveal" style,
 *       not a moving cone of vision.  Only freshly explored cells are
 *       updated each call.
 *
 *   renderWorld(state, statusMsg)
 *       The buffered rendering entry point called once per frame by the
 *       main game loop.  Writes the complete terminal frame — including
 *       the game viewport, HUD bars, and status message — into the static
 *       renderBuf[] and then flushes it with a single write() call.
 *
 *       Viewport rendering rules (checked in order for each cell):
 *         1. Player (@)         — bright yellow, always on top
 *         2. Alive enemy (B)    — bold red, drawn over terrain
 *         3. Out-of-bounds cell — sky cell (above surface) or black void
 *         4. BLOCK_SKY / surface AIR — delegated to renderSkyCell()
 *         5. Hidden block       — dark grey colon (':') — fog of war
 *         6. Red-zone tinted    — red background with white block char
 *         7. Normal visible block — coloured block char from getBlockColor()
 *
 *       HUD content (written after the viewport in the same buffer):
 *         Line 1 — HP bar (20-segment colour gradient) + score + equipment
 *         Line 2 — Inventory counts (wood/stone/iron/gold/diamond) +
 *                  facing direction arrow + depth + world coordinates
 *         Line 3 — Fire zone alert (blinks red/yellow when EVENT_RED_ZONE
 *                  is active and alertTicks > 0), conditionally included
 *         Line 4 — Status message (green for success, red for error/warning),
 *                  conditionally included only when statusMsg is non-empty
 *
 *       The function uses ANSI escape \033[H (cursor home) at the start so
 *       that the frame overwrites the previous one in-place without scrolling,
 *       and \033[J (erase to end of screen) before the HUD to clean up any
 *       residue from a previous taller frame.  The cursor is hidden with
 *       \033[?25l to prevent it from flickering across the screen during the
 *       write.
 *
 * =========================================================================
 * DEPENDENCIES
 * =========================================================================
 *   <string>   — for std::string (statusMsg parameter type)
 *   types.h    — for GameState, Block, BlockType, Enemy, RandomEvent,
 *                EVENT_RED_ZONE, SURFACE_LEVEL, STONE_LEVEL, DEEP_LEVEL
 */

#ifndef FOG_OF_WAR_H
#define FOG_OF_WAR_H

#include <string>
#include "types.h"

// -------------------------------------------------------------------------
// getVisibilityRadius
//
// Returns the circular reveal radius (in world-space blocks) for the given
// world row index.  The radius governs how far around the player blocks are
// permanently revealed when the player occupies (or passes through) a cell
// in that row.
//
// Parameters:
//   worldY — the world-space Y coordinate of the row to query.
//             Row 0 is the top of the world (sky); increasing Y goes deeper.
//
// Return values:
//   -1  — The row is at or above STONE_LEVEL (surface / dirt).
//          Blocks here are always visible regardless of player position,
//          so no radius calculation is required.  The caller should use the
//          surface sweep path instead.
//
//    3  — The row is in the underground / stone layer (STONE_LEVEL <= y < DEEP_LEVEL).
//          A circular area of radius 3 is revealed around the player each tick.
//
//    2  — The row is in the deep layer (y >= DEEP_LEVEL).
//          A smaller circular area of radius 2 is revealed, increasing
//          the sense of claustrophobia and danger at great depth.
// -------------------------------------------------------------------------
int getVisibilityRadius(int worldY);

// -------------------------------------------------------------------------
// updateWorldVisibility
//
// Refreshes the Block.visible flags for the entire world based on the
// player's current position stored in state.player.pos.
//
// This function must be called at least once per tick (ideally after the
// player's position has been updated) so that newly entered areas are
// correctly revealed before the frame is rendered.
//
// Parameters:
//   state — mutable reference to the full GameState.  The function reads
//            state.player.pos, state.camera, state.viewportWidth/Height,
//            state.worldWidth/Height, and writes state.world[y][x].visible.
//
// Behaviour:
//   Surface pass:
//     For every viewport row whose world Y < STONE_LEVEL, every block in
//     that row (within worldWidth bounds) is permanently marked visible.
//     This pass runs regardless of where the player is standing.
//
//   Underground circular pass:
//     Calls getVisibilityRadius(player.pos.y) to get the radius r.
//     If r > 0, iterates a (2r+1)×(2r+1) square centred on the player
//     and marks all cells with dx²+dy² ≤ r² as permanently visible.
//     If r == -1 (player is at the surface) this pass is skipped entirely.
// -------------------------------------------------------------------------
void updateWorldVisibility(GameState& state);

// -------------------------------------------------------------------------
// renderWorld
//
// Renders the complete terminal frame — viewport, HUD, and status line —
// into a single static buffer and flushes it with one write() call.
//
// This all-in-one approach guarantees that the terminal never shows a
// partially drawn frame; the entire screen update is atomic from the
// terminal's perspective, eliminating visible tearing or flickering.
//
// Parameters:
//   state     — const reference to the full game state (camera, player,
//               enemies, world, active event, viewport dimensions, score).
//               This function does not modify state; all writes go to the
//               terminal via the static renderBuf.
//   statusMsg — the most recent game feedback message (e.g. "Mined iron ore",
//               "Need more wood", "BURNING!").  Pass an empty string to
//               suppress the status line.  The function automatically colours
//               the message red if it contains "Failed", "Need", or "BURNING";
//               otherwise it is coloured green.
//
// Output structure (written to STDOUT):
//   \033[?25l\033[H       — hide cursor, move to top-left
//   <viewport rows>       — vpH rows of vpW cells, one line per row,
//                           each line terminated with \033[K\n (clear to EOL)
//   \033[J                — erase from cursor to end of screen
//   <HUD line 1>          — HP bar + score + pickaxe + armour
//   <HUD line 2>          — inventory + direction + depth + coordinates
//   [HUD line 3]          — fire zone alert (only when active)
//   [HUD line 4]          — status message (only when non-empty)
// -------------------------------------------------------------------------
void renderWorld(const GameState& state, const std::string& statusMsg);

#endif
