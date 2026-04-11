/*
 * fog_of_war.cpp
 *
 * Fog of War visibility system and world rendering for TermiCraft.
 * Uses buffered output to prevent terminal flicker.
 *
 * Author: Mohit
 */

#include "fog_of_war.h"
#include "day_night.h"
#include "colors.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>

// ----- VISIBILITY RADIUS BY DEPTH -----

int getVisibilityRadius(int worldY) {
    if (worldY < STONE_LEVEL) {
        return -1;  // surface + dirt: always visible
    } else if (worldY < DEEP_LEVEL) {
        return 3;   // underground / stone layer
    } else {
        return 2;   // deep layer
    }
}

// ----- UPDATE VISIBILITY -----

void updateWorldVisibility(GameState& state) {
    int px = state.player.pos.x;
    int py = state.player.pos.y;

    // Surface/dirt rows: always fully visible within viewport
    int camX = state.camera.x;
    int camY = state.camera.y;
    for (int vy = 0; vy < state.viewportHeight; vy++) {
        int wy = camY + vy;
        if (wy < 0 || wy >= state.worldHeight) continue;
        if (wy < STONE_LEVEL) {
            for (int vx = 0; vx < state.viewportWidth; vx++) {
                int wx = camX + vx;
                if (wx >= 0 && wx < state.worldWidth)
                    state.world[wy][wx].visible = true;
            }
        }
    }

    // Underground/deep: circular reveal around player, permanent
    int radius = getVisibilityRadius(py);
    if (radius > 0) {
        int r2 = radius * radius;
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                if (dx * dx + dy * dy > r2) continue;
                int wx = px + dx;
                int wy = py + dy;
                if (wx >= 0 && wx < state.worldWidth &&
                    wy >= 0 && wy < state.worldHeight) {
                    state.world[wy][wx].visible = true;
                }
            }
        }
    }
}

// ----- BUFFERED WORLD RENDERING -----
// Everything goes into buf[] first, then one write() call. No flicker.

// Max buffer: 80 cols * 40 rows * ~30 bytes per cell (escape codes) + HUD
static char renderBuf[500000];

void renderWorld(const GameState& state) {
    int pos = 0;

    // Cursor home (overwrite in place, not scroll)
    const char* home = "\033[H";
    int homeLen = strlen(home);
    memcpy(renderBuf + pos, home, homeLen);
    pos += homeLen;

    int camX = state.camera.x;
    int camY = state.camera.y;
    int vpW  = state.viewportWidth;
    int vpH  = state.viewportHeight;

    for (int vy = 0; vy < vpH; vy++) {
        int wy = camY + vy;

        for (int vx = 0; vx < vpW; vx++) {
            int wx = camX + vx;

            // Player
            if (wx == state.player.pos.x && wy == state.player.pos.y) {
                pos += sprintf(renderBuf + pos,
                    "\033[38;5;226m@\033[0m");
                continue;
            }

            // Out of bounds
            if (wx < 0 || wx >= state.worldWidth ||
                wy < 0 || wy >= state.worldHeight) {
                renderBuf[pos++] = ' ';
                continue;
            }

            const Block& b = state.world[wy][wx];

            // Sky cells — delegate to day_night module
            if (wy < SURFACE_LEVEL && (b.type == BLOCK_SKY || b.type == BLOCK_AIR)) {
                pos += renderSkyCell(wy, vx, vpW, renderBuf + pos);
                continue;
            }

            // Fog — unrevealed blocks
            if (!b.visible) {
                pos += sprintf(renderBuf + pos,
                    "\033[38;5;236m:\033[0m");
                continue;
            }

            // Visible block with color
            pos += sprintf(renderBuf + pos,
                "%s%c\033[0m", getBlockColor(b.type), getBlockChar(b.type));
        }

        renderBuf[pos++] = '\n';
    }

    renderBuf[pos] = '\0';
    write(STDOUT_FILENO, renderBuf, pos);
}
