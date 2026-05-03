// =============================================================================
// world_gen.cpp
// TermiCraft — World Generation Module Implementation
//
// Generates the 200×80 world with three biomes:
//   Forest (cols 0–49)   — dramatic hilly terrain, dense trees, ores below
//   Cave   (cols 50–139) — sparse surface, massive winding cave with branches
//   Light Cave (cols 140–199) — underground cavern with dragon portal
//
// All randomness uses a deterministic hash seeded by state.seed so that
// the same seed always produces the same world.
//
// Author:       Mohit
// Dependencies: world_gen.h, types.h
//
// =============================================================================
// INTERNAL FUNCTION REFERENCE
// =============================================================================
//
//   worldHash(seed, col, row)  [static]
//       Core pseudo-random hash.  Takes a seed and two integer coordinates,
//       mixes them with prime multipliers and XOR-shift operations (similar
//       to MurmurHash2), and returns a 32-bit unsigned integer.  All other
//       random functions in this file derive from this one.
//       Used by: hashPercent, hashPercent2, normalTrunkHeight, smoothNoiseWG,
//                pickOre, placeTree, generateWorld (tree/cave/ore decisions).
//
//   hashPercent(seed, col, row)  [static]
//       Returns worldHash(seed, col, row) % 100 as an int in [0, 99].
//       Convenient for probability checks (e.g., "if hashPercent < 20 place tree").
//       Used as the primary random channel for per-cell decisions.
//
//   hashPercent2(seed, col, row)  [static]
//       A second independent hash channel — XORs the seed with 0xDEADBEEF
//       and offsets col/row by large constants before calling worldHash().
//       This ensures that two probability checks at the same cell (e.g.,
//       "is this gold?" and "is this iron?") do not collide statistically.
//
//   normalTrunkHeight(seed, col)  [static]
//       Approximates a normally distributed trunk height using the Central
//       Limit Theorem: sums 6 uniform random values in [0, 5], which
//       produces a near-normal distribution with mean ~15 and SD ~3.5.
//       Linearly maps the result to the range [3, 8].
//       Used in the forest tree placement pass of generateWorld().
//
//   clamp(v, lo, hi)  [static]
//       Integer clamp helper.  Returns v clamped to [lo, hi] inclusive.
//       Used throughout the generation steps to keep coordinates in bounds.
//
//   smoothNoiseWG(seed, x)  [static]
//       One-dimensional smooth noise using smoothstep interpolation between
//       adjacent integer-lattice random values.  The smoothstep function
//       (3t² − 2t³) produces C¹ continuity at lattice points, avoiding the
//       sharp transitions that linear interpolation would cause.
//       Returns a float in approximately [0.0, 1.0].
//       Used as the building block for terrainHeightWG().
//
//   terrainHeightWG(seed, x)  [static]
//       Three-octave fractal noise (fBm) built from smoothNoiseWG().
//       Octave frequencies and amplitudes:
//         Octave 1: frequency 0.02, amplitude 6.0  (large hills)
//         Octave 2: frequency 0.05, amplitude 3.0  (medium bumps)
//         Octave 3: frequency 0.10, amplitude 1.5  (fine detail)
//       Output range is approximately 0.0–10.5.
//       Used by buildSurfaceHeightmap() to create the terrain profile.
//
//   buildSurfaceHeightmap(seed, heightmap, worldW, forestEnd, caveEnd)  [static]
//       Fills the heightmap array (one entry per world column) with the
//       Y row index of the grass block for that column.  Biome-specific
//       amplitude multipliers and clamp ranges shape each region:
//         Forest    — multiplier 1.2, hills ±6 rows from SURFACE_LEVEL
//         Cave      — multiplier 0.5, hills ±2 rows
//         LightCave — multiplier 0.3, gentle slope ±1/+2 rows
//
//   placeTree(world, worldW, worldH, col, grassRow, trunkHeight, lastTreeCol)  [static]
//       Attempts to place a tree at column col with a trunk of trunkHeight
//       cells.  Returns false (and places nothing) if:
//         — Fewer than 3 columns have elapsed since the last tree (spacing rule)
//         — The trunk extends above row 0 (top of world)
//         — The tree would clip the left or right world edge
//         — Any trunk cell already holds a non-air/non-sky block
//       If placement succeeds:
//         — Trunk cells (grassRow-trunkHeight … grassRow-1) → BLOCK_WOOD
//         — Canopy cells (diamond pattern centred on the top trunk cell,
//           radius 3, dy in [−2, 0]) → BLOCK_LEAVES (only overwrites air/sky)
//         — Updates lastTreeCol to col so spacing is enforced for the next call
//
//   pickOre(seed, col, row)  [static]
//       Chooses the block type for a stone cell at (col, row) based on depth:
//         row >= 50 → 2% chance of BLOCK_DIAMOND  (deepest, rarest)
//         row >= 30 → 3% chance of BLOCK_GOLD     (deep, rare)
//         row >= 12 → 15% chance of BLOCK_IRON    (mid-depth, common)
//         else      → BLOCK_STONE                 (no ore)
//       Multiple checks use independent hash channels (hashPercent vs
//       hashPercent2) to prevent diamond/gold checks from correlating.
//
//   initWorld(state)  [public — declared in world_gen.h]
//       See world_gen.h for the full specification.
//       Allocates state.world as Block**[worldHeight][worldWidth] on the heap.
//
//   generateWorld(state)  [public — declared in world_gen.h]
//       See world_gen.h for the nine-step generation pipeline overview.
//       Calls the helpers above in sequence to build the complete world.
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//   world_gen.h  — declares initWorld() and generateWorld()
//   types.h      — Block, BlockType, GameState, WORLD_WIDTH, WORLD_HEIGHT,
//                  SURFACE_LEVEL, STONE_LEVEL, DEEP_LEVEL, BLOCK_* constants
//   <algorithm>  — for std::max()
//   <cstring>    — for memset() (not directly called but included transitively)
//   <cmath>      — for sqrtf (included for reference; actual code uses integer math)
//   <vector>     — for std::vector<int> heightmap and cavePath buffers
// =============================================================================

#include "world_gen.h"
#include <algorithm>
#include <cstring>
#include <cmath>   // for sqrtf / normal-distribution approximation
#include <vector>

// =============================================================================
// SECTION 1 — DETERMINISTIC HASH FUNCTIONS
// =============================================================================

// -----------------------------------------------------------------------------
// worldHash
//
// The foundational pseudo-random function for the entire world generator.
// Given a seed and two integer coordinates (col, row), returns a 32-bit
// unsigned integer that is effectively random but perfectly reproducible.
//
// Algorithm:
//   1. XOR the seed with a linear combination of col and row weighted by
//      large primes (2971, 31337) to embed the spatial coordinates into h.
//   2. XOR-shift right by 13 to avalanche high bits into lower positions.
//   3. Multiply by 0x5bd1e995 (the MurmurHash2 mixing constant) to spread
//      bits across the full 32-bit range.
//   4. XOR-shift right by 15 for a final avalanche pass.
//
// The result has no visible statistical patterns for nearby (col, row) pairs,
// making it suitable for per-cell tile decisions.
// -----------------------------------------------------------------------------
static unsigned int worldHash(unsigned int seed, int col, int row) {
    // Combine seed with spatially distinct contributions from col and row
    // The large prime multipliers reduce axis-alignment artefacts
    unsigned int h = seed ^ (unsigned int)(col * 2971 + row * 31337);

    // First avalanche: spread high-bit information downward
    h ^= h >> 13;

    // Multiplicative mix: near-prime constant ensures good bit distribution
    h *= 0x5bd1e995;

    // Second avalanche: clean up any remaining low-bit bias
    h ^= h >> 15;

    return h;
}

// -----------------------------------------------------------------------------
// hashPercent
//
// Returns a value in [0, 99] suitable for percentage-based probability checks.
// Uses the primary hash channel (unmodified seed).
//
// Example usage:
//   if (hashPercent(seed, x, y) < 20) { placeTree(...); }  // 20% chance
// -----------------------------------------------------------------------------
static int hashPercent(unsigned int seed, int col, int row) {
    // Reduce the full 32-bit hash to the [0, 99] range via modulus
    return (int)(worldHash(seed, col, row) % 100);
}

// -----------------------------------------------------------------------------
// hashPercent2
//
// A second independent hash channel for the same (col, row) cell.
// XORing the seed with 0xDEADBEEF and offsetting col/row by large constants
// before calling worldHash() ensures this channel produces uncorrelated
// outputs from hashPercent() at the same coordinates.
//
// This matters when two independent probability checks are needed for the
// same cell — e.g., "is this gold?" and "is this diamond?" — because using
// the same hash for both would cause them to always agree or always disagree.
// -----------------------------------------------------------------------------
static int hashPercent2(unsigned int seed, int col, int row) {
    // Use a perturbed seed and offset coordinates for independence from channel 1
    return (int)(worldHash(seed ^ 0xDEADBEEF, col + 7777, row + 3333) % 100);
}

// =============================================================================
// SECTION 2 — TREE HEIGHT DISTRIBUTION
// =============================================================================

// -----------------------------------------------------------------------------
// normalTrunkHeight
//
// Generates a trunk height that follows an approximately normal distribution
// by exploiting the Central Limit Theorem: the sum of n uniform random
// variables approaches a normal distribution as n grows.
//
// Here n = 6 uniform values in [0, 5], giving:
//   Sum range : [0, 30]
//   Mean      : 15  (midpoint of range)
//   SD        : approximately 3.5
//
// The sum is linearly mapped to the target range [3, 8]:
//   sum = 0  → height ≈ 3  (shortest possible trunk)
//   sum = 15 → height ≈ 5  (most common — near the mean)
//   sum = 30 → height ≈ 8  (tallest possible trunk)
//
// The final value is clamped to [3, 8] to handle rounding edge cases.
//
// Parameters:
//   seed — world seed (passed through to worldHash for reproducibility)
//   col  — the column the tree is being placed in (used to vary per-column)
//
// Returns:
//   An integer in [3, 8] representing the number of trunk blocks.
// -----------------------------------------------------------------------------
static int normalTrunkHeight(unsigned int seed, int col) {
    // Sum 6 values in [0,5] → range [0,30], mean ~15, sd ~3.5
    int sum = 0;
    for (int i = 0; i < 6; i++) {
        // Each call uses a different hash input (col * 7 + i) to get independent samples
        sum += (int)(worldHash(seed, col * 7 + i, 9999) % 6);
    }

    // Map [0,30] → mean=5, clamp [3,8]:
    //   sum=15 → 5,  sum=0 → ~2,  sum=30 → ~8
    int h = 3 + (sum * 5) / 30;   // linear map: 0→3, 30→8

    // Clamp to the valid trunk height range to account for rounding
    if (h < 3) h = 3;
    if (h > 8) h = 8;
    return h;
}

// =============================================================================
// SECTION 3 — WORLD ALLOCATION
// =============================================================================

// -----------------------------------------------------------------------------
// initWorld  (PUBLIC)
//
// Allocates the two-dimensional Block array on the heap and stores it in
// state.world.  Sets state.worldWidth and state.worldHeight accordingly.
//
// worldWidth is derived from the viewport width if it has been set, otherwise
// it falls back to the WORLD_WIDTH constant from types.h.  worldHeight always
// equals WORLD_HEIGHT (a fixed constant).
//
// The allocation uses a pointer-to-pointer (Block**) scheme:
//   state.world         → array of (worldHeight) Block* pointers
//   state.world[y]      → array of (worldWidth)  Block  structs for row y
//   state.world[y][x]   → the Block at column x, row y
//
// After returning, all Block fields are zero-initialised by the default
// Block constructor (type = 0, mined = false, visible = false).
// generateWorld() must be called next to fill the content.
// -----------------------------------------------------------------------------
void initWorld(GameState& state) {
    // Determine world width: prefer the viewport width if it has been set
    state.worldWidth  = (state.viewportWidth > 0) ? state.viewportWidth : WORLD_WIDTH;

    // World height is always the fixed constant from types.h
    state.worldHeight = WORLD_HEIGHT;

    // Allocate the row-pointer array (outer dimension)
    state.world = new Block*[state.worldHeight];

    // Allocate each row as a separate array (inner dimension)
    for (int y = 0; y < state.worldHeight; y++) {
        state.world[y] = new Block[state.worldWidth];
    }
}

// =============================================================================
// SECTION 4 — INTERNAL HELPERS
// =============================================================================

// -----------------------------------------------------------------------------
// clamp
//
// Returns v clamped to the closed interval [lo, hi].
// Used throughout the generation code to keep row/column indices in bounds
// without verbose if-else chains.
// -----------------------------------------------------------------------------
static int clamp(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// =============================================================================
// SECTION 5 — SMOOTH NOISE FOR TERRAIN
// =============================================================================

// -----------------------------------------------------------------------------
// smoothNoiseWG
//
// One-dimensional lattice noise with smoothstep interpolation.  Samples
// pseudo-random values at integer lattice points and interpolates between
// them using the smoothstep polynomial (3t² − 2t³).
//
// Why smoothstep instead of linear interpolation?
//   Linear interpolation produces visible "kinks" (C⁰ continuity) at each
//   integer lattice point.  Smoothstep has zero derivative at t=0 and t=1,
//   producing C¹ continuity — the resulting noise has no sharp corners.
//
// Parameters:
//   seed — world seed, ensures different seeds produce different terrains
//   x    — floating-point sample position along the noise axis
//
// Returns:
//   A float in approximately [0.0, 1.0], smoothly varying with x.
// -----------------------------------------------------------------------------
static float smoothNoiseWG(unsigned int seed, float x) {
    // Split x into integer part (ix) and fractional part (frac)
    int ix = (int)x;
    // Correct integer part for negative x (C truncation rounds toward zero)
    if (x < 0.0f) ix--;
    float frac = x - (float)ix;

    // Apply smoothstep: maps frac in [0,1] to t in [0,1] with zero-derivative endpoints
    float t = frac * frac * (3.0f - 2.0f * frac); // Smoothstep S-curve

    // Sample pseudo-random lattice values at ix and ix+1
    float a = (float)(worldHash(seed, ix,     0) % 1000) / 1000.0f;
    float b = (float)(worldHash(seed, ix + 1, 0) % 1000) / 1000.0f;

    // Linearly interpolate using the smoothstep weight
    return a + t * (b - a);
}

// -----------------------------------------------------------------------------
// terrainHeightWG
//
// Three-octave fractal Brownian motion (fBm) noise for terrain elevation.
// Combines three calls to smoothNoiseWG() at increasing frequencies and
// decreasing amplitudes:
//
//   Octave 1: 0.02 frequency, 6.0 amplitude  — broad rolling hills
//   Octave 2: 0.05 frequency, 3.0 amplitude  — medium-scale bumps
//   Octave 3: 0.10 frequency, 1.5 amplitude  — fine surface roughness
//
// Each octave uses a different seed offset (seed, seed+100, seed+200) so
// the three octaves are statistically independent.
//
// The output range is approximately 0.0–10.5 (sum of amplitudes = 10.5 max).
// Callers then scale and clamp this value into a world-row offset.
// -----------------------------------------------------------------------------
static float terrainHeightWG(unsigned int seed, int x) {
    float h = 0.0f;

    // Octave 1 — dominant hills (largest wavelength, highest amplitude)
    h += smoothNoiseWG(seed,       x * 0.02f) * 6.0f;

    // Octave 2 — secondary bumps (medium wavelength, medium amplitude)
    h += smoothNoiseWG(seed + 100, x * 0.05f) * 3.0f;

    // Octave 3 — fine detail (smallest wavelength, smallest amplitude)
    h += smoothNoiseWG(seed + 200, x * 0.10f) * 1.5f;

    return h; // range roughly 0–10.5
}

// =============================================================================
// SECTION 6 — SURFACE HEIGHTMAP
// =============================================================================

// -----------------------------------------------------------------------------
// buildSurfaceHeightmap
//
// Fills the heightmap[] array (indexed by world column x) with the Y row
// index of the grass block for that column.  Lower Y = higher terrain
// (Y increases downward in TermiCraft).
//
// Biome-specific parameters control how dramatic the terrain is:
//
//   Forest (x <= forestEnd):
//     Multiplier 1.2 — amplifies noise to create dramatic hills.
//     Offset -5 centres the variation around SURFACE_LEVEL.
//     Clamped to [SURFACE_LEVEL-6, SURFACE_LEVEL+4] — allows peaks 6 rows
//     above and valleys 4 rows below the nominal surface.
//
//   Cave (forestEnd < x <= caveEnd):
//     Multiplier 0.5 — moderate hills, less dramatic than forest.
//     Offset -1.
//     Clamped to [SURFACE_LEVEL-2, SURFACE_LEVEL+2].
//
//   LightCave (x > caveEnd):
//     Multiplier 0.3 — very gentle undulation.
//     No offset.
//     Clamped to [SURFACE_LEVEL-1, SURFACE_LEVEL+2].
//
// Parameters:
//   seed      — world seed (passed to terrainHeightWG)
//   heightmap — output array of length worldW, allocated by the caller
//   worldW    — total world width in columns
//   forestEnd — last column of the forest biome
//   caveEnd   — last column of the cave biome
// -----------------------------------------------------------------------------
static void buildSurfaceHeightmap(unsigned int seed, int* heightmap, int worldW,
                                  int forestEnd, int caveEnd) {
    for (int x = 0; x < worldW; x++) {
        // Sample the three-octave terrain noise at this column
        float baseH = terrainHeightWG(seed, x);

        if (x <= forestEnd) {
            // Forest — dramatic rolling hills matching the demo screenshot
            // baseH is [0..10.5]; multiply by 1.2 gives [0..12.6], offset by -5
            int offset = (int)(baseH * 1.2f) - 5;
            heightmap[x] = clamp(SURFACE_LEVEL + offset, SURFACE_LEVEL - 6, SURFACE_LEVEL + 4);
        } else if (x <= caveEnd) {
            // Cave biome — moderate hills, less variation than the forest
            int offset = (int)(baseH * 0.5f) - 1;
            heightmap[x] = clamp(SURFACE_LEVEL + offset, SURFACE_LEVEL - 2, SURFACE_LEVEL + 2);
        } else {
            // Light cave surface — gentle undulation, nearly flat
            int offset = (int)(baseH * 0.3f);
            heightmap[x] = clamp(SURFACE_LEVEL + offset, SURFACE_LEVEL - 1, SURFACE_LEVEL + 2);
        }
    }
}

// =============================================================================
// SECTION 7 — TREE PLACEMENT
// =============================================================================

// -----------------------------------------------------------------------------
// placeTree
//
// Attempts to place a single tree (trunk + canopy) at world column col.
//
// Trunk shape:
//   BLOCK_WOOD blocks from row (grassRow - trunkHeight) up to (grassRow - 1).
//   The topmost trunk cell is called trunkTop; the bottommost is trunkBase.
//   The trunk stands one block above the grass surface.
//
// Canopy shape (diamond / lozenge):
//   Leaves fill all cells (lx, ly) where abs(dx) + abs(dy) <= 3, with
//   dx in [-2, +2] and dy in [-2, 0] (above the trunk top only).
//   This creates the shape:
//
//         ***        dy = -2, abs(dx) <= 1  (3 leaf cells)
//        *****       dy = -1, abs(dx) <= 2  (5 leaf cells)
//        **|**       dy =  0, |=trunk, *=leaf (4 leaf cells + trunk)
//
//   BLOCK_LEAVES only overwrites BLOCK_AIR or BLOCK_SKY cells, so trunks
//   of adjacent trees are never clobbered by a neighbour's canopy.
//
// Spacing rule:
//   col must be at least 3 greater than lastTreeCol.  If not, the function
//   returns false immediately without placing anything.
//
// Bounds checks (any failure → return false, nothing placed):
//   — trunkTop - 2 >= 0        (canopy doesn't exit the top of the world)
//   — col - 2 >= 0             (canopy doesn't clip the left world edge)
//   — col + 2 < worldW         (canopy doesn't clip the right world edge)
//   — All trunk cells are AIR or SKY (don't overwrite solid blocks)
//
// Parameters:
//   world        — the Block** world array
//   worldW       — world width (column count)
//   worldH       — world height (row count)
//   col          — column to place the tree in
//   grassRow     — the row index of the grass block at this column
//   trunkHeight  — number of wood blocks in the trunk
//   lastTreeCol  — in/out: updated to col if placement succeeds
//
// Returns true if the tree was placed; false if it was rejected.
// -----------------------------------------------------------------------------
static bool placeTree(Block** world, int worldW, int worldH,
                      int col, int grassRow, int trunkHeight, int& lastTreeCol) {
    // Enforce minimum 3-column gap between adjacent trees
    if (col - lastTreeCol < 3) return false;

    int trunkTop  = grassRow - trunkHeight;  // Topmost trunk row (= top of canopy centre)
    int trunkBase = grassRow - 1;            // Bottommost trunk row (one above grass)

    // Ensure the canopy's topmost row (trunkTop - 2) is within the world
    if (trunkTop - 2 < 0) return false;

    // Ensure the canopy's widest extent doesn't clip the horizontal world edges
    if (col - 2 < 0 || col + 2 >= worldW) return false;

    // Verify every trunk cell is clear (only AIR or SKY is overwritable)
    for (int y = trunkTop; y <= trunkBase; y++) {
        if (y < 0 || y >= worldH) return false; // Row out of world bounds
        BlockType t = world[y][col].type;
        if (t != BLOCK_AIR && t != BLOCK_SKY) return false; // Blocked by terrain
    }

    // All checks passed — place the trunk blocks
    for (int y = trunkTop; y <= trunkBase; y++) {
        world[y][col].type = BLOCK_WOOD;
    }

    // Place the canopy: diamond centred on trunkTop (the top trunk cell)
    //   dy ranges -2..0  (canopy sits above and level with trunk top)
    //   abs(dx)+abs(dy) <= 3  (lozenge / half-diamond shape)
    for (int dy = -2; dy <= 0; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            // Discard cells outside the lozenge boundary
            if (abs(dx) + abs(dy) > 3) continue;

            int lx = col + dx;       // Leaf cell world column
            int ly = trunkTop + dy;  // Leaf cell world row

            // Skip cells outside the world array bounds
            if (lx < 0 || lx >= worldW || ly < 0 || ly >= worldH) continue;

            // Only overwrite air/sky — never clobber other tree trunks or terrain
            BlockType t = world[ly][lx].type;
            if (t == BLOCK_AIR || t == BLOCK_SKY) {
                world[ly][lx].type = BLOCK_LEAVES;
            }
        }
    }

    // Record the column of this tree so the next call can enforce spacing
    lastTreeCol = col;
    return true; // Tree was successfully placed
}

// =============================================================================
// SECTION 8 — ORE SELECTION
// =============================================================================

// -----------------------------------------------------------------------------
// pickOre
//
// Determines the block type for a stone cell at (col, row) by checking for
// ore deposits from rarest (deepest) to most common.  Uses independent hash
// channels for each ore type to prevent correlations between adjacent checks.
//
// Ore distribution by depth (row = world Y coordinate, 0 = top):
//
//   Diamond (BLOCK_DIAMOND):
//     Only possible at row >= 50 (very deep).
//     2% chance per cell (hashPercent % 100 < 2).
//     Rarest — exists only in the deepest zone.
//
//   Gold (BLOCK_GOLD):
//     Only possible at row >= 30 (deep zone).
//     3% chance per cell (hashPercent2 % 100 < 3).
//     Uses the secondary hash channel to avoid correlation with diamond check.
//
//   Iron (BLOCK_IRON):
//     Only possible at row >= 12 (mid-depth, below the shallow dirt layer).
//     15% chance per cell (hashPercent channel, different seed offset).
//     Common — the main ore the player encounters early underground.
//
//   Stone (BLOCK_STONE):
//     Default when no ore check succeeds.
//
// Parameters:
//   seed — world seed
//   col  — world column of the cell
//   row  — world row of the cell (depth, increases downward)
//
// Returns the BlockType the cell should contain.
// -----------------------------------------------------------------------------
static BlockType pickOre(unsigned int seed, int col, int row) {
    // Diamond: only in the deepest zone (row >= 50), 2% chance
    if (row >= 50) {
        int r = hashPercent(seed, col, row);
        if (r < 2) return BLOCK_DIAMOND;
    }

    // Gold: in the deep zone (row >= 30), 3% chance (independent channel)
    if (row >= 30) {
        int r = hashPercent2(seed, col, row);
        if (r < 3) return BLOCK_GOLD;
    }

    // Iron: mid-depth (row >= 12), 15% chance (shifted hash input for independence)
    if (row >= 12) {
        int r = hashPercent(seed, col + 5000, row + 5000);
        if (r < 15) return BLOCK_IRON;
    }

    // Default: plain stone for cells that don't roll an ore
    return BLOCK_STONE;
}

// =============================================================================
// SECTION 9 — MAIN GENERATION ROUTINE
// =============================================================================

// -----------------------------------------------------------------------------
// generateWorld  (PUBLIC)
//
// The nine-step procedural world generation pipeline.  Operates on the Block**
// array allocated by initWorld(), filling every cell with appropriate block
// types, initial visibility flags, and biome-specific features.
//
// The steps are:
//   0 — Default fill (everything is hidden stone)
//   1 — Build surface heightmap via three-octave noise
//   2 — Place sky, grass, dirt, ores, and bedrock per column
//   3 — Mark surface blocks initially visible
//   4 — Place trees per biome with density and height distributions
//   5 — Carve the main cave tunnel through the cave biome
//   6 — Carve 4–6 branch tunnels off the main cave
//   7 — Hollow out the light cave cavern and place the dragon portal
//   8 — Connect the main cave exit to the light cave entrance
//   9 — Carve the forest-to-cave entrance slope
//
// All randomness is derived from state.seed via worldHash(), making the
// entire pipeline deterministic and reproducible.
// -----------------------------------------------------------------------------
void generateWorld(GameState& state) {
    // Cache frequently used state fields into local variables for readability
    unsigned int seed = state.seed;
    int W = state.worldWidth;
    int H = state.worldHeight;
    Block** world = state.world;

    // Validate preconditions — abort silently if the world is not ready
    if (W <= 0 || H <= 0 || world == nullptr) return;

    // Compute biome boundary columns as percentages of the world width
    // forestEnd  — last column of the forest biome (~25% of world)
    int forestEnd = clamp((W * 25) / 100 - 1, 0, W - 1);
    // caveEnd    — last column of the cave biome (~70% of world = forestEnd + 45%)
    int caveEnd = clamp(forestEnd + (W * 45) / 100, forestEnd + 1, W - 1);
    // caveStart  — first column of the cave biome (immediately after forest)
    int caveStart = forestEnd + 1;
    // lightStart — first column of the light cave biome (after cave)
    int lightStart = caveEnd + 1;
    // lightEnd   — last column of the light cave biome (rightmost world column)
    int lightEnd = W - 1;

    // --------------------------------------------------
    // STEP 0: Default everything to BLOCK_STONE, hidden
    // All subsequent steps carve, replace, or mark cells from this baseline.
    // Starting with solid stone means unexplored underground areas correctly
    // appear as stone in the rare case that fog of war misses a cell.
    // --------------------------------------------------
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            world[y][x].type    = BLOCK_STONE;   // Solid stone everywhere by default
            world[y][x].mined   = false;          // No cell has been mined yet
            world[y][x].visible = false;          // Everything hidden until explored
        }
    }

    // --------------------------------------------------
    // STEP 1: Build surface heightmap
    // Fills heightmap[x] with the world row index of the grass block for
    // each column x.  Lower values = higher terrain (Y increases downward).
    // --------------------------------------------------
    std::vector<int> heightmap(W, SURFACE_LEVEL); // Default all columns to nominal surface
    buildSurfaceHeightmap(seed, heightmap.data(), W, forestEnd, caveEnd);

    // --------------------------------------------------
    // STEP 2: Fill sky, grass, dirt, stone+ores for all columns
    // Processes each column independently using the heightmap from Step 1.
    // --------------------------------------------------
    for (int x = 0; x < W; x++) {
        // Grass row for this column (from the heightmap)
        int grassRow = heightmap[x];

        // Sky above grass — all rows above the grass block are open air sky
        for (int y = 0; y < grassRow; y++) {
            world[y][x].type = BLOCK_SKY;
        }

        // Grass — the surface block; the player walks on top of this
        if (grassRow >= 0 && grassRow < H)
            world[grassRow][x].type = BLOCK_GRASS;

        // Dirt below grass (variable depth 3-5, only above stone level)
        // The randomised depth creates a more natural-looking terrain profile.
        int dirtDepth = 3 + (int)(worldHash(seed, x, 8910) % 3);
        for (int y = grassRow + 1; y < grassRow + 1 + dirtDepth && y < H; y++) {
            // Only place dirt if we haven't reached the hard stone transition depth
            if (y < STONE_LEVEL)
                world[y][x].type = BLOCK_DIRT;
        }

        // Stone + ores from STONE_LEVEL down to (but not including) the bedrock row
        // pickOre() decides the exact block type based on depth
        for (int y = STONE_LEVEL; y < H - 1; y++) {
            if (world[y][x].type == BLOCK_DIRT) continue; // Dirt already placed — skip
            world[y][x].type = pickOre(seed, x, y);
        }

        // Bedrock — the indestructible bottom row, prevents the player from mining out
        world[H - 1][x].type = BLOCK_BEDROCK;
    }

    // --------------------------------------------------
    // STEP 3: Surface visibility — reveal sky + terrain rows
    //
    // Mark the sky and every surface row visible so the player sees the
    // full hillscape without fog immediately on spawn.
    // We scan each column and mark everything from y=0 down to
    // (grassRow + 2) visible, giving a thin slice of exposed dirt.
    // --------------------------------------------------
    for (int x = 0; x < W; x++) {
        int grassRow = heightmap[x];
        // +2 exposes two rows of dirt below grass so the transition looks natural
        int revealTo = clamp(grassRow + 2, 0, H - 1);
        // Mark every row from the sky top down to revealTo as visible
        for (int y = 0; y <= revealTo; y++) {
            world[y][x].visible = true;
        }
    }

    // --------------------------------------------------
    // STEP 4: Trees
    //
    // Place trees across all three biomes with biome-specific density
    // and trunk height distributions.
    //
    // Forest:    ~20% density, trunk height [3, 8] (normally distributed)
    // Cave:      ~10% density, trunk height [2, 6] (shorter, sparser)
    // LightCave: ~5%  density, trunk height [2, 4] (small cave trees)
    //
    // The lastTree variable tracks the last column a tree was placed in,
    // enforcing the minimum 3-column spacing rule via placeTree().
    // --------------------------------------------------
    {
        int lastTree = -10; // Start far left so the first column is always eligible

        // ── Forest trees ────────────────────────────────────────────────────

        // Guarantee at least one tree near the left edge (cols 2-6) if possible.
        // This ensures the player always has a visual reference point when spawning.
        bool leftTreePlaced = false;
        int leftStart = 2;                          // First column to try for the guaranteed tree
        int leftEnd = std::min(forestEnd, 6);       // Last column to try
        for (int x = leftStart; x <= leftEnd && !leftTreePlaced; x++) {
            int grassRow = heightmap[x];
            int trunkH = normalTrunkHeight(seed, x); // Height from normal distribution
            if (placeTree(world, W, H, x, grassRow, trunkH, lastTree)) {
                leftTreePlaced = true; // Stop searching once one tree is placed
            }
        }

        // Main forest tree pass: ~20% density, starting from leftStart to forestEnd
        for (int x = leftStart; x <= forestEnd; x++) {
            int grassRow = heightmap[x];
            int chance = hashPercent(seed, x, 8888); // Roll in [0, 99]
            if (chance < 20) { // 20% probability of attempting a tree here
                int trunkH = normalTrunkHeight(seed, x);
                placeTree(world, W, H, x, grassRow, trunkH, lastTree);
            }
        }

        // ── Cave biome surface trees ─────────────────────────────────────────
        // Sparser than the forest; shorter trunks with mean ~4, range [2, 6]
        lastTree = forestEnd - 2; // Reset spacing tracker at the biome boundary
        for (int x = caveStart; x <= caveEnd; x++) {
            int grassRow = heightmap[x];
            int chance = hashPercent(seed, x, 8888);
            if (chance < 10) { // 10% probability — half the density of the forest
                // Shorter mean (4) for cave biome: sum 6 values in [0, 4]
                int raw = 0;
                for (int i = 0; i < 6; i++)
                    raw += (int)(worldHash(seed, x * 7 + i, 9999) % 5);
                // Map [0, 30] to [2, 6] with a lower ceiling than the forest
                int trunkH = clamp(2 + (raw * 4) / 30, 2, 6);
                placeTree(world, W, H, x, grassRow, trunkH, lastTree);
            }
        }

        // ── Light Cave surface trees ──────────────────────────────────────────
        // Very sparse; very short (mean ~3, range [2, 4]) to fit under the cave ceiling
        lastTree = caveEnd - 2; // Reset spacing tracker at the biome boundary
        for (int x = lightStart; x < W; x++) {
            int grassRow = heightmap[x];
            int chance = hashPercent(seed, x, 7000);
            if (chance < 5) { // 5% probability — rarest trees in the game
                // Sum 4 values in [0, 2] for a very short height distribution
                int raw = 0;
                for (int i = 0; i < 4; i++)
                    raw += (int)(worldHash(seed, x * 5 + i, 7001) % 3);
                // Map to range [2, 4]
                int trunkH = clamp(2 + (raw * 2) / 12, 2, 4);
                placeTree(world, W, H, x, grassRow, trunkH, lastTree);
            }
        }
    }

    // --------------------------------------------------
    // STEP 5: Main cave tunnel (Cave biome, cols 50–139)
    //
    // Carves a winding horizontal tunnel through the entire cave biome.
    // The tunnel path is stored in cavePath[x] (the centre row for each column).
    //
    // Path generation algorithm:
    //   Start at centerY = 20 (near the surface).
    //   Target depth increases from row 20 to row 45 across the biome.
    //   Every 4 columns, apply a random vertical delta in [-2, +2].
    //   A bias nudges the delta toward the target depth if we drift too far.
    //
    // Tunnel carving:
    //   At each column, carve a 6-wide × 5-tall rectangle centred on cavePath[x].
    //   The entrance (first 4 columns) uses a wider 8×8 opening.
    //   Cave floor grass is placed 3 rows below each tunnel centre.
    // --------------------------------------------------
    std::vector<int> cavePath(W, 0); // cavePath[x] = centre row of tunnel at column x

    {
        int centerY = 20;  // Initial depth — near the surface
        int targetY = 45;  // Target depth — approaches deep stone by the cave exit

        // ── Build the path array ──────────────────────────────────────────────
        for (int x = caveStart; x <= caveEnd; x++) {
            // Record the centre row for this column
            cavePath[x] = centerY;

            // Update the centre every 4 columns (avoids overly jagged paths)
            if ((x - caveStart) % 4 == 0 && x < caveEnd) {
                int r = hashPercent(seed, x, 7777) % 5; // Random value in [0, 4]
                int delta = r - 2;                       // Map to [-2, +2]

                // Bias toward the target depth proportional to how far we are through the biome
                float progress = (float)(x - caveStart) / (float)(caveEnd - caveStart + 1);
                int ideal = 20 + (int)(progress * (targetY - 20));
                // If too far above ideal, force a downward move; too far below → upward
                if (centerY < ideal - 2) delta = clamp(delta, 0, 2);
                if (centerY > ideal + 2) delta = clamp(delta, -2, 0);

                // Apply the delta, clamped to the valid world depth range
                centerY = clamp(centerY + delta, 15, 60);
            }
        }

        // ── Carve the main tunnel: 6 wide, 5 tall (halfW=3, halfH=2) ─────────
        for (int x = caveStart; x <= caveEnd; x++) {
            int cy = cavePath[x]; // Centre row for this column
            int halfW = 3;        // Half-width of the tunnel rectangle
            int halfH = 2;        // Half-height of the tunnel rectangle

            // Wider entrance opening for the first few columns (easier to enter)
            if (x <= caveStart + 3) { halfH = 4; halfW = 4; } // wide entrance

            // Carve a rectangular cross-section centred on (x, cy)
            for (int dy = -halfH; dy <= halfH; dy++) {
                for (int dx = -halfW; dx <= halfW; dx++) {
                    int wx = x + dx;
                    int wy = cy + dy;
                    // Skip cells outside world bounds
                    if (wx < 0 || wx >= W || wy < 0 || wy >= H - 1) continue;
                    // Never carve through bedrock
                    if (world[wy][wx].type == BLOCK_BEDROCK) continue;
                    world[wy][wx].type = BLOCK_AIR; // Hollow out the tunnel cell
                }
            }
        }

        // ── Cave floor grass ──────────────────────────────────────────────────
        // Place BLOCK_GRASS 3 rows below the tunnel centre if the cell above is air
        for (int x = caveStart; x <= caveEnd; x++) {
            int cy = cavePath[x];
            int floorY = cy + 3; // 3 rows below centre = near the bottom of the tunnel
            if (floorY >= 0 && floorY < H - 1) {
                // Only place grass if the block is a solid type (stone/iron/gold)
                if (world[floorY][x].type == BLOCK_STONE ||
                    world[floorY][x].type == BLOCK_IRON  ||
                    world[floorY][x].type == BLOCK_GOLD) {
                    // And only if the cell above it was carved into air
                    if (floorY - 1 >= 0 && world[floorY - 1][x].type == BLOCK_AIR) {
                        world[floorY][x].type = BLOCK_GRASS;
                    }
                }
            }
        }
    }

    // --------------------------------------------------
    // STEP 6: Branch tunnels (4–6 branches off main cave)
    //
    // Carves short diagonal side-tunnels off the main cave path to create
    // a more complex underground network and give the player more to explore.
    //
    // Each branch:
    //   Starts at a random column along the main cave path
    //   Chooses one of four diagonal directions (±dx, ±dy)
    //   Carves a 3×3 wide path for 20–30 steps
    //   Stops if it exits the cave biome bounds or the valid depth range
    // --------------------------------------------------
    {
        // Number of branches: 4 to 6, determined by the world seed
        int numBranches = 4 + (int)(worldHash(seed, 12345, 67890) % 3);

        for (int b = 0; b < numBranches; b++) {
            // Pick a start column along the main cave, avoiding the first and last 3 cols
            int caveSpan = std::max(1, caveEnd - caveStart - 4);
            int startX = caveStart + 3 + (int)(worldHash(seed, b * 137, 4444) % caveSpan);
            if (startX > caveEnd - 3) startX = caveEnd - 3; // Clamp to safe range
            int startY = cavePath[startX]; // Start at the main cave path depth

            // Choose a diagonal direction: one of (±1, ±1) combinations
            int dirSeed = (int)(worldHash(seed, b * 271, 5555) % 4);
            int bdx, bdy;
            switch (dirSeed) {
                case 0: bdx =  1; bdy = -1; break; // Right and upward
                case 1: bdx =  1; bdy =  1; break; // Right and downward
                case 2: bdx = -1; bdy = -1; break; // Left and upward
                default: bdx = -1; bdy =  1; break; // Left and downward
            }

            // Branch length: 20 to 30 steps
            int branchLen = 20 + (int)(worldHash(seed, b * 311, 6666) % 11);
            int bx = startX, by = startY; // Current head of the branch

            for (int step = 0; step < branchLen; step++) {
                // Carve a 3×3 cross-section at the current branch head position
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int wx = bx + dx, wy = by + dy;
                        // Keep branches within the cave biome horizontally
                        if (wx < caveStart || wx > caveEnd) continue;
                        // Keep branches within safe vertical bounds
                        if (wy < 1 || wy >= H - 1) continue;
                        // Never carve through bedrock
                        if (world[wy][wx].type == BLOCK_BEDROCK) continue;
                        world[wy][wx].type = BLOCK_AIR;
                    }
                }

                // Advance the branch head diagonally
                bx += bdx;
                // Apply vertical movement on every other step to avoid 45° steepness
                by += (step % 2 == 0) ? bdy : 0;

                // Stop the branch if it exits valid bounds
                if (bx < caveStart || bx > caveEnd || by < 13 || by >= H - 2) break;
            }
        }
    }

    // --------------------------------------------------
    // STEP 7: Light Cave (cols 140–199)
    //
    // Creates a large underground cavern with:
    //   — Open sky space in rows 0–19 (same as the overworld)
    //   — A hollowed-out main chamber in rows 20–65
    //   — A hilly grass floor at approximately row 50
    //   — Short trees on the floor
    //   — The dragon portal centred in the biome
    // --------------------------------------------------

    // lightCaveFloor[x] stores the grass row for each light cave column
    std::vector<int> lightCaveFloor(W, 0);
    if (lightStart < W) {
        // ── Clear the cavern space (rows 20–65) ──────────────────────────────
        // Hollow out rows 20-65
        for (int y = 20; y <= 65; y++) {
            for (int x = lightStart; x < W; x++) {
                if (world[y][x].type == BLOCK_BEDROCK) continue; // Never touch bedrock
                world[y][x].type = BLOCK_AIR; // Open up the cavern ceiling and walls
            }
        }

        // ── Hilly floor ───────────────────────────────────────────────────────
        // Build a gently rolling floor at approximately row 50, varying ±7 rows
        {
            int floorY = 50; // Starting floor depth
            // Walk left to right, randomly adjusting floor height every 4 columns
            for (int x = lightStart; x < W; x++) {
                lightCaveFloor[x] = floorY; // Record the floor row for this column
                if ((x - lightStart) % 4 == 0 && x < W - 1) {
                    // Random delta: -1, 0, or +1 (maps hash % 3 → delta - 1)
                    int r = hashPercent(seed, x, 8000) % 3;
                    int delta = r - 1;
                    // Clamp to keep floor between rows 46 and 57
                    floorY = clamp(floorY + delta, 46, 57);
                }
            }

            // Place grass, dirt, and stone for the floor profile
            for (int x = lightStart; x < W; x++) {
                int gy = lightCaveFloor[x]; // Grass row for this column
                world[gy][x].type = BLOCK_GRASS; // Top of floor is grass

                // Three rows of dirt below the grass
                for (int d = 1; d <= 3; d++) {
                    if (gy + d < 65) world[gy + d][x].type = BLOCK_DIRT;
                }

                // Remaining rows down to row 65 are solid stone
                for (int y = gy + 4; y <= 65; y++) {
                    world[y][x].type = BLOCK_STONE;
                }
            }
        }

        // ── Light Cave floor trees (5%) ───────────────────────────────────────
        // Small trees (height 2–4) scattered on the cave floor at 5% density
        {
            int lastTree = caveEnd - 2; // Reset spacing tracker
            for (int x = lightStart; x < W; x++) {
                int gy = lightCaveFloor[x]; // Grass row for this column
                int chance = hashPercent(seed, x, 7000);
                if (chance < 5) { // 5% chance of a tree at this column
                    // Short trunk: sum 4 values in [0, 2] → range [2, 4]
                    int raw = 0;
                    for (int i = 0; i < 4; i++)
                        raw += (int)(worldHash(seed, x * 5 + i, 7001) % 3);
                    int trunkH = clamp(2 + (raw * 2) / 12, 2, 4);
                    placeTree(world, W, H, x, gy, trunkH, lastTree);
                }
            }
        }

        // ── Dragon portal centred in the light cave ───────────────────────────
        // The dragon portal is a 3-wide × 4-tall structure of BLOCK_DRAGON_CAVE
        // placed on the cavern floor at the horizontal centre of the biome.
        {
            // Centre column of the light cave biome
            int portalCol = clamp(lightStart + (lightEnd - lightStart) / 2, lightStart, lightEnd);
            int floorRow  = lightCaveFloor[portalCol]; // Floor row at the portal column
            int portalTop = floorRow - 4;              // Top row of the portal structure

            // Fill the 3×4 portal block area with BLOCK_DRAGON_CAVE
            for (int dy = 0; dy < 4; dy++) {
                for (int dx = 0; dx < 3; dx++) {
                    int px2 = portalCol + dx;
                    int py2 = portalTop + dy + 1;
                    // Bounds-check before writing
                    if (px2 >= 0 && px2 < W && py2 >= 0 && py2 < H)
                        world[py2][px2].type = BLOCK_DRAGON_CAVE;
                }
            }

            // Clear a 3-tall approach corridor to the left of the portal
            // so the player can walk up to the portal without needing to mine
            if (portalCol - 1 >= 0) {
                world[floorRow][portalCol - 1].type = BLOCK_AIR;
                if (floorRow - 1 >= 0) world[floorRow - 1][portalCol - 1].type = BLOCK_AIR;
                if (floorRow - 2 >= 0) world[floorRow - 2][portalCol - 1].type = BLOCK_AIR;
            }
        }
    }

    // --------------------------------------------------
    // STEP 8: Connect main cave → Light Cave (cols 134-148)
    //
    // Carves a smooth 9-wide tunnel that bridges the exit of the main cave
    // tunnel and the entrance of the light cave cavern.  Without this step
    // there would be a solid stone wall between the two biomes.
    //
    // The tunnel path is linearly interpolated (lerped) between:
    //   exitY  — the depth of the main cave at its rightmost column
    //   entryY — the fixed top-of-cavern entry depth (row 40)
    //
    // The bridge spans from (caveEnd - 2) to (lightStart + 3) to overlap
    // slightly with both biomes and ensure no gaps.
    // --------------------------------------------------
    if (lightStart < W) {
        int exitY  = cavePath[caveEnd]; // Depth of main cave at the exit
        int entryY = 40;                // Depth to aim for at the light cave entrance

        int bridgeStart = clamp(caveEnd - 2, 0, W - 1);
        int bridgeEnd = clamp(lightStart + 3, 0, W - 1);
        int span = std::max(1, bridgeEnd - bridgeStart); // Avoid division by zero

        for (int x = bridgeStart; x <= bridgeEnd; x++) {
            // Compute the lerp parameter [0.0, 1.0] along the bridge
            float t = (float)(x - bridgeStart) / (float)span;
            // Interpolate the centre row between exitY and entryY
            int cy = exitY + (int)(t * (entryY - exitY));

            // Carve a 9-tall vertical slice at this column (±4 rows from centre)
            for (int dy = -4; dy <= 4; dy++) {
                int wy = cy + dy;
                // Bounds-check: stay within valid world rows
                if (wy < 1 || wy >= H - 1) continue;
                if (x < 0 || x >= W) continue;
                // Never carve bedrock
                if (world[wy][x].type == BLOCK_BEDROCK) continue;
                world[wy][x].type = BLOCK_AIR;
            }
        }
    }

    // --------------------------------------------------
    // STEP 9: Forest cave entrance slope (cols 45-53)
    //
    // Carves a tapered downward-sloping tunnel that connects the open forest
    // surface to the entrance of the main cave tunnel.  Without this, the
    // player would face a vertical stone wall where the forest meets the cave.
    //
    // The slope path is linearly interpolated between:
    //   surfaceY   — the height of the forest floor at forestEnd
    //   caveEntryY — the depth of the main cave path at caveStart
    //
    // The tunnel width increases from 2 at the surface to 4 at the cave mouth.
    // --------------------------------------------------
    {
        int surfaceY   = heightmap[forestEnd]; // Forest surface height at the boundary
        int caveEntryY = cavePath[caveStart];  // Cave tunnel depth at its entrance

        int slopeStart = clamp(caveStart - 5, 0, W - 1); // Begin slightly before the biome edge
        int slopeEnd = clamp(caveStart + 3, 0, W - 1);   // End slightly inside the cave biome
        int span = std::max(1, slopeEnd - slopeStart);    // Avoid division by zero

        for (int x = slopeStart; x <= slopeEnd; x++) {
            // Lerp parameter [0.0, 1.0] along the slope
            float t = (float)(x - slopeStart) / (float)span;
            // Interpolated centre row of the slope tunnel at this column
            int cy = surfaceY + (int)(t * (caveEntryY - surfaceY));
            // Half-height grows from 2 at the surface to 4 at the cave entrance
            int halfH = 2 + (int)(t * 2);

            for (int dy = -halfH; dy <= halfH; dy++) {
                int wy = cy + dy;
                // Bounds-check
                if (wy < 1 || wy >= H - 1) continue;
                if (x < 0 || x >= W) continue;
                // Never carve bedrock
                if (world[wy][x].type == BLOCK_BEDROCK) continue;
                // Don't carve above the surface in the forest section to avoid
                // creating a hole in the open-air terrain
                if (wy < surfaceY - 1 && x < caveStart - 2) continue;
                world[wy][x].type = BLOCK_AIR; // Hollow out the slope corridor
            }
        }
    }
}
