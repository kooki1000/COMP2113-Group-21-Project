/*
 * fog_of_war.cpp
 *
 * Fog of War visibility system and world rendering for TermiCraft.
 * Everything — world, HUD, status line — is written into one large buffer
 * and flushed with a single write() call so there is never any tearing.
 *
 * Author: Mohit
 *
 * =========================================================================
 * MODULE OVERVIEW
 * =========================================================================
 * This file implements two closely related responsibilities:
 *
 *   1.  FOG OF WAR  — deciding which blocks the player can see based on
 *       their current depth and position in the world.  Surface blocks are
 *       always visible; underground blocks are revealed in a small circle
 *       around the player and stay permanently visible once seen.
 *
 *   2.  WORLD RENDERING  — converting the GameState into ANSI escape
 *       sequences and writing them to the terminal.  The entire frame —
 *       viewport cells, HUD bars, and status message — is assembled into
 *       one static character buffer (renderBuf) and flushed with a single
 *       write() call.  This is the key technique that eliminates flicker:
 *       because the OS writes the buffer atomically from the terminal's
 *       perspective, the screen is never partially updated.
 *
 * =========================================================================
 * FUNCTION REFERENCE
 * =========================================================================
 *
 *   getVisibilityRadius(worldY)
 *       Returns the circular reveal radius for a given world row.
 *       -1  → surface (always visible, no radius needed)
 *        3  → stone/underground layer
 *        2  → deep layer (tighter fog for extra tension)
 *
 *   updateWorldVisibility(state)
 *       Two-pass fog update called each tick after the player moves.
 *       Pass 1: marks all surface-level viewport cells visible regardless
 *               of player position.
 *       Pass 2: marks all blocks within a circular radius of the player
 *               permanently visible if the player is underground.
 *
 *   appendHUD(buf, pos, state, statusMsg)  [static]
 *       Internal helper that appends the HUD and status line to the render
 *       buffer at position pos.  Returns the new buffer position.
 *       Writes:
 *         Line 1 — HP bar (20 block segments with colour gradient by HP
 *                  fraction) + score + pickaxe tier + armour tier
 *         Line 2 — Resource inventory (T/# /I/G/D counts) + facing arrow
 *                  + depth below surface + world (x, y) coordinates
 *         Line 3 — Fire zone alert (alternating red/yellow blink), only
 *                  when EVENT_RED_ZONE is active with alertTicks > 0
 *         Line 4 — Status message coloured green (info) or red (error)
 *
 *   renderWorld(state, statusMsg)
 *       Public entry point.  Builds the complete terminal frame into
 *       renderBuf and flushes it with write(STDOUT_FILENO, ...).
 *       For each viewport cell the rendering priority is:
 *         1. Player '@' (bright yellow)
 *         2. Alive enemy 'B' (bold red) — first live enemy at this cell
 *         3. Out-of-bounds — sky cell (row ≤ SURFACE_LEVEL+1) or void ' '
 *         4. BLOCK_SKY or surface BLOCK_AIR — delegates to renderSkyCell()
 *         5. !b.visible — fog colon ':' in dark grey
 *         6. Inside red-zone radius — red background + white block char
 *         7. Visible block — normal colour from getBlockColor() + char
 *       After the viewport, calls appendHUD() in the same buffer pass.
 *
 * =========================================================================
 * DEPENDENCIES
 * =========================================================================
 *   fog_of_war.h — this module's own header (forward declarations)
 *   day_night.h  — for renderSkyCell() and SURFACE_LEVEL / STONE_LEVEL /
 *                  DEEP_LEVEL constants
 *   colors.h     — for getBlockColor(), getBlockChar(), getMaterialColor(),
 *                  getMaterialName()
 *   <cstdio>     — for sprintf()
 *   <cstring>    — for memcpy()
 *   <cstdlib>    — for abs()
 *   <string>     — for std::string (statusMsg)
 *   <unistd.h>   — for write() and STDOUT_FILENO
 */

#include "fog_of_war.h"
#include "day_night.h"
#include "colors.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <unistd.h>

// =========================================================================
// SECTION 1 — VISIBILITY RADIUS BY DEPTH
// =========================================================================

// -------------------------------------------------------------------------
// getVisibilityRadius
//
// Maps a world Y coordinate to the circular reveal radius that the
// fog-of-war system uses when the player is in that row.  The function
// implements a three-zone model:
//
//   Zone 1 — Surface/dirt  (worldY < STONE_LEVEL)
//       Always fully visible — the player can see the entire landscape.
//       Returns -1 as a sentinel; callers use the surface sweep instead
//       of calling the circular reveal loop.
//
//   Zone 2 — Stone/underground  (STONE_LEVEL <= worldY < DEEP_LEVEL)
//       Visibility radius 3.  Represents torch-lit mine tunnels where
//       the player can see a moderate distance in all directions.
//
//   Zone 3 — Deep layer  (worldY >= DEEP_LEVEL)
//       Visibility radius 2.  The deep zones are poorly lit; only the
//       immediate surroundings are revealed, increasing tension.
// -------------------------------------------------------------------------
int getVisibilityRadius(int worldY) {
    if (worldY < STONE_LEVEL) {
        return -1;  // Surface + dirt zone: always visible, no radius required
    } else if (worldY < DEEP_LEVEL) {
        return 3;   // Underground / stone layer: moderate visibility radius
    } else {
        return 2;   // Deep layer: tight visibility radius for atmosphere
    }
}

// =========================================================================
// SECTION 2 — UPDATE VISIBILITY
// =========================================================================

// -------------------------------------------------------------------------
// updateWorldVisibility
//
// Refreshes which blocks are marked visible based on the player's current
// world position.  Should be called once per game tick, after movement is
// resolved, and before the frame is rendered.
//
// The function uses a two-pass approach so that surface and underground
// visibility are handled by separate, efficient loops:
//
//   PASS 1 — Surface sweep
//     Iterates every viewport row/column combination.  For each cell whose
//     world Y < STONE_LEVEL, Block.visible is set to true permanently.
//     This ensures the player always sees the full sky and terrain contour
//     even if they are standing underground.  The sweep is bounded by the
//     current viewport dimensions and world bounds.
//
//   PASS 2 — Circular underground reveal
//     Reads the player's current Y to determine the visibility radius via
//     getVisibilityRadius().  If radius > 0, the function iterates a square
//     of side (2*radius + 1) centred on the player's (px, py) coordinate,
//     computing dx²+dy² for each offset and skipping cells that fall outside
//     the circle.  All cells within the circle are permanently marked visible
//     (Block.visible = true), respecting world boundary checks.
//     If the player is at the surface (radius == -1) this pass is skipped.
// -------------------------------------------------------------------------
void updateWorldVisibility(GameState& state) {
    // Cache player world position for the underground reveal pass
    int px = state.player.pos.x;
    int py = state.player.pos.y;

    // Cache camera origin for viewport-to-world coordinate conversion
    int camX = state.camera.x;
    int camY = state.camera.y;

    // ------------------------------------------------------------------
    // PASS 1: Surface / dirt rows — always visible within the viewport
    // ------------------------------------------------------------------
    // Scan every row that the viewport currently shows
    for (int vy = 0; vy < state.viewportHeight; vy++) {
        // Convert viewport row to world row
        int wy = camY + vy;

        // Skip rows that are outside the valid world bounds
        if (wy < 0 || wy >= state.worldHeight) continue;

        // Only apply the always-visible rule above the stone layer
        if (wy < STONE_LEVEL) {
            // Mark every in-bounds column in this surface row as visible
            for (int vx = 0; vx < state.viewportWidth; vx++) {
                int wx = camX + vx;
                // Guard against accessing beyond the horizontal world boundary
                if (wx >= 0 && wx < state.worldWidth)
                    state.world[wy][wx].visible = true;
            }
        }
    }

    // ------------------------------------------------------------------
    // PASS 2: Underground circular reveal centred on the player
    // ------------------------------------------------------------------
    // Determine how far blocks should be revealed around the player's depth
    int radius = getVisibilityRadius(py);

    // A radius of -1 means the player is at the surface — nothing to do here
    if (radius > 0) {
        // Pre-compute radius² to avoid sqrt in the inner loop (integer comparison)
        int r2 = radius * radius;

        // Iterate a square bounding box, then discard corners to make a circle
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                // Discard cells outside the circular radius (Euclidean distance)
                if (dx * dx + dy * dy > r2) continue;

                // Compute the world coordinate of this offset cell
                int wx = px + dx;
                int wy = py + dy;

                // Bounds-check before writing to avoid out-of-bounds memory access
                if (wx >= 0 && wx < state.worldWidth &&
                    wy >= 0 && wy < state.worldHeight) {
                    // Permanently reveal this block — visible flags are never cleared
                    state.world[wy][wx].visible = true;
                }
            }
        }
    }
}

// =========================================================================
// SECTION 3 — BUFFERED RENDERING
// =========================================================================

// Static render buffer — large enough to hold the worst-case frame.
// 100 cols × 50 rows × ~30 bytes/cell ≈ 150 000 bytes for the viewport,
// plus generous headroom for HUD lines and ANSI escape sequences.
// The extra space ensures no overflow even with very wide terminals.
// World + HUD + status all go into one buffer, one write() — zero tearing.
static char renderBuf[700000];

// -------------------------------------------------------------------------
// appendHUD  [static — internal only]
//
// Writes the HUD lines (HP bar, inventory, fire alert, status) into buf
// starting at offset pos and returns the new write position.  This is
// called at the end of renderWorld() so both the viewport and HUD travel
// in the same buffer flush.
//
// Parameters:
//   buf       — pointer to the start of renderBuf (NOT buf+pos)
//   pos       — current write offset into buf
//   state     — const game state for player stats, inventory, active events
//   statusMsg — last game message to display; empty string → no line 4
//
// HUD line layout:
//   Line 1: HP:<bar> hp/maxhp  PTS:<score>  ⛏ <pickaxe>  🛡 <armour>
//   Line 2: T:<wood> #:<stone> I:<iron> G:<gold> D:<diamond>
//            DIR:<arrow>  Depth:<y - SURFACE_LEVEL>  (<x>,<y>)
//   Line 3: (conditional) fire zone alert — alternates red blink / yellow text
//   Line 4: (conditional) status message — green for info, red for errors
// -------------------------------------------------------------------------
static int appendHUD(char* buf, int pos, const GameState& state,
                     const std::string& statusMsg) {

    // ── Line 1: HP bar + score + equipment ──────────────────────────────────

    // Guard against division by zero if maxHealth is somehow 0
    int maxHp = state.player.maxHealth > 0 ? state.player.maxHealth : 1;

    // Scale current HP to a 20-segment bar; clamp result to [0, 20]
    int hpFilled = (state.player.health * 20) / maxHp;
    if (hpFilled < 0)  hpFilled = 0;   // Clamp lower bound (health cannot be negative visually)
    if (hpFilled > 20) hpFilled = 20;  // Clamp upper bound (never more than full bar)

    // Write the HP label and opening bracket for the bar
    pos += sprintf(buf + pos, "\033[1;37m HP:\033[0m [");

    // Draw each of the 20 HP bar segments
    for (int i = 0; i < 20; i++) {
        if (i < hpFilled) {
            // Filled segment — colour varies by fraction:
            //   > 50% HP → green   (safe)
            //   > 25% HP → yellow  (caution)
            //   ≤ 25% HP → red     (danger)
            const char* col = (hpFilled > 10) ? "\033[0;32m"   // Healthy: green
                            : (hpFilled >  5) ? "\033[0;33m"   // Low:     yellow
                                              : "\033[0;31m";  // Critical: red
            // U+2588 FULL BLOCK (█) — solid filled segment
            pos += sprintf(buf + pos, "%s\xe2\x96\x88\033[0m", col);
        } else {
            // Empty segment — U+2591 LIGHT SHADE (░) — indicates missing HP
            pos += sprintf(buf + pos, "\033[2m\xe2\x96\x91\033[0m");
        }
    }

    // Print the numeric HP value after the bar (e.g. "14/20")
    pos += sprintf(buf + pos, "] %d/%d", state.player.health, state.player.maxHealth);

    // Print the player's current point score in yellow
    pos += sprintf(buf + pos, "  \033[0;33mPTS:%d\033[0m", state.score);

    // Print the equipped pickaxe tier with its material colour
    // ⛏ = U+26CF encoded as UTF-8 \xe2\x9b\x8f
    pos += sprintf(buf + pos, "  %s\xe2\x9b\x8f %s\033[0m",
                   getMaterialColor(state.player.equipment.pickaxe),
                   getMaterialName(state.player.equipment.pickaxe).c_str());

    // Print the equipped armour tier with its material colour
    // 🛡 = U+1F6E1 encoded as UTF-8 \xf0\x9f\x9b\xa1
    pos += sprintf(buf + pos, "  %s\xf0\x9f\x9b\xa1  %s\033[0m\n",
                   getMaterialColor(state.player.equipment.armor),
                   getMaterialName(state.player.equipment.armor).c_str());

    // ── Line 2: Inventory + direction + depth ────────────────────────────────

    // Print each resource count with a colour-coded single-letter prefix:
    //   T = wood (tan/brown), # = stone (grey), I = iron (orange-grey),
    //   G = gold (yellow-gold), D = diamond (bright purple)
    pos += sprintf(buf + pos,
        " \033[38;5;130mT:%d\033[0m"    // Wood count  — warm brown
        " \033[38;5;102m#:%d\033[0m"    // Stone count — muted grey
        " \033[38;5;208mI:%d\033[0m"    // Iron count  — orange
        " \033[38;5;220mG:%d\033[0m"    // Gold count  — bright yellow
        " \033[95mD:%d\033[0m",         // Diamond count — light magenta
        state.player.inventory.wood,
        state.player.inventory.stone,
        state.player.inventory.iron,
        state.player.inventory.gold,
        state.player.inventory.diamond);

    // Convert the player's current facing vector to a Unicode arrow
    // facingX and facingY are unit vectors; only one is non-zero at a time
    const char* dirArrow =
        (state.player.facingX ==  1) ? "\xe2\x86\x92" :   // → facing right
        (state.player.facingX == -1) ? "\xe2\x86\x90" :   // ← facing left
        (state.player.facingY == -1) ? "\xe2\x86\x91" :   // ↑ facing up
        (state.player.facingY ==  1) ? "\xe2\x86\x93" :   // ↓ facing down
                                       "?";                // Unknown / diagonal

    // Print the direction arrow, depth below surface, and absolute world coords
    pos += sprintf(buf + pos,
        "  \033[0;36mDIR:%s\033[0m"     // Direction arrow in cyan
        "  Depth:%d  (%d,%d)\n",        // Depth and (x,y) in default colour
        dirArrow,
        state.player.pos.y - SURFACE_LEVEL,   // Positive = below surface
        state.player.pos.x, state.player.pos.y);

    // ── Line 3 (conditional): fire zone alert ────────────────────────────────
    // Only rendered when an EVENT_RED_ZONE is active and the alert is counting down
    const RandomEvent& ev = state.activeEvent;
    if (ev.active && ev.type == EVENT_RED_ZONE && ev.alertTicks > 0) {
        // Blink effect: alternate between two messages every 8 ticks
        // (alertTicks / 8) gives the blink period, % 2 selects the phase
        bool blink = (ev.alertTicks / 8) % 2 == 0;
        if (blink) {
            // Phase A: bold red with full block border characters for high drama
            pos += sprintf(buf + pos,
                "\033[1;31m \xe2\x96\x88\xe2\x96\x88"            // ██ left border
                " FIRE ZONE ACTIVE \xe2\x96\x88\xe2\x96\x88"     // ██ right of text
                "  EVACUATE OR TAKE BURN DAMAGE  "               // centred warning
                "\xe2\x96\x88\xe2\x96\x88 FIRE ZONE \xe2\x96\x88\xe2\x96\x88"  // ██ right border
                "\033[0m\n");
        } else {
            // Phase B: yellow, less dramatic — still urgent but calmer
            pos += sprintf(buf + pos,
                "\033[1;33m !! DANGER: FIRE ZONE -- move away from the burning area !!\033[0m\n");
        }
    }

    // ── Line 4: status message ───────────────────────────────────────────────
    // Only rendered when there is a non-empty message to show
    if (!statusMsg.empty()) {
        // Detect error-class messages by looking for known failure keywords
        // "Failed" → crafting/action failed
        // "Need"   → missing resources
        // "BURNING"→ player is taking fire damage
        bool isErr = (statusMsg.find("Failed") != std::string::npos ||
                      statusMsg.find("Need")   != std::string::npos ||
                      statusMsg.find("BURNING") != std::string::npos);

        // Print with red for errors/warnings, green for informational messages
        pos += sprintf(buf + pos, "%s >> %s\033[0m\n",
                       isErr ? "\033[0;31m" : "\033[0;32m",
                       statusMsg.c_str());
    }

    // Return the updated write position so the caller can continue appending
    return pos;
}

// -------------------------------------------------------------------------
// renderWorld  (PUBLIC)
//
// Builds the complete terminal frame into renderBuf and flushes it with a
// single write() call.  This is the heart of TermiCraft's rendering system.
//
// Flow:
//   1. Emit cursor-hide + cursor-home escape to begin overwriting in-place.
//   2. Iterate every viewport cell (vy × vx) in row-major order.
//      For each cell, compute the world coordinate (wx, wy = camX+vx, camY+vy)
//      and determine the correct character + colour via the priority chain.
//   3. After each viewport row, emit \033[K\n (clear-to-EOL + newline).
//   4. After all rows, emit \033[J (erase to end of screen) to clean up
//      any residual content from a previously taller frame.
//   5. Call appendHUD() to write the HUD and status line into the same buffer.
//   6. NUL-terminate the buffer and flush it to STDOUT with write().
// -------------------------------------------------------------------------
void renderWorld(const GameState& state, const std::string& statusMsg) {
    // pos tracks how many bytes have been written into renderBuf so far
    int pos = 0;

    // Emit the preamble: hide the cursor and jump to top-left of the terminal
    // \033[?25l — hide cursor (prevents flickering cursor during the write)
    // \033[H    — cursor home (moves to row 1, column 1, i.e. top-left)
    // Together these 9 bytes ensure we overwrite the previous frame in-place
    const char* home = "\033[?25l\033[H";
    memcpy(renderBuf + pos, home, 9);
    pos += 9;

    // Cache frequently accessed state fields to avoid repeated struct dereferences
    int camX = state.camera.x;       // World X coordinate of the viewport left edge
    int camY = state.camera.y;       // World Y coordinate of the viewport top edge
    int vpW  = state.viewportWidth;  // Number of columns in the viewport
    int vpH  = state.viewportHeight; // Number of rows in the viewport

    // ── Viewport rendering loop ───────────────────────────────────────────
    // Iterate rows top-to-bottom, columns left-to-right (row-major order)
    for (int vy = 0; vy < vpH; vy++) {
        // Convert viewport row to world row
        int wy = camY + vy;

        for (int vx = 0; vx < vpW; vx++) {
            // Convert viewport column to world column
            int wx = camX + vx;

            // ── Priority 1: Player character ─────────────────────────────
            // The player '@' is always drawn on top of everything else.
            // Rendered in bright yellow (256-colour #226) to stand out.
            if (wx == state.player.pos.x && wy == state.player.pos.y) {
                pos += sprintf(renderBuf + pos, "\033[38;5;226m@\033[0m");
                continue; // Cell is fully resolved — skip remaining priorities
            }

            // ── Priority 2: Enemies ──────────────────────────────────────
            // Scan the enemy list for any alive enemy occupying this cell.
            // Rendered as bold red 'B' (for "Beast" / generic mob).
            // Only the first match is drawn (no stacking rendering).
            bool isEnemy = false;
            for (const Enemy& e : state.enemies) {
                if (e.alive && e.pos.x == wx && e.pos.y == wy) {
                    pos += sprintf(renderBuf + pos, "\033[1;31mB\033[0m");
                    isEnemy = true;
                    break; // Stop after the first enemy found at this cell
                }
            }
            if (isEnemy) continue; // Cell resolved — skip remaining priorities

            // ── Priority 3: Out-of-bounds cells ─────────────────────────
            // Cells whose world coordinates fall outside the world array
            // bounds are either sky (near the surface) or void (underground).
            if (wx < 0 || wx >= state.worldWidth ||
                wy < 0 || wy >= state.worldHeight) {
                // Rows close to or above the surface look like sky
                if (wy >= 0 && wy <= SURFACE_LEVEL + 1)
                    pos += renderSkyCell(wy, vx, vpW, renderBuf + pos);
                else
                    // Deep out-of-bounds rows are rendered as black void
                    pos += sprintf(renderBuf + pos, "\033[40m \033[0m");
                continue; // Cell resolved
            }

            // For in-bounds cells, read the block data once
            const Block& b = state.world[wy][wx];

            // ── Priority 4: Sky and surface-air cells ────────────────────
            // Delegate sky rendering to the day_night module.
            // Two conditions qualify as sky:
            //   (a) The block type is explicitly BLOCK_SKY
            //   (b) The block is BLOCK_AIR and the world row is at or above
            //       the surface level (handles terrain dips and light cave
            //       internal sky rows)
            if (b.type == BLOCK_SKY || (wy <= SURFACE_LEVEL && b.type == BLOCK_AIR)) {
                pos += renderSkyCell(wy, vx, vpW, renderBuf + pos);
                continue; // Cell resolved — day_night handled the output
            }

            // ── Priority 5: Fog of war — unrevealed blocks ───────────────
            // Hidden blocks are rendered as a dark grey colon ':'.
            // This gives the player a visual cue that unexplored area exists
            // while revealing nothing about the block type underneath.
            if (!b.visible) {
                pos += sprintf(renderBuf + pos, "\033[38;5;236m:\033[0m");
                continue; // Cell resolved
            }

            // ── Priority 6: Red zone tint ────────────────────────────────
            // When an EVENT_RED_ZONE is active, blocks within the event
            // radius (Manhattan distance ≤ ev.radius) are drawn with a
            // bright red background to warn the player of fire damage.
            const RandomEvent& ev = state.activeEvent;
            if (ev.active && ev.type == EVENT_RED_ZONE) {
                // Use Manhattan distance for a diamond-shaped tint region
                int evdx = abs(wx - ev.x);
                int evdy = abs(wy - ev.y);
                if (evdx + evdy <= ev.radius) {
                    // Red background (\033[41m) + bold white foreground
                    pos += sprintf(renderBuf + pos,
                        "\033[41m\033[1;37m%s\033[0m", getBlockChar(b.type));
                    continue; // Cell resolved with tint
                }
            }

            // ── Priority 7: Normal visible block ─────────────────────────
            // The block is visible, in-bounds, and not in a special zone.
            // Render it with its standard colour and character from colors.h.
            pos += sprintf(renderBuf + pos,
                "%s%s\033[0m", getBlockColor(b.type), getBlockChar(b.type));
        }

        // End of viewport row: clear any leftover characters to the EOL,
        // then advance to the next terminal line
        pos += sprintf(renderBuf + pos, "\033[K\n");
    }

    // Erase everything below the viewport rows to remove stale content
    // from a previous frame that may have been taller (e.g., after resize).
    // \033[J — erase from the current cursor position to the end of screen.
    pos += sprintf(renderBuf + pos, "\033[J");

    // Append the HUD lines into the same buffer, continuing from where
    // the viewport rendering left off.  Both sections will be flushed
    // together in the single write() call below.
    pos = appendHUD(renderBuf, pos, state, statusMsg);

    // NUL-terminate the buffer (for safety, though write() uses byte count)
    renderBuf[pos] = '\0';

    // Flush the entire frame to the terminal in one atomic system call.
    // Using write() rather than fwrite/printf avoids stdio buffering and
    // ensures the full frame lands in the terminal buffer simultaneously,
    // which is what eliminates tearing and flicker.
    write(STDOUT_FILENO, renderBuf, pos);
}
