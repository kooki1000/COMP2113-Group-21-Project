// =============================================================================
// world_gen.h
// TermiCraft — World Generation Module Header
//
// Declares world allocation and procedural generation for TermiCraft.
// Three biomes: Forest (cols 0–49), Cave (cols 50–139), Light Cave (cols 140–199).
//
// Author:       Mohit
// Dependencies: types.h
//
// =============================================================================
// MODULE OVERVIEW
// =============================================================================
// This module is responsible for two tasks that must happen before the game
// loop starts:
//
//   1.  ALLOCATION  (initWorld)
//       Dynamically allocates the two-dimensional Block array on the heap and
//       stores the pointer in state.world.  The array dimensions come from
//       state.worldWidth (snapped from the viewport or the WORLD_WIDTH
//       constant) and the fixed WORLD_HEIGHT constant.  After this call,
//       every state.world[y][x] address is valid and zero-initialised.
//
//   2.  GENERATION  (generateWorld)
//       Fills the allocated array with terrain, biome features, ores, trees,
//       caves, and the dragon portal using a fully deterministic algorithm
//       driven by state.seed.  Given the same seed, the output is always
//       identical — worlds are reproducible.
//
// The world is divided into three biomes along the X axis.  The exact column
// boundaries are computed as percentages of the world width so that the layout
// scales if the world is made wider:
//
//   Forest biome     (cols 0  … forestEnd)   — ~25% of world width
//       Dense woodland with dramatic rolling hills, tall trees (trunk height
//       3–8), and rich ore deposits.  The visually richest surface biome.
//
//   Cave biome       (cols caveStart … caveEnd)  — ~45% of world width
//       Sparse surface trees, moderate hills, and a massive winding underground
//       cave tunnel that starts near the surface and descends to deep stone.
//       The main cave has 4–6 branch tunnels and a grassy floor.
//
//   Light Cave biome (cols lightStart … W-1)   — remaining ~30%
//       A large underground cavern with a hilly grass floor, short trees, sky
//       overhead (rows 0–19 are open air), ores scattered in the floor, and
//       the dragon portal — the game's final objective — centred in the biome.
//
// =============================================================================
// FUNCTION REFERENCE
// =============================================================================
//
//   initWorld(state)
//       Allocates state.world as a heap array of (worldHeight) pointers, each
//       pointing to a heap array of (worldWidth) Block structs.  Sets
//       state.worldWidth and state.worldHeight from the viewport size /
//       WORLD_WIDTH and WORLD_HEIGHT constants respectively.
//
//       After this call:
//         state.worldWidth  — number of columns in the world
//         state.worldHeight — number of rows in the world
//         state.world[y][x] — valid Block at every (y, x) within bounds
//
//       The caller is responsible for eventually freeing the arrays with
//       delete[].  This function does not initialise block content — that
//       is done by generateWorld().
//
//   generateWorld(state)
//       Fills the array allocated by initWorld() using a nine-step pipeline:
//
//       Step 0 — Default fill
//           Every block is set to BLOCK_STONE with visible = false and
//           mined = false.  This establishes the "everything is rock" baseline
//           that later steps carve away.
//
//       Step 1 — Surface heightmap
//           Calls buildSurfaceHeightmap() to produce an array of grass row
//           indices, one per column.  Uses three-octave smooth noise (Perlin-
//           style) with biome-specific amplitude multipliers:
//             Forest    — multiplier 1.2, clamp [SURFACE-6, SURFACE+4]
//             Cave      — multiplier 0.5, clamp [SURFACE-2, SURFACE+2]
//             LightCave — multiplier 0.3, clamp [SURFACE-1, SURFACE+2]
//
//       Step 2 — Sky, grass, dirt, stone, and ores
//           For each column x:
//             Rows 0 … grassRow-1    → BLOCK_SKY
//             Row  grassRow          → BLOCK_GRASS
//             Rows grassRow+1 … +dirtDepth → BLOCK_DIRT  (depth 3–5, variable)
//             Rows STONE_LEVEL … H-2 → pickOre() — BLOCK_STONE / IRON / GOLD /
//                                       DIAMOND depending on row depth
//             Row  H-1               → BLOCK_BEDROCK
//
//       Step 3 — Initial surface visibility
//           Marks all blocks from y=0 down to (grassRow + 2) as visible so
//           the player sees the full landscape immediately on spawn without
//           needing to explore the surface first.
//
//       Step 4 — Trees
//           Places trees using placeTree() with biome-specific density and
//           trunk height distributions (approximated via a sum of uniform
//           randoms — the Central Limit Theorem gives a near-normal shape):
//             Forest    — ~20% density, trunk height [3, 8]
//             Cave      — ~10% density, trunk height [2, 6]
//             LightCave — ~5%  density, trunk height [2, 4]
//           A minimum 3-column spacing between trees is enforced.
//           One tree near the left edge (cols 2–6) is always guaranteed.
//
//       Step 5 — Main cave tunnel
//           Carves a winding 6-wide × 5-tall horizontal tunnel through the
//           cave biome.  The tunnel path is computed tick-by-tick: every 4
//           columns a random vertical delta (−2…+2) is applied, with a
//           gentle bias toward a "target" depth that increases from shallow
//           (row ~20) at the cave entrance to deep (row ~45) at the exit.
//           A cave-floor grass strip is placed 3 rows below the tunnel centre.
//
//       Step 6 — Branch tunnels
//           Generates 4–6 diagonal branches off the main cave, each 20–30
//           cells long, carved 3-wide.  Branches are seeded from random
//           positions along the main tunnel and proceed in one of four
//           diagonal directions (±dx, ±dy).
//
//       Step 7 — Light cave cavern
//           Hollows out rows 20–65 across the light cave columns, then
//           rebuilds a hilly grass floor at approximately row 50 (±7),
//           fills below with dirt and stone, and places trees on the floor.
//           The dragon portal (a 3×4 block of BLOCK_DRAGON_CAVE) is centred
//           horizontally in the biome on the floor surface.
//
//       Step 8 — Cave → Light Cave connector
//           Carves a 9-wide smooth tunnel (lerped Y path) between the exit
//           of the main cave and the top of the light cave cavern, ensuring
//           the player can walk continuously between the two biomes.
//
//       Step 9 — Forest → Cave entrance slope
//           Carves a tapered tunnel (4-wide at entrance, growing to 6-wide
//           at the cave mouth) bridging the forest surface and the cave
//           entrance, so the player descends naturally from the open air
//           into the underground cave system.
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//   types.h  — for GameState, Block, BlockType, and all world-constant macros
//              (WORLD_WIDTH, WORLD_HEIGHT, SURFACE_LEVEL, STONE_LEVEL,
//               DEEP_LEVEL, and the BLOCK_* enum values).
// =============================================================================

#ifndef WORLD_GEN_H
#define WORLD_GEN_H

#include "types.h"

// -----------------------------------------------------------------------------
// initWorld
//
// Allocates the two-dimensional Block array that represents the game world.
// Must be called exactly once, before generateWorld(), during startup.
//
// Parameters:
//   state — mutable reference to GameState.  The function writes to:
//             state.worldWidth  — set to state.viewportWidth if > 0, else WORLD_WIDTH
//             state.worldHeight — always set to WORLD_HEIGHT
//             state.world       — set to a freshly heap-allocated Block** array
//
// After this call, state.world[y] points to a valid array of state.worldWidth
// Block structs for every y in [0, state.worldHeight).  Block content is
// uninitialised at this point — call generateWorld() to fill it.
// -----------------------------------------------------------------------------
void initWorld(GameState& state);

// -----------------------------------------------------------------------------
// generateWorld
//
// Fills the world array allocated by initWorld() with procedurally generated
// terrain, biomes, ores, trees, caves, and the dragon portal.
//
// The entire generation is deterministic: the same state.seed always produces
// the same world.  No global random state (rand()/srand()) is used; all
// randomness comes from the internal worldHash() function seeded by state.seed.
//
// Parameters:
//   state — mutable reference to GameState.  Reads:
//             state.seed, state.worldWidth, state.worldHeight, state.world
//           Writes Block.type, Block.visible, and Block.mined for every cell.
//
// Preconditions:
//   state.world must be a valid pointer allocated by initWorld().
//   state.worldWidth and state.worldHeight must be > 0.
//   If either dimension is 0 or state.world is null, the function returns
//   immediately without generating anything.
// -----------------------------------------------------------------------------
void generateWorld(GameState& state);

#endif
