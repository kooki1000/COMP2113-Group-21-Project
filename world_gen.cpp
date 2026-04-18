// =============================================================================
// world_gen.cpp
// TermiCraft — World Generation Module Implementation
//
// Generates the 200×80 world with three biomes:
//   Forest (cols 0–49)   — gentle rolling terrain, dense trees, ores below
//   Cave   (cols 50–139) — sparse surface, massive winding cave with branches
//   Light Cave (cols 140–199) — underground cavern that looks like outdoors,
//                               with sky ceiling, grassy hills, dragon portal
//
// All randomness uses a deterministic hash seeded by state.seed so that
// the same seed always produces the same world.
//
// Author:       Mohit
// Dependencies: world_gen.h, types.h
// =============================================================================

#include "world_gen.h"
#include <cstring>

// ---------------------------------------------------------------------------
// Deterministic hash — replaces rand() everywhere
// ---------------------------------------------------------------------------
static unsigned int worldHash(unsigned int seed, int col, int row) {
    unsigned int h = seed ^ (unsigned int)(col * 2971 + row * 31337);
    h ^= h >> 13;
    h *= 0x5bd1e995;
    h ^= h >> 15;
    return h;
}

static int hashPercent(unsigned int seed, int col, int row) {
    return (int)(worldHash(seed, col, row) % 100);
}

// Second independent hash channel so two checks on the same cell don't collide
static int hashPercent2(unsigned int seed, int col, int row) {
    return (int)(worldHash(seed ^ 0xDEADBEEF, col + 7777, row + 3333) % 100);
}

// ---------------------------------------------------------------------------
// initWorld — allocate the 2-D Block array
// ---------------------------------------------------------------------------
void initWorld(GameState& state) {
    state.worldWidth  = WORLD_WIDTH;
    state.worldHeight = WORLD_HEIGHT;

    state.world = new Block*[state.worldHeight];
    for (int y = 0; y < state.worldHeight; y++) {
        state.world[y] = new Block[state.worldWidth];
    }
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Clamp an int to [lo, hi]
static int clamp(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// Generate a gentle surface heightmap for the above-ground terrain.
// Returns the grass row for each column (the row where BLOCK_GRASS goes).
static void buildSurfaceHeightmap(unsigned int seed, int* heightmap) {
    // Forest (0–49): gentle hills, base around row 8
    heightmap[0] = SURFACE_LEVEL;
    for (int x = 1; x < 50; x++) {
        int r = hashPercent(seed, x, 9999) % 5; // 0-4
        int delta = 0;
        if (r == 0)      delta = -1;  // 20% go up
        else if (r == 1) delta =  1;  // 20% go down
        // else stay same                60% stay

        // Only shift every 3-4 columns for smooth terrain
        if (x % 3 != 0) delta = 0;

        heightmap[x] = clamp(heightmap[x - 1] + delta,
                             SURFACE_LEVEL - 2,   // row 6 min
                             SURFACE_LEVEL + 1);   // row 9 max
    }

    // Cave biome surface (50–139): similar gentle terrain
    heightmap[50] = SURFACE_LEVEL;
    for (int x = 51; x < 140; x++) {
        int r = hashPercent(seed, x, 9998) % 5;
        int delta = 0;
        if (r == 0)      delta = -1;
        else if (r == 1) delta =  1;
        if (x % 4 != 0) delta = 0;

        heightmap[x] = clamp(heightmap[x - 1] + delta,
                             SURFACE_LEVEL - 1,
                             SURFACE_LEVEL + 1);
    }

    // Light Cave surface (140–199): same as cave biome surface
    heightmap[140] = heightmap[139];
    for (int x = 141; x < 200; x++) {
        int r = hashPercent(seed, x, 9997) % 5;
        int delta = 0;
        if (r == 0)      delta = -1;
        else if (r == 1) delta =  1;
        if (x % 4 != 0) delta = 0;

        heightmap[x] = clamp(heightmap[x - 1] + delta,
                             SURFACE_LEVEL - 1,
                             SURFACE_LEVEL + 1);
    }
}

// Place a tree rooted at (grassRow-1, col).  Returns true if placed.
// trunkHeight = how many BLOCK_WOOD cells (4 for surface, 2-3 for cave/light)
// Checks bounds and 2-col spacing via lastTreeCol.
static bool placeTree(Block** world, int worldW, int worldH,
                      int col, int grassRow, int trunkHeight, int& lastTreeCol) {
    // Spacing rule: at least 2 cols from the last tree
    if (col - lastTreeCol < 3) return false;

    int trunkBase = grassRow - 1;  // first wood block row (just above grass)
    int trunkTop  = trunkBase - trunkHeight + 1;

    // Leaves cap is 3×3 centered one row above trunk top
    int leavesCenter = trunkTop - 1;

    // Bounds check
    if (leavesCenter - 1 < 0) return false;
    if (col - 1 < 0 || col + 1 >= worldW) return false;

    // Make sure we aren't overwriting solid blocks with the trunk
    for (int y = trunkTop; y <= trunkBase; y++) {
        if (y < 0 || y >= worldH) return false;
        BlockType t = world[y][col].type;
        if (t != BLOCK_AIR && t != BLOCK_SKY) return false;
    }

    // Place trunk
    for (int y = trunkTop; y <= trunkBase; y++) {
        world[y][col].type = BLOCK_WOOD;
    }

    // Place 3×3 leaves cap
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int lx = col + dx;
            int ly = leavesCenter + dy;
            if (lx < 0 || lx >= worldW || ly < 0 || ly >= worldH) continue;
            BlockType t = world[ly][lx].type;
            if (t == BLOCK_AIR || t == BLOCK_SKY) {
                world[ly][lx].type = BLOCK_LEAVES;
            }
        }
    }

    lastTreeCol = col;
    return true;
}

// Determine ore type for a given cell (diamond > gold > iron).
// Returns the block type, or BLOCK_STONE if no ore.
static BlockType pickOre(unsigned int seed, int col, int row) {
    if (row >= 50) { // DIAMOND_LEVEL
        int r = hashPercent(seed, col, row);
        if (r < 2) return BLOCK_DIAMOND;  // 2%
    }
    if (row >= 30) { // GOLD_LEVEL
        int r = hashPercent2(seed, col, row);
        if (r < 3) return BLOCK_GOLD;     // 3%
    }
    if (row >= 12) { // STONE_LEVEL
        int r = hashPercent(seed, col + 5000, row + 5000);
        if (r < 15) return BLOCK_IRON;    // 15%
    }
    return BLOCK_STONE;
}

// ---------------------------------------------------------------------------
// generateWorld — the main generation routine
// ---------------------------------------------------------------------------
void generateWorld(GameState& state) {
    unsigned int seed = state.seed;
    int W = state.worldWidth;
    int H = state.worldHeight;
    Block** world = state.world;

    // --------------------------------------------------
    // STEP 0: Default everything to BLOCK_STONE, hidden
    // --------------------------------------------------
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            world[y][x].type    = BLOCK_STONE;
            world[y][x].mined   = false;
            world[y][x].visible = false;
        }
    }

    // --------------------------------------------------
    // STEP 1: Build surface heightmap
    // --------------------------------------------------
    int heightmap[WORLD_WIDTH];
    buildSurfaceHeightmap(seed, heightmap);

    // --------------------------------------------------
    // STEP 2: Fill sky, grass, dirt for all columns
    // --------------------------------------------------
    for (int x = 0; x < W; x++) {
        int grassRow = heightmap[x];

        // Sky above grass
        for (int y = 0; y < grassRow; y++) {
            world[y][x].type = BLOCK_SKY;
        }

        // Grass
        world[grassRow][x].type = BLOCK_GRASS;

        // Dirt below grass (3 rows)
        for (int y = grassRow + 1; y < grassRow + 4 && y < H; y++) {
            if (y < 12) { // only dirt above stone level
                world[y][x].type = BLOCK_DIRT;
            }
        }

        // Stone + ores from STONE_LEVEL downward
        for (int y = 12; y < H - 1; y++) { // leave row 79 for bedrock
            // If it was already set to dirt, skip
            if (world[y][x].type == BLOCK_DIRT) continue;
            world[y][x].type = pickOre(seed, x, y);
        }

        // Bedrock at bottom
        world[H - 1][x].type = BLOCK_BEDROCK;
    }

    // --------------------------------------------------
    // STEP 3: Visibility — rows 0 through max surface level visible
    // --------------------------------------------------
    int maxSurface = SURFACE_LEVEL + 2; // a bit below the lowest grass
    for (int y = 0; y <= maxSurface; y++) {
        for (int x = 0; x < W; x++) {
            world[y][x].visible = true;
        }
    }

    // --------------------------------------------------
    // STEP 4: Trees — Forest (20%), Cave surface (10%), Light Cave surface (10%)
    // --------------------------------------------------
    {
        int lastTree = -10;

        // Forest trees (20%)
        for (int x = 0; x < 50; x++) {
            int grassRow = heightmap[x];
            int chance = hashPercent(seed, x, 8888);
            if (chance < 20) {
                placeTree(world, W, H, x, grassRow, 4, lastTree);
            }
        }

        // Cave biome surface trees (10%)
        lastTree = 47; // reset spacing for new biome
        for (int x = 50; x < 140; x++) {
            int grassRow = heightmap[x];
            int chance = hashPercent(seed, x, 8888);
            if (chance < 10) {
                placeTree(world, W, H, x, grassRow, 4, lastTree);
            }
        }

        // Light Cave surface trees (10%)
        lastTree = 137;
        for (int x = 140; x < 200; x++) {
            int grassRow = heightmap[x];
            int chance = hashPercent(seed, x, 8888);
            if (chance < 10) {
                placeTree(world, W, H, x, grassRow, 4, lastTree);
            }
        }
    }

    // --------------------------------------------------
    // STEP 5: Main cave tunnel (Cave biome, cols 50–139)
    // Enters at ~col 50, row 20.  Exits at ~col 139, row 45.
    // --------------------------------------------------
    int cavePath[200]; // center row of cave at each column
    memset(cavePath, 0, sizeof(cavePath));

    {
        int centerY = 20; // start row
        int targetY = 45; // end row at col 139

        for (int x = 50; x <= 139; x++) {
            cavePath[x] = centerY;

            // Every 4 columns, shift center toward the exit row
            if ((x - 50) % 4 == 0 && x < 139) {
                int r = hashPercent(seed, x, 7777) % 5;
                int delta = r - 2; // -2, -1, 0, +1, +2

                // Bias toward targetY
                float progress = (float)(x - 50) / 89.0f;
                int ideal = 20 + (int)(progress * (targetY - 20));
                if (centerY < ideal - 2) delta = clamp(delta, 0, 2);
                if (centerY > ideal + 2) delta = clamp(delta, -2, 0);

                centerY = clamp(centerY + delta, 15, 60);
            }
        }

        // Carve the main tunnel: 6 wide, 5 tall
        for (int x = 50; x <= 139; x++) {
            int cy = cavePath[x];
            int halfW = 3;
            int halfH = 2;

            // Widen entrance at cols 50–53
            if (x <= 53) {
                halfH = 4; // 8 tiles tall
                halfW = 4;
            }

            for (int dy = -halfH; dy <= halfH; dy++) {
                for (int dx = -halfW; dx <= halfW; dx++) {
                    int wx = x + dx;
                    int wy = cy + dy;
                    if (wx < 0 || wx >= W || wy < 0 || wy >= H - 1) continue;
                    if (world[wy][wx].type == BLOCK_BEDROCK) continue;
                    world[wy][wx].type = BLOCK_AIR;
                }
            }
        }

        // Add floor surfaces inside the cave (grass on stone below air)
        for (int x = 50; x <= 139; x++) {
            int cy = cavePath[x];
            int floorY = cy + 3; // bottom of the 5-tall tunnel
            if (floorY >= 0 && floorY < H - 1) {
                // If below is stone and above is air, place grass
                if (world[floorY][x].type == BLOCK_STONE ||
                    world[floorY][x].type == BLOCK_IRON ||
                    world[floorY][x].type == BLOCK_GOLD) {
                    if (floorY - 1 >= 0 && world[floorY - 1][x].type == BLOCK_AIR) {
                        world[floorY][x].type = BLOCK_GRASS;
                    }
                }
            }
        }
    }

    // --------------------------------------------------
    // STEP 6: Branch tunnels off the main cave (4–6 branches)
    // --------------------------------------------------
    {
        int numBranches = 4 + (int)(worldHash(seed, 12345, 67890) % 3); // 4-6

        for (int b = 0; b < numBranches; b++) {
            // Pick a starting column along the main cave
            int startX = 55 + (int)(worldHash(seed, b * 137, 4444) % 80);
            if (startX > 135) startX = 135;
            int startY = cavePath[startX];

            // Direction: pick from a set of diagonal/horizontal directions
            int dirSeed = (int)(worldHash(seed, b * 271, 5555) % 4);
            int bdx, bdy;
            switch (dirSeed) {
                case 0: bdx =  1; bdy = -1; break; // up-right
                case 1: bdx =  1; bdy =  1; break; // down-right
                case 2: bdx = -1; bdy = -1; break; // up-left
                case 3: bdx = -1; bdy =  1; break; // down-left
            }

            int branchLen = 20 + (int)(worldHash(seed, b * 311, 6666) % 11); // 20-30

            int bx = startX;
            int by = startY;
            for (int step = 0; step < branchLen; step++) {
                // Carve 3 wide, 2 tall
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int wx = bx + dx;
                        int wy = by + dy;
                        if (wx < 50 || wx > 139) continue; // stay in biome
                        if (wy < 1 || wy >= H - 1) continue;
                        if (world[wy][wx].type == BLOCK_BEDROCK) continue;
                        world[wy][wx].type = BLOCK_AIR;
                    }
                }

                bx += bdx;
                by += (step % 2 == 0) ? bdy : 0; // diagonal every other step

                if (bx < 50 || bx > 139 || by < 13 || by >= H - 2) break;
            }
        }
    }

    // --------------------------------------------------
    // STEP 7: Cave underground trees (5% on cave floor grass)
    // --------------------------------------------------
    {
        int lastTree = 47;
        for (int x = 50; x <= 139; x++) {
            if (world[cavePath[x] + 3][x].type != BLOCK_GRASS) continue;
            int chance = hashPercent(seed, x, 6543);
            if (chance < 5) {
                int grassRow = cavePath[x] + 3;
                int trunkH = 2 + (int)(worldHash(seed, x, 6544) % 2); // 2-3
                placeTree(world, W, H, x, grassRow, trunkH, lastTree);
            }
        }
    }

    // --------------------------------------------------
    // STEP 8: Light Cave (cols 140–199) — underground cavern
    // --------------------------------------------------

    // Carve the cavern hollow: rows 15–71 become AIR
    for (int y = 15; y <= 71; y++) {
        for (int x = 140; x < 200; x++) {
            if (world[y][x].type == BLOCK_BEDROCK) continue;
            world[y][x].type = BLOCK_AIR;
        }
    }

    // Stone ceiling: rows 12–14 stay as stone (they already are)
    // Stone floor shell: rows 72–74 stay as stone (they already are)

    // Sky layer inside the cavern: rows 15–22
    for (int y = 15; y <= 22; y++) {
        for (int x = 140; x < 200; x++) {
            world[y][x].type = BLOCK_SKY;
        }
    }

    // Cloud clusters (3–4 clusters of 3×1 BLOCK_SKY patches in rows 16–20)
    {
        int numClouds = 3 + (int)(worldHash(seed, 9001, 9001) % 2);
        for (int c = 0; c < numClouds; c++) {
            int cx = 145 + (int)(worldHash(seed, c * 431, 9002) % 48);
            int cy = 16 + (int)(worldHash(seed, c * 557, 9003) % 5);
            for (int dx = 0; dx < 3; dx++) {
                int wx = cx + dx;
                if (wx >= 140 && wx < 200 && cy >= 15 && cy <= 22) {
                    world[cy][wx].type = BLOCK_SKY;
                }
            }
        }
    }

    // Hilly floor inside the cavern
    int lightCaveFloor[200];
    memset(lightCaveFloor, 0, sizeof(lightCaveFloor));
    {
        int floorY = 55; // starting height
        for (int x = 140; x < 200; x++) {
            lightCaveFloor[x] = floorY;

            if ((x - 140) % 4 == 0 && x < 199) {
                int r = hashPercent(seed, x, 8000) % 3;
                int delta = r - 1; // -1, 0, +1
                floorY = clamp(floorY + delta, 50, 62);
            }
        }

        // Place grass, dirt, stone for the hills
        for (int x = 140; x < 200; x++) {
            int gy = lightCaveFloor[x];

            // Grass at the top
            world[gy][x].type = BLOCK_GRASS;

            // Dirt for 3 rows below
            for (int d = 1; d <= 3; d++) {
                if (gy + d < 72) {
                    world[gy + d][x].type = BLOCK_DIRT;
                }
            }

            // Stone below dirt to row 71
            for (int y = gy + 4; y <= 71; y++) {
                world[y][x].type = BLOCK_STONE;
            }
        }
    }

    // Trees on the Light Cave floor (5%)
    {
        int lastTree = 137;
        for (int x = 140; x < 200; x++) {
            int gy = lightCaveFloor[x];
            int chance = hashPercent(seed, x, 7000);
            if (chance < 5) {
                int trunkH = 2 + (int)(worldHash(seed, x, 7001) % 2); // 2-3
                placeTree(world, W, H, x, gy, trunkH, lastTree);
            }
        }
    }

    // Dragon portal at column 170, centered on local floor height
    {
        int portalCol = 170;
        int floorRow  = lightCaveFloor[portalCol];

        // 3 wide × 4 tall portal, bottom sits on the grass row
        int portalTop = floorRow - 4;
        for (int dy = 0; dy < 4; dy++) {
            for (int dx = 0; dx < 3; dx++) {
                int px = portalCol + dx;
                int py = portalTop + dy + 1; // +1 so bottom row = floorRow
                if (px >= 0 && px < W && py >= 0 && py < H) {
                    world[py][px].type = BLOCK_DRAGON_CAVE;
                }
            }
        }

        // Clear tile in front of the portal (col 169, floor row)
        if (portalCol - 1 >= 0) {
            world[floorRow][portalCol - 1].type = BLOCK_AIR;
            // Also clear a couple tiles above for walking clearance
            if (floorRow - 1 >= 0) world[floorRow - 1][portalCol - 1].type = BLOCK_AIR;
            if (floorRow - 2 >= 0) world[floorRow - 2][portalCol - 1].type = BLOCK_AIR;
        }
    }

    // --------------------------------------------------
    // STEP 9: Connect main cave to Light Cave
    // The main cave exits at ~col 139, row cavePath[139].
    // Carve a passage from col 139 to col 142, matching heights.
    // --------------------------------------------------
    {
        int exitY = cavePath[139];
        int entryY = 40; // roughly mid-cavern, above the hills

        // Smooth transition from exit to Light Cave interior
        for (int x = 137; x <= 145; x++) {
            float t = (float)(x - 137) / 8.0f;
            int cy = exitY + (int)(t * (entryY - exitY));

            for (int dy = -3; dy <= 3; dy++) {
                int wy = cy + dy;
                if (wy < 1 || wy >= H - 1) continue;
                if (x < 0 || x >= W) continue;
                if (world[wy][x].type == BLOCK_BEDROCK) continue;
                world[wy][x].type = BLOCK_AIR;
            }
        }
    }

    // --------------------------------------------------
    // STEP 10: Forest cave entrance visible from surface
    // Carve a gentle slope from the forest surface down to the cave entrance
    // at col 50, row 20.  This makes the cave mouth visible.
    // --------------------------------------------------
    {
        int surfaceY = heightmap[49]; // grass row at forest edge
        int caveEntryY = cavePath[50];

        // Slope from col 45 to col 53
        for (int x = 45; x <= 53; x++) {
            float t = (float)(x - 45) / 8.0f;
            int cy = surfaceY + (int)(t * (caveEntryY - surfaceY));

            int halfH = 2 + (int)(t * 2); // gets taller as we go deeper
            for (int dy = -halfH; dy <= halfH; dy++) {
                int wy = cy + dy;
                if (wy < 1 || wy >= H - 1) continue;
                if (x < 0 || x >= W) continue;
                if (world[wy][x].type == BLOCK_BEDROCK) continue;
                // Don't carve through the sky/surface layer too aggressively
                if (wy < surfaceY - 1 && x < 48) continue;
                world[wy][x].type = BLOCK_AIR;
            }
        }
    }
}
