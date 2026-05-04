# COMP2113-Group-21-Project

## Team Members

| Name | Student ID | Role |
| :--- | :--- | :--- |
| Sohan Gupta Thedla | 3036636025 | Boss fight, score system |
| Aryan Sokhiya | 3036484587 | Wordle Implementation, Sudoku Implementation |
| Koki Ukai | 3036505795 | Player controller, mining system, crafting system |
| Nan Jiang | 3036475225 | Minesweeper implementation, TwentyFour implementation |
| Sheikh Mohammad Saarim | 3036520068 | Main game logic, menu, integration of minigames, display, types, save file, makefile |
| Mohit Reddy Vuyyuru | 3036517750 | World generation, fog of war, day and night(setting) |

---

## Game Overview

TermiCraft is a 2D text-based survival and mining game played entirely in the terminal. Each run generates a fresh ASCII world with surface terrain, underground layers, and ore deposits. Players mine resources to craft progressively stronger tools and armor, and some crafting tiers trigger minigames (Wordle, Minesweeper, TwentyFour, Sudoku). The goal is to reach the dragon cave, defeat the final boss, and finish with the highest score.

## Video demo

[Demo Link](https://connecthkuhk-my.sharepoint.com/:v:/g/personal/u3663602_connect_hku_hk/IQCt81M8rkMkRYQBxVnZdqKSAS_sefPqCBEr9zjzRP_1WAs?nav=eyJyZWZlcnJhbEluZm8iOnsicmVmZXJyYWxBcHAiOiJPbmVEcml2ZUZvckJ1c2luZXNzIiwicmVmZXJyYWxBcHBQbGF0Zm9ybSI6IldlYiIsInJlZmVycmFsTW9kZSI6InZpZXciLCJyZWZlcnJhbFZpZXciOiJNeUZpbGVzTGlua0NvcHkifX0&e=7SvDTx)

## Requirements

- C++11 compiler (tested with `g++`)
- `make`
- `ncurses` development library
- Terminal with ANSI support

## Build and Run

1. Build the game:

  ```bash
  make
  ```

2. Launch:

  ```bash
  ./termicraft
  ```

## Project Structure

- `main.cpp` / `menu.cpp`: entry point, menus, and game loop coordination
- `world_gen.cpp`, `fog_of_war.cpp`, `day_night.cpp`: world generation and visual systems
- `player.cpp`, `crafting.cpp`: player movement, mining, crafting, and progression
- `wordle.cpp`, `minesweeper.cpp`, `twentyfour.cpp`, `sudoku.cpp`: minigames
- `final_fight.cpp`, `score.cpp`: boss fight and scoring system
- `fileio.cpp`: save/load and high score persistence
- `types.h`, `colors.h`: shared types and rendering utilities

---

## Implementation Summary

### Player Controller & Mining System (`player.h`, `player.cpp`)

Implements the core player entity with real-time keyboard input handling (WASD movement, SPACE mining) and collision checks, plus mining flow with tool-tier requirements. Manages camera tracking, mining rewards/penalties, and crafting-progression hooks that trigger minigames when higher-tier upgrades are attempted.

**How coding elements are met:**

- **Random events (Element 1):** Mining minigame triggers use a calculated probability formula (`baseChance * (MINIGAME_COUNT / NUM_DISTINCT_ORES)`) with random rolls in the range [10%, 16%] to determine if a challenge occurs; dragon caves always trigger challenges regardless of roll. Minigame selection for mining and for crafting progression is randomized among Wordle, Minesweeper, 24 Game, and Sudoku.

- **Data structures (Element 2):** Defines and manipulates `GameState` data containing `Position`, `Inventory`, `Equipment`, and mining/minigame state such as `pendingMinePos`, `pendingMineType`, and `currentMinigame`. Uses shared enums like `BlockType`, `MaterialTier`, and `MinigameType` to drive tool gating and minigame flow.

- **Dynamic memory management (Element 3):** Meets the requirement through runtime-sized containers: `GameState::enemies` is a `std::vector<Enemy>` that grows via `push_back` when `trySpawnEnemy()` creates a Cave Bug, so heap allocation happens on demand and is cleaned up automatically by RAII.

- **File I/O (Element 4):** Integrates with the score system by calling `addScore()` from `score.h` upon successful mining, which delegates to `fileio` for persistent high score storage. Does not perform direct file operations, maintaining clean separation of concerns.

- **Multiple files (Element 5):** Split across `player.h` (interface) and `player.cpp` (implementation). Integrates with `types.h` (shared state), `colors.h` (rendering), `menu.h` (UI utilities), `crafting.h` (equipment checks), and `score.h` (persistence). The minigame initialization uses `std::shuffle` from `<algorithm>` on a static array of minigame types.

- **Difficulty levels (Element 6):** Reads difficulty settings from `GameState` to set player starting health and minigame damage (`minigameDamage`). Tool requirements for mining blocks create a soft difficulty curve (hands → wood → stone → iron → gold → diamond), while dragon cave blocks remain accessible regardless of tier, providing risk/reward choices on higher difficulties.

---

### World Generation (`world_gen.h`, `world_gen.cpp`)

Procedurally builds the full 200×80 TermiCraft world from a single integer seed.
All terrain, ores, trees, caves, and structures are generated deterministically —
the same seed always produces a byte-for-byte identical world.
Generation is split across two public functions declared in `world_gen.h`:
`initWorld()` allocates the grid, and `generateWorld()` fills it.

The world is divided into three horizontal biomes:

- **Forest** (cols 0–49)
  - Dramatic rolling hills with amplitude multiplier 1.2
  - Surface height clamped to ±6 rows from `SURFACE_LEVEL`
  - Tree density ~20%, trunk heights normally distributed in [3, 8]
  - One tree near cols 2–6 is always guaranteed on spawn
  - Rich ore deposits beginning at `STONE_LEVEL`

- **Cave** (cols 50–139)
  - Moderate surface hills (amplitude 0.5, ±2 rows)
  - Tree density ~10%, shorter trunks in [2, 6]
  - A winding main tunnel 6 wide × 5 tall
  - Tunnel centreline descends from row ~20 to row ~45
  - 4–6 diagonal branch tunnels, each 20–30 steps long
  - Grassy floor strip placed 3 rows below each tunnel centre

- **Light Cave** (cols 140–199)
  - Hollowed cavern: rows 20–65 cleared to `BLOCK_AIR`
  - Open sky in rows 0–19, same as the overworld
  - Hilly grass floor at approximately row 50 (±7 rows)
  - Short trees at ~5% density, trunks in [2, 4]
  - Dragon portal: 3×4 `BLOCK_DRAGON_CAVE` centred in the biome
  - Smooth connector tunnel bridging the main cave exit to the cavern entrance

Generation inside `generateWorld()` follows a strict nine-step pipeline:

1. Fill every block with hidden `BLOCK_STONE` (the baseline)
2. Build surface heightmap via three-octave smooth noise
3. Stamp sky, grass, dirt, ores, and bedrock per column
4. Mark surface rows immediately visible on spawn
5. Place trees per biome with density and height distributions
6. Carve the main cave tunnel with a biased random walk
7. Carve 4–6 diagonal branch tunnels off the main passage
8. Hollow the light cave, rebuild its floor, place the dragon portal
9. Carve the forest-to-cave entrance slope and the cave-to-light-cave connector

**How coding elements are met:**

- **Random events (Element 1):**
  All procedural decisions are driven by `worldHash()` — a custom 32-bit hash
  using prime coordinate multipliers (2971, 31337) and the MurmurHash2 mixing
  constant `0x5bd1e995` with two XOR-shift avalanche passes.
  Two independent channels are used to prevent correlation at the same cell:
  `hashPercent()` is the primary channel;
  `hashPercent2()` XORs the seed with `0xDEADBEEF` and offsets coordinates.
  Ore spawn rates are depth-gated:
  - Diamond: 2% chance below row 50 (channel 1)
  - Gold:    3% chance below row 30 (channel 2, independent of diamond)
  - Iron:   15% chance below row 12 (channel 1, shifted input)
  Tree trunk heights use `normalTrunkHeight()`, which sums 6 uniform samples
  to approximate a normal distribution via the Central Limit Theorem.
  No `rand()` or `srand()` is used anywhere in this module.

- **Data structures (Element 2):**
  The world is stored as a `Block**` pointer-to-pointer array,
  with one heap-allocated `Block[]` row per world row.
  Three `std::vector<int>` buffers are used during generation:
  - `heightmap[x]`      — grass row index for each column
  - `cavePath[x]`       — cave centreline row for each column
  - `lightCaveFloor[x]` — light cave floor row for each column
  Biome boundaries (forestEnd, caveEnd, lightStart, lightEnd)
  are computed as integer percentages of the world width.
  Block content is represented by the `BlockType` enum and `Block` struct
  imported from `types.h`.
  All internal helpers are declared `static` to prevent symbol leakage
  to other translation units.

- **Dynamic memory management (Element 3):**
  `initWorld()` allocates the world grid in two heap passes:
  - `new Block*[worldHeight]` for the outer row-pointer array
  - `new Block[worldWidth]`  for each individual row
  The resulting `Block**` is stored in `state.world`.
  `worldWidth` is derived from the runtime viewport size,
  so the allocation size is not known at compile time.
  The caller is responsible for freeing with paired `delete[]` loops.
  No static or global arrays are used for the world grid.

- **File I/O (Element 4):**
  World generation does not directly read or write files.
  Because generation is fully deterministic from `state.seed`,
  `fileio.cpp` only needs to save the seed and the per-block
  `visible` and `mined` flags — not the full terrain layout.
  On load, `generateWorld()` is called again to reconstruct block types,
  then the saved flags are overlaid to restore exploration and mining progress.

- **Multiple files (Element 5):**
  `world_gen.h` declares only the two public functions:
  `initWorld()` and `generateWorld()`.
  `world_gen.cpp` contains all internal helpers:
  `worldHash`, `hashPercent`, `hashPercent2`, `normalTrunkHeight`,
  `clamp`, `smoothNoiseWG`, `terrainHeightWG`, `buildSurfaceHeightmap`,
  `placeTree`, `pickOre`, and the full `generateWorld()` pipeline.
  Standard library dependencies: `<algorithm>`, `<vector>`, `<cmath>`, `<cstring>`.

- **Difficulty levels (Element 6):**
  The three biomes create a spatial difficulty gradient:
  - Forest: beginner-friendly, abundant wood, shallow ores, open terrain
  - Cave: requires tunnel navigation, limited visibility, deeper ore targets
  - Light Cave: endgame zone, deep placement, dragon portal as the final objective
  Ore depth-gating paces equipment progression naturally:
  Iron (row 12) → pickaxe upgrade → Gold (row 30) → Diamond (row 50).
  This gradient applies regardless of the explicit difficulty setting.

---

### Fog of War (`fog_of_war.h`, `fog_of_war.cpp`)

Controls which blocks the player can see and owns the entire world rendering pipeline.
Visibility is governed by a depth-zone model:
surface and dirt rows are always fully visible,
the stone layer grants a circular reveal radius of 3 blocks around the player,
and the deep layer reduces this to 2 blocks.
Once revealed, a block stays visible permanently — exploration is one-way.

All rendering (world cells, entities, HUD, status line) is assembled into a single
700KB static buffer (`renderBuf[700000]`) and flushed to the terminal with one
`write(STDOUT_FILENO, renderBuf, pos)` call per frame.
This eliminates tearing by making the entire screen update atomic.

`updateWorldVisibility()` runs two passes each tick:

- **Pass 1 (surface sweep):**
  Every viewport cell whose world Y is below `STONE_LEVEL` is marked visible
  regardless of player position, so the landscape is always fully shown.

- **Pass 2 (circular underground reveal):**
  A (2r+1)×(2r+1) square is iterated centred on the player,
  and cells with Euclidean distance ≤ r are permanently marked visible.
  Skipped entirely if the player is at the surface (radius returns −1).

`renderWorld()` processes each viewport cell through a seven-priority chain:

1. Player `@` — bright yellow, always on top
2. Alive enemy `B` — bold red, drawn over terrain
3. Out-of-bounds — sky cell (row ≤ `SURFACE_LEVEL + 1`) or black void
4. `BLOCK_SKY` or surface `BLOCK_AIR` — delegated to `renderSkyCell()`
5. `!b.visible` — dark grey colon `:` (fog of war)
6. Inside red-zone radius — red background with white block character
7. Normal visible block — colour from `getBlockColor()` + character from `getBlockChar()`

After the viewport, `appendHUD()` adds four HUD lines to the same buffer:

- Line 1: 20-segment HP bar with colour gradient + score + pickaxe + armour tier
- Line 2: Inventory counts (T / # / I / G / D) + facing arrow + depth + world coordinates
- Line 3: Fire zone alert, blinking red/yellow when `EVENT_RED_ZONE` is active
- Line 4: Status message, coloured green for info or red when it contains "Failed", "Need", or "BURNING"

**How coding elements are met:**

- **Random events (Element 1):**
  The fire zone (`EVENT_RED_ZONE`) alert blink is driven by `ev.alertTicks`:
  the expression `(ev.alertTicks / 8) % 2` alternates the message
  between bold red block-bordered text and a plain yellow warning every 8 ticks.
  Star field rendering in sky cells uses `isStarAt()` from `day_night.cpp`
  via `renderSkyCell()`, producing a stable hash-based star distribution.
  The red zone tint check uses Manhattan distance (`abs(dx) + abs(dy) ≤ ev.radius`)
  to create a diamond-shaped danger region rather than a square.

- **Data structures (Element 2):**
  Operates directly on `state.world` (the `Block**` grid from `GameState`),
  reading `b.type` and `b.visible` for each cell.
  Accesses `std::vector<Enemy>` for entity positions —
  the enemy loop scans the full vector and breaks on the first live match.
  Uses `state.camera.x / y` and `state.viewportWidth / Height`
  to convert between viewport and world coordinate spaces.
  The `RandomEvent` struct is read for red-zone centre, radius, and alert state.

- **Dynamic memory management (Element 3):**
  Uses a statically allocated 700KB `char renderBuf[700000]` at file scope.
  This avoids heap allocation and `malloc` overhead during the render loop,
  ensuring predictable latency at 20 ticks per second.
  The buffer is large enough for 100 columns × 50 rows × ~30 bytes per cell
  plus generous HUD and ANSI escape headroom.
  No per-frame allocation or deallocation occurs in the rendering path.

- **File I/O (Element 4):**
  No file operations are performed.
  The render buffer is transient — it is rewritten from scratch every frame.
  Persistent state (visibility flags, player stats) is saved and loaded
  by `fileio.cpp` operating on `GameState`.

- **Multiple files (Element 5):**
  `fog_of_war.h` declares three public functions:
  `getVisibilityRadius()`, `updateWorldVisibility()`, and `renderWorld()`.
  `fog_of_war.cpp` implements these and the internal `appendHUD()` helper,
  which is `static` and not exposed in the header.
  Dependencies: `day_night.h` (sky rendering), `colors.h` (block colours/characters),
  `types.h` (GameState, Block, Enemy, RandomEvent), `<unistd.h>` (write syscall).

- **Difficulty levels (Element 6):**
  `getVisibilityRadius()` directly encodes the difficulty gradient underground:
  - Above `STONE_LEVEL`: unlimited visibility (safe surface exploration)
  - Stone layer (< `DEEP_LEVEL`): radius 3 (moderate underground visibility)
  - Deep layer (≥ `DEEP_LEVEL`): radius 2 (tight fog, high tension)
  The red zone HUD alert escalates pressure during `EVENT_RED_ZONE` events,
  flashing more aggressively as `alertTicks` counts down.

---

### Day/Night Cycle (`day_night.h`, `day_night.cpp`)

Drives the atmospheric lighting and sky animation across a 6000-tick full cycle,
equivalent to approximately 5 minutes at 20 ticks per second.
The cycle progresses through four named phases:

- **TIME_DAY**   — ticks 0–2399   — bright azure blue sky, sun traverses left to right
- **TIME_DUSK**  — ticks 2400–2999 — warm orange sky, sun slides off the right edge
- **TIME_NIGHT** — ticks 3000–5099 — dark navy sky, moon rises, stars appear
- **TIME_DAWN**  — ticks 5100–5999 — muted pink sky, moon sets, sun begins to rise

The global `tickCount` (defined in `day_night.cpp`, declared `extern` in `day_night.h`)
is the single source of truth for game time.
Only `tickDayCycle()` writes to it; all other modules read it via the header.

The sun and moon are rendered using 3-line ASCII art:

```
Sun:            Moon:
 \|/             _
- O -           ( )
 /|\             ~
```

Their horizontal positions are computed as linear interpolations across the
viewport width, parameterised by progress through the current phase.
During TIME_NIGHT the sun returns −100 (off-screen); during TIME_DAY the moon does the same.

`renderSkyCell()` is the public entry point called by `fog_of_war.cpp`
for every sky cell in the viewport each frame.
It checks four layers in priority order and writes the winning ANSI sequence to the buffer:

1. Sun glyph   — bright yellow (`\033[38;5;226m`), if the cell falls inside the 5×3 art
2. Moon glyph  — bright white  (`\033[38;5;255m`), if the cell falls inside the 3×3 art
3. Star        — white at night, grey at dawn/dusk, drawn at hash-selected positions
4. Plain sky   — single space with background colour only

`getSkyBg()` maps the current phase to an ANSI 256-colour background:

- TIME_DAY   → `\033[48;5;39m`  (bright blue)
- TIME_DUSK  → `\033[48;5;130m` (warm orange)
- TIME_NIGHT → `\033[48;5;17m`  (dark navy)
- TIME_DAWN  → `\033[48;5;95m`  (muted pink)

**How coding elements are met:**

- **Random events (Element 1):**
  Star placement uses `isStarAt(row, col)` — a standalone hash function
  that combines row and column with prime multipliers (7919, 6271),
  applies two XOR-shift passes and the MurmurHash2 constant,
  then returns `true` when `(h % 12) == 0` (approximately 8.3% density).
  Because the hash depends only on coordinates and not on `tickCount`,
  the star field is perfectly stable across all frames and save/load cycles —
  stars never flicker or shift position.
  Star glyph selection (`. + * \``) also uses a small coordinate hash
  so each star position always shows the same character.

- **Data structures (Element 2):**
  Defines the `TimeOfDay` enum: `TIME_DAY`, `TIME_DUSK`, `TIME_NIGHT`, `TIME_DAWN`.
  Phase boundaries are declared as `const int` in `day_night.h`:
  `CYCLE_LENGTH = 6000`, `DUSK_START`, `NIGHT_START`, `DAWN_START`.
  Sun and moon art are stored as `static const char*[]` arrays in `day_night.cpp`.
  The star character palette is a `static const char[]` of four glyphs.
  The global `tickCount` is a plain `int` — simple but sufficient,
  since `getTimeOfDay()` always folds it with `% CYCLE_LENGTH`.

- **Dynamic memory management (Element 3):**
  No dynamic allocation occurs anywhere in this module.
  All data is either static constants, stack-local variables,
  or written directly into the caller-supplied `char* buf` pointer.
  `renderSkyCell()` returns the byte count written so the caller can
  advance its buffer position with a simple `pos += renderSkyCell(...)`.

- **File I/O (Element 4):**
  No file operations are performed.
  `tickCount` is serialized by `fileio.cpp` as part of `GameState`
  so the time of day is correctly restored on game load.
  The sky colours and celestial positions then resume from the saved tick
  automatically, with no special handling required in this module.

- **Multiple files (Element 5):**
  `day_night.h` declares all public constants, the `TimeOfDay` enum,
  the `extern int tickCount` shared variable, and all five public functions:
  `getTimeOfDay()`, `getTimeLabel()`, `getSkyBg()`, `tickDayCycle()`, `renderSkyCell()`.
  `day_night.cpp` implements all of the above plus five internal helpers
  declared `static`: `getSunCol()`, `getMoonCol()`, `getSunChar()`,
  `getMoonChar()`, and `isStarAt()`.
  The module is called from two sites: the main game loop calls `tickDayCycle()`
  once per tick, and `fog_of_war.cpp` calls `renderSkyCell()` once per sky cell per frame.

- **Difficulty levels (Element 6):**
  The day/night cycle length is constant across all difficulty settings.
  However, the TIME_NIGHT phase (dark navy background, stars only)
  creates atmospheric tension that complements the mechanical difficulty
  of the reduced underground visibility radius in `fog_of_war.cpp`.
  Players navigating deep caves at night face both limited sight radius
  and a visually darker, more oppressive environment.
  The DUSK and DAWN transitions also serve as soft timers,
  giving the player visual cues that conditions are about to change.

---

## Final Boss Fight and Score System

**Author:** Sohan Gupta Thedla  
**UID:** 3036636025  
**Files:** `final_fight.h`, `final_fight.cpp`, `score.h`, `score.cpp`

### Final Boss Fight (`final_fight.h` / `final_fight.cpp`)

---

#### Overview

The player enters the dragon cave portal and is taken into a full-screen ncurses
arena. A large ASCII dragon moves left and right across the top of the screen,
firing fireballs downward in phase-dependent patterns. The player moves their
sprite horizontally along the bottom of the arena and fires upward arrows using
rapid fire. The fight ends when either the dragon's HP reaches zero (victory) or
the player's HP reaches zero (defeat). The player can also press `Q` to flee,
which counts as a loss but still saves their partial score.

**Arena layout — adapts fully to terminal size via ncurses `getmaxyx`:**

```
row 0       ╔══════════════════════════════════════════════╗
row 1       ║  TERMICRAFT: THE LAIR  │ [PHASE I] │ SCORE  ║
row 2       ║  DRAGON ████████████████------- 75%  HP:x/y ║
row 3       ╠══════════════════════════════════════════════╣
rows 4-15   ║   dragon ASCII art block (12 rows)          ║
rows 16+    ║   combat zone — fireballs (*) and arrows (^)║
row  H-5    ║   [/\=====/\]  (player sprite)              ║
row  H-4    ║   PLAYER ████████████------- 60%  [Iron]    ║
row  H-3    ╠══════════════════════════════════════════════╣
row  H-2    ║   [A] move left  [D] move right  [SPACE] .. ║
row  H-1    ╚══════════════════════════════════════════════╝
```

---

#### How Coding Elements Are Met

**Random events (Element 1):** Fireball spawn positions are derived from the
dragon's live `x` position, which changes every tick as the dragon bounces
unpredictably across the arena. Because the dragon starts at a fixed column but
immediately begins moving, and because fireball volleys are timed on a fixed
interval against an ever-changing dragon x, no two runs produce the same
fireball pattern. The opening roar damage (Easy: 20, Normal: 30, Hard: 45) is
also reduced by the player's armor percentage, producing a different value per
armor+difficulty combination. The opening shockwave animation uses
`frame % 2 == 0` to alternate between `'W'` and `'*'` characters each frame,
producing a pseudo-random visual at the impact point.

**Data structures (Element 2):** Four custom structs manage all fight state.
`BossConfig` stores all difficulty parameters built once at fight start.
`Dragon` tracks the full live state of the enemy. `Fireball` and `Arrow` each
hold position, an `active` flag, and the horizontal drift value `dx`. Both
projectile types are stored in fixed-size pool arrays (`fbs[FF_MAX_FIREBALLS]`,
`arrows[FF_MAX_ARROWS]`) so slots are recycled in place by toggling `active`
without any heap allocation during the game loop.

**Dynamic memory management (Element 3):** All projectile pools are
stack-allocated fixed-size arrays — no `new` or `malloc` anywhere in the fight
loop. The layout globals (`NC_ROWS`, `NC_COLS`, and all `ROW_*` variables) are
not hardcoded. They are recomputed from `getmaxyx()` every time the player
resizes the terminal (via `KEY_RESIZE`), allowing the arena to adapt at runtime
without reallocating any buffer.

**File I/O (Element 4):** At the end of every run (win or death), `score.cpp`'s
`saveFinalScore()` builds a `HighScore` struct and calls `fileio`'s
`addHighScore()`, which reads the existing top-10 list, appends the new entry,
sorts descending, trims to 10, and rewrites `termicraft_highscores.dat`. The
previous top score is snapshotted before the save so new-record detection works
correctly.

**Multiple files (Element 5):** The boss fight is split across `final_fight.h`
(constants, structs, public API) and `final_fight.cpp` (implementation). Score
logic lives in its own `score.h`/`score.cpp`. The fight integrates with
`fileio.cpp` for persistence, `score.cpp` for score updates, and `types.h` for
shared game state — no logic is duplicated across files.

**Difficulty levels (Element 6):** `initBossConfig()` maps the `Difficulty` enum
to a fully populated `BossConfig` struct. All fight parameters scale with
difficulty — dragon HP, fireball damage, fire rate, movement speed, Phase 3
spread, and whether Hard-mode enrage is active. All values reference named
`FF_*` constants from `final_fight.h` — no magic numbers appear in the game
loop.

---

#### Features

### Three-Phase Dragon

The dragon transitions through three phases as its HP drops, with each phase
making the fight harder. Phase transitions trigger a blinking centred
announcement banner that flashes for approximately one second (25 ticks).
`updatePhase()` is called every tick and checks HP percentage against
`FF_PHASE2_PCT` (66%) and `FF_PHASE3_PCT` (33%). Phase can only advance —
it never reverses. On each phase advance, dragon speed increases by 1 col/tick
above the base speed from `BossConfig`.

| HP remaining | Phase | Dragon colour | Eyes | Mouth | Speed bonus | Fireball pattern |
|---|---|---|---|---|---|---|
| 100% – 67% | Phase I | Green | `@ @` | `\VV/` | +0 | 1 straight down |
| 66% – 34% | Phase II | Yellow | `x @` | `\VV/` | +1 col/tick | 2 fireballs at ±3 cols |
| 33% – 0% | Phase III | Red + blink | `x x` | `\XX/` | +2 col/tick | 3 fireballs at centre ±5 cols |

On **Hard difficulty only**, the dragon also enrages when HP drops to or below
50%. `dragon.enraged` is set to true, speed increases by an additional 3
col/tick, `currentFireRate` is halved (minimum 5 ticks), and a red
`!! DRAGON ENRAGED !!` banner flashes for 2 seconds (40 ticks). Enrage triggers
only once per fight.

### Armor-Based HP and Damage

Player HP for the boss fight is calculated locally and kept separate from
`state.player.health` throughout the fight. It starts at a base value per
difficulty plus a flat armor bonus, with no upper cap:

| Difficulty | Base HP | + Stone (+15) | + Iron (+30) | + Gold (+50) | + Diamond (+80) |
|---|---|---|---|---|---|
| Easy | 20 | 35 | 50 | 70 | 100 |
| Normal | 15 | 30 | 45 | 65 | 95 |
| Hard | 10 | 25 | 40 | 60 | 90 |

Armor also reduces incoming fireball damage by a percentage, calculated inline
in the fight loop as `dmg = config.fireballDmg * (100 - armorPct) / 100`:

| Armor | Fireball damage reduction | Arrow damage to dragon per hit |
|---|---|---|
| None / Wood | 0% | 1 |
| Stone | 10% | 1 |
| Iron | 20% | 4 |
| Gold | 30% | 7 |
| Diamond | 45% | 12 |

The opening roar damage uses the same `armorPct` reduction. If the player enters
without diamond armor, the intro screen shows `DIAMOND ARMOR RECOMMENDED` in
reverse red text.

### Difficulty Scaling

| Parameter | Easy | Normal | Hard |
|---|---|---|---|
| Dragon HP | 50 | 80 | 140 |
| Fireball damage (before armor) | 4 HP | 9 HP | 15 HP |
| Fire rate | 35 ticks (~1.75s) | 18 ticks (~0.9s) | 12 ticks (~0.6s) |
| Dragon base speed | 1 col/tick | 2 col/tick | 3 col/tick |
| Enrage at 50% HP | No | No | Yes |
| Phase 3 fireball spread | ±5 cols | ±5 cols | ±8 cols |
| Score multiplier | ×1.0 | ×1.5 | ×2.0 |
| Opening roar damage (no armor) | 20 HP | 30 HP | 45 HP |

### Opening Animation

Before the main fight loop starts, `openingTicks` counts down from 50. Each
tick during this window calls `renderOpening()` instead of `renderFrame()` and
skips all normal game logic. A shockwave ring of `*` characters expands outward
from the arena centre — radius grows by 1 every 5 ticks. For the first 10
frames, a dense `'W'`/`'*'` cluster fills the centre on alternating frames.
A "ROOAARRR! Dragon breathes fire! -N HP" message blinks in reverse red every
5 frames. The roar damage is applied to `playerHp` on exactly tick 1 (clamped
to minimum 1 HP). The player can press `Q` to flee during the opening.

---

#### Function Reference — `final_fight.cpp`

### `initFightColors()`

Registers 14 ncurses colour pairs after `start_color()`. Uses
`use_default_colors()` so `-1` as the background preserves the terminal's own
background colour. The 14 pairs cover: border (cyan), title (white), HUD
(yellow), dragon phases (green/yellow/red), player (cyan), fireballs (red),
arrows (white), HP bar states (green/yellow/red), dimmed text, and warnings.
These pair IDs (`CP_BORDER` through `CP_WARN`) are referenced by every render
function via `attron(COLOR_PAIR(...))`.

### `computeLayout()`

Calls `getmaxyx(stdscr, NC_ROWS, NC_COLS)` to read the current terminal
dimensions, then calculates all `ROW_*` globals and dragon movement bounds.
The top rows are anchored from row 0 downward: HUD at row 1, dragon HP bar at
row 2, separator at row 3, dragon starts at row 4. The bottom rows are anchored
upward from `NC_ROWS`: controls at `NC_ROWS-2`, bottom separator at `NC_ROWS-3`,
player HP bar at `NC_ROWS-4`, player sprite at `NC_ROWS-5`. Dragon horizontal
bounds: `DRAG_MIN_X = 1`, `DRAG_MAX_X = NC_COLS - FF_DRAGON_COLS - 3`, with a
safety clamp so `DRAG_MAX_X` is always at least `DRAG_MIN_X + 4`. Called once
at fight start and again whenever `getch()` returns `KEY_RESIZE`.

### `drawFrame()`

Uses ncurses `box(stdscr, 0, 0)` to draw the outer border using terminal
line-drawing characters. Then draws two internal horizontal separators using
`mvhline()` with `ACS_HLINE`. Each separator's left and right endpoints are
replaced with `ACS_LTEE` and `ACS_RTEE` (the ╠ and ╣ characters) using
`mvaddch()`. All drawing is done with `CP_BORDER | A_BOLD` for a bold cyan
border.

### `drawBar(row, col, width, cur, maxV)`

Draws a filled HP bar of `width` characters at the given screen position.
Computes `filled = cur * width / maxV` and `pct = cur * 100 / maxV`. Colour is
chosen by percentage: `CP_HP_G` (green) above 50%, `CP_HP_Y` (yellow) above
20%, `CP_HP_R` (red) at or below 20%. Filled cells print `' '` with
`A_BOLD | A_REVERSE` so they appear as solid coloured blocks. Empty cells
print `'-'` with `CP_DIM | A_DIM`. Guards `maxV` against zero and clamps `cur`
to zero if negative.

### `renderHUD(score, dragon)`

Renders the top two rows of the arena. Row 1 has three sections: title
`"TERMICRAFT: THE LAIR"` at column 2 in white bold; the current phase string
`"[ PHASE I/II/III ]"` centred by computing
`midCol = (NC_COLS - strlen(phaseStr)) / 2` in the phase colour (green/yellow/
red); and the score `"SCORE: 000000"` right-aligned at
`NC_COLS - strlen(scoreBuf) - 1` in yellow bold. Row 2 shows `"DRAGON"` label
followed by a `drawBar()` call sized to `max(8, NC_COLS/2 - 16)`, then a
`" NNN%  HP: current/max"` suffix.

### `renderDragon(dragon, tick, hitFlash)`

Selects the art array via `DRAGON_ART[dragon.phase - 1]`, choosing `DRAGON_P1`,
`DRAGON_P2`, or `DRAGON_P3`. Sets the colour pair to `CP_DRAG_P1/P2/P3`
(green/yellow/red). For Phase 3, adds `A_BLINK` on alternating tick pairs
(`(tick/2) % 2 == 0`) making the near-death dragon flash. When `hitFlash` is
true (active for 4 ticks after an arrow lands), the entire block is drawn with
`A_REVERSE | A_BOLD` — a bright white flash — instead of the phase colour,
providing clear hit feedback. Each of the 12 art lines is printed character by
character starting at `1 + dragon.x`, clamped to `NC_COLS - 1`. Lines that
would overlap `ROW_PLAYER - 2` are clipped.

### `renderProjectiles(fbs, arrows)`

Iterates both projectile pools. For each active fireball, skips if its row is
on or above `ROW_TOP_SEP` or on or below `ROW_PLAYER`, and skips if outside the
border columns. Prints `'*'` at `(fbs[i].y, 1 + fbs[i].x)` in
`CP_FIRE | A_BOLD` (red bold). For each active arrow, prints `'^'` at
`(arrows[i].y, 1 + arrows[i].x)` in `CP_ARROW | A_BOLD` (white bold). The
`+1` column offset accounts for the left border character.

### `renderPlayer(playerX, playerHp, playerMaxHp, armor)`

Sprite is `"[/\=====/\]"`. Start column is `1 + playerX - strlen(sprite)/2`
so the sprite is centred on `playerX`, then clamped to stay inside the border
on both sides. Colour is chosen by HP percentage: `CP_PLAYER` (cyan) above 50%,
`CP_HP_Y` (yellow) above 20%, `CP_HP_R` (red) at or below 20% — a visual
danger warning. Below the sprite, `"PLAYER"` label is printed followed by a
`drawBar()` sized to `max(8, NC_COLS/3)`, then `" NNN%  [ArmorName armor]"`.

### `renderControls()`

Prints `"[A] move left    [D] move right    [SPACE] shoot    [Q] flee"` at
column 2 of `ROW_CTRL` using `CP_DIM | A_DIM` so it visually recedes behind
the active fight elements.

### `renderBanners(dragon)`

Only executes when `dragon.announceTicks > 0`. Computes
`blink = (dragon.announceTicks / 5) % 2 == 0` so the banner alternates visible
and invisible every 5 ticks (0.25 seconds). When visible, selects the message:
`"!! DRAGON ENRAGED !!"` if `dragon.enraged` is true, otherwise
`"-- PHASE III --"` or `"-- PHASE II --"` by current phase. Enrage uses
`CP_WARN` (red); phase banners use `CP_HUD` (yellow). Both use
`A_BOLD | A_REVERSE`. Banner is horizontally centred using
`(NC_COLS - strlen(msg)) / 2`.

### `renderOpening(playerX, playerHp, playerMaxHp, score, dragon, armor, openingTicks, roarDmg)`

Called once per tick during the 50-tick opening sequence. Computes
`frame = 50 - openingTicks` (0–49). `radius = frame / 5` grows from 0 to the
maximum that fits in the combat zone. The shockwave ring is drawn by iterating
the top and bottom horizontal edges, then the left and right vertical edges,
placing `'*'` at each valid position bounded by `ROW_TOP_SEP` and `ROW_PLAYER`.
For the first 10 frames, a 3×5 cluster at the arena centre alternates `'W'` and
`'*'`. The roar message blinks by only rendering when `(frame/5) % 2 == 0`.
Player sprite and controls are always rendered so the HP drop is visible.

### `renderFrame(dragon, fbs, arrows, playerX, playerHp, playerMaxHp, score, armor, tick, hitFlash)`

Composes one complete game frame by calling all sub-renderers in z-order:
`erase()` to blank the previous frame, `drawFrame()`, `renderHUD()`,
`renderDragon()`, `renderProjectiles()`, `renderPlayer()`, `renderControls()`,
`renderBanners()`, then `refresh()` to flush everything to the terminal in one
pass. The erase-then-refresh pattern is what gives ncurses its flicker-free
output.

### `showIntro(state)`

Renders a full-screen ncurses intro. The Phase 1 dragon art is centred
vertically at `NC_ROWS/2 - 9` and horizontally at `(NC_COLS - artWidth) / 2`,
printed in `CP_DRAG_P3 | A_BOLD` (red bold). The `"~ THE DRAGON CAVE ~"` title
is printed two rows above the art. Below the art, a lore line and stats line
show armor name, current HP, and score. If armor is below `MATERIAL_DIAMOND`,
`DIAMOND ARMOR RECOMMENDED` is printed in `CP_WARN | A_BOLD | A_REVERSE`.
Then `nodelay(stdscr, FALSE)` is set so `getch()` blocks until the player presses
any key, after which `nodelay(stdscr, TRUE)` is restored for the game loop.

### `showScoreBreakdown(won, miningSnap, p1h, p2h, p3h, killBonus, total, mult)`

Called after `endwin()` so it uses ANSI escape codes via `std::cout` rather than
ncurses. Uses `clearAndCenterV()` to vertically centre the content and `hpad()`
to compute horizontal padding strings for centring each art block and the table.
Prints a large block-letter `"YOU WIN"` banner in green or `"GAME OVER"` banner
in red using Unicode block characters. Then builds a bordered score table using
UTF-8 box-drawing bytes. Rows are added for: mining score carried in, Phase I/II/III
hits with raw points multiplied by `mult`, and kill bonus if applicable. The total
row is printed in yellow bold. Calls `getTopHighScore()` to check if the new total
beats the stored record and prints `"** NEW HIGH SCORE! **"` if so. Reads one
keypress with `read(STDIN_FILENO, &dummy, 1)` to pause before returning.

### `initBossConfig(diff)`

Switches on the `Difficulty` enum and fills a `BossConfig` struct. Easy:
`dragonHp=50`, `fireballDmg=4`, `fireRateTicks=35`, `dragonSpeed=1`,
`hasEnrage=false`, `spread3=FF_SPREAD_P3`. Normal: HP 80, dmg 9, rate 18,
speed 2, no enrage, standard spread. Hard: HP 140, dmg 15, rate 12, speed 3,
`hasEnrage=true`, `spread3=8` (wider than the standard 5). Returns by value.

### `calcFightHP(diff, armor)`

Reads the base HP constant for the difficulty (`FF_BASE_HP_EASY/NORMAL/HARD`),
then adds the flat armor bonus constant (`FF_HP_BONUS_STONE/IRON/GOLD/DIAMOND`)
for the given `MaterialTier`. `MATERIAL_NONE` and `MATERIAL_WOOD` fall through
to `bonus=0`. Returns `base + bonus` with no upper cap — armor always adds on
top regardless of difficulty.

### `calcArmorDamage(armor)`

A switch returning `FF_DMG_IRON` (4), `FF_DMG_GOLD` (7), or `FF_DMG_DIAMOND`
(12) for the respective tiers. All other tiers (None, Wood, Stone) return
`FF_DMG_DEFAULT` (1). This value is stored as `arrowDmg` in `runBossFight()` and
applied as `dragon.hp -= arrowDmg` on every arrow hit.

### `initDragon(cfg)`

Zero-initialises a `Dragon` struct with `= {}`, then sets: `x = FF_DRAGON_COLS`
(one art-block-width from the left border), `y = ROW_DRAG_START`,
`hp = maxHp = cfg.dragonHp`, `speed = cfg.dragonSpeed`, `direction = 1`
(starts moving right), `phase = FF_PHASE1`, `enraged = false`,
`announceTicks = 0`. Returns by value.

### `updatePhase(dragon, baseSpeed)`

Guards immediately if `dragon.hp <= 0`. Computes
`pct = dragon.hp * 100 / dragon.maxHp`. Determines `newPhase` as `FF_PHASE3`
if `pct <= 33`, `FF_PHASE2` if `pct <= 66`, else `FF_PHASE1`. Only applies
the transition if `newPhase > dragon.phase` (one-way only). On advance, sets
`dragon.phase = newPhase` and `dragon.speed = baseSpeed + (dragon.phase - 1)`
so Phase 2 adds +1 and Phase 3 adds +2 above base.

### `spawnFireballs(fbs, dragon, spread3)`

Computes the dragon's horizontal centre as `cx = dragon.x + FF_DRAGON_COLS / 2`.
Spawn row is `ROW_DRAG_END + 1` (just below the dragon body). Builds arrays
`spawnX[]` and `spawnDX[]` per phase: Phase 1 — one fireball at `cx`, `dx=0`;
Phase 2 — two at `cx ± FF_SPREAD_P2` with `dx = ∓1` (diverging); Phase 3 —
three at `cx - spread3`, `cx`, `cx + spread3` with `dx = -1, 0, +1`. Iterates
the `fbs` pool to find the first `cnt` inactive slots and activates them with
struct literal assignment. If all slots are active, the volley is silently
dropped.

### `fireArrow(arrows, playerX)`

Iterates the `arrows` pool and activates the first inactive slot as
`{playerX, ROW_PLAYER - 1, true}` (spawns one row above the player sprite).
Returns immediately after activating one slot — one arrow per SPACE press.
If all `FF_MAX_ARROWS` slots are active the shot is silently dropped.

### `getArmorName(armor)`

Returns a `const char*` display string for each `MaterialTier`: `"Stone"`,
`"Iron"`, `"Gold"`, `"Diamond"`, or `"None"` for all other values. Used in
`renderPlayer()` for the HP bar suffix and in `showIntro()` for the stats line.

### `runBossFight(state)` — Main Entry Point

The full sequence of execution inside this function:

1. `setlocale(LC_ALL, "")` enables UTF-8 so ncurses renders Unicode box
   characters correctly on the HKU server.
2. `initscr()`, `cbreak()`, `noecho()`, `nodelay(stdscr, TRUE)`,
   `keypad(stdscr, TRUE)`, `curs_set(0)` configure ncurses: raw input, no echo,
   non-blocking `getch()`, arrow key support, cursor hidden.
3. `initFightColors()` and `computeLayout()` register colour pairs and compute
   all layout row globals from live terminal dimensions.
4. Terminal size check: if `NC_ROWS < 24` or `NC_COLS < 60`, calls `endwin()`,
   prints an ANSI error message with current dimensions, reads one keypress,
   returns `false`.
5. Builds `BossConfig` from `state.difficulty`. Reads `playerMaxHp` and
   `playerHp` directly from `state.player.maxHealth` and `state.player.health`
   (the values set by the broader game before entering the fight — `calcFightHP()`
   is defined for external use but is not called here). Computes
   `arrowDmg` from `calcArmorDamage()` and `armorPct` (0/10/20/30/45%) from a
   switch on armor tier.
6. Snapshots `miningSnap = state.score` to separate mining points from boss
   points in the post-fight breakdown.
7. Computes `roarDmg`: Easy 20, Normal 30, Hard 45, then reduces by
   `(100 - armorPct) / 100` and clamps to minimum 5.
8. Sets `fireballTick = -20` giving a 20-tick grace period before the first
   fireball volley. Sets `openingTicks = 50`.
9. Calls `showIntro(state)` — blocking ncurses intro.
10. Enters `while (running)`, ticking once per `usleep(FF_TICK_US)` (50ms):
    - Applies roar damage on `tick == 1`, clamped to minimum 1 HP.
    - If `openingTicks > 0`: decrements, drains input for Q only, calls
      `renderOpening()`, sleeps, `continue`s — all normal game logic is skipped.
    - Drains the full key buffer with `while (getch() != ERR)`, setting boolean
      flags `wantLeft`, `wantRight`, `wantShoot`. This allows move and shoot
      to both register in the same tick. Handles `KEY_RESIZE` by calling
      `computeLayout()`.
    - Moves dragon: `dragon.x += speed * direction`, flips `direction` on
      `DRAG_MIN_X`/`DRAG_MAX_X` walls.
    - Spawns fireballs when `++fireballTick >= currentFireRate`, resets counter.
    - Moves fireballs: `y++`, `x += dx`, deactivates if below `ROW_PLAYER` or
      outside columns.
    - Moves arrows: `y--`, deactivates if above `ROW_TOP_SEP`.
    - Arrow-dragon collision: for each active arrow checks
      `hitRow = y in [ROW_DRAG_START, ROW_DRAG_END]` and
      `hitCol = x in [dragon.x, dragon.x + FF_DRAGON_COLS)`. On hit: deactivates
      arrow, applies `dragon.hp -= arrowDmg`, calls `addScore(state, pts)`
      with phase-appropriate raw points (10/20/30), increments phase hit
      counter, sets `flashTicks = 4`.
    - Fireball-player collision: for each active fireball checks
      `y == ROW_PLAYER` and `x in [playerX-HALF, playerX+HALF]`. On hit:
      deactivates fireball, computes `dmg = config.fireballDmg*(100-armorPct)/100`
      (minimum 1), subtracts from `playerHp`.
    - Calls `updatePhase()`. If phase advanced, sets `announceTicks = 25`.
    - Hard enrage check: if `config.hasEnrage`, not yet enraged, HP > 0, and
      `hp * 100 / maxHp <= 50`: sets `dragon.enraged = true`, adds 3 to speed,
      halves `currentFireRate` (minimum 5), sets `announceTicks = 40`.
    - Decrements `announceTicks` if > 0.
    - Win check: if `dragon.hp <= 0`, calls `addScore(state, FF_SCORE_KILL)`,
      sets `killBonus = true`, `won = true`, `running = false`.
    - Loss check: if `playerHp <= 0` and still running, sets `won = false`,
      `running = false`.
    - Calls `renderFrame()`, then `usleep(FF_TICK_US)`.
11. `endwin()` restores the terminal to its pre-ncurses state.
12. Sets `state.dragonDefeated = won` and `state.phase` to `PHASE_VICTORY` or
    `PHASE_GAMEOVER`.
13. Calls `showScoreBreakdown()`. Note: `saveFinalScore()` is **not** called from `runBossFight()` — scores are not currently saved to the leaderboard file. This is a known gap in the implementation.
14. Returns `won`.

---

---

### Score System (`score.h` / `score.cpp`)

---

#### Overview

The score system provides the single point of truth for all score increments in
the game. Instead of each module applying the difficulty multiplier
independently — which risks inconsistency if one module uses a different formula
— both `player.cpp` and `final_fight.cpp` call `addScore()` every time points
are earned. The module also handles end-of-run high score saving, delegating all
actual file operations to `fileio.cpp`.

---

#### How Coding Elements Are Met

**Data structures (Element 2):** Uses the shared `HighScore` struct from
`types.h` — no duplicate struct definitions. `saveFinalScore()` populates all
five fields (`playerName`, `score`, `difficulty`, `timestamp`,
`defeatedDragon`) before passing the struct to `fileio`.

**File I/O (Element 4):** `score.cpp` delegates all file operations to
`fileio.cpp`. `saveFinalScore()` calls `addHighScore()` (sorts top 10, writes
`termicraft_highscores.dat`) and `getTopHighScore()` (reads current best).
`score.cpp` itself never opens, reads, or writes any file directly.

**Multiple files (Element 5):** `score.h` exposes only two public functions.
`player.cpp` and `final_fight.cpp` both `#include "score.h"` and call
`addScore()` — the multiplier logic exists in exactly one place, making it
impossible for the two modules to apply the multiplier differently.

**Difficulty levels (Element 6):** `addScore()` reads
`state.settings.scoreMultiplier`, which is set to 1.0, 1.5, or 2.0 by
`getDifficultySettings()` in `types.h` when the game starts. Every single point
earned anywhere in the game — mining or boss fight — is automatically multiplied
by the difficulty level.

---

#### Features

### Centralised Score Calculation

`addScore()` is the only function in the codebase that modifies `state.score`
during gameplay. Both `player.cpp` (on every block mined) and `final_fight.cpp`
(on every arrow hit and kill bonus) call it. This means the difficulty multiplier
is applied identically to every point earned regardless of source. The function
also guards against negative input so no buggy call can reduce the score.

Score awarded during the boss fight:

| Event | Raw points | Easy (×1.0) | Normal (×1.5) | Hard (×2.0) |
|---|---|---|---|---|
| Arrow hit — Phase I | 10 | 10 | 15 | 20 |
| Arrow hit — Phase II | 20 | 20 | 30 | 40 |
| Arrow hit — Phase III | 30 | 30 | 45 | 60 |
| Dragon kill bonus | 500 | 500 | 750 | 1000 |

Score awarded during mining (via `player.cpp`):

| Block | Raw points | Easy | Normal | Hard |
|---|---|---|---|---|
| Wood | 1 | 1 | 1 | 2 |
| Stone / Coal | 2 | 2 | 3 | 4 |
| Iron | 3 | 3 | 4 | 6 |
| Gold | 4 | 4 | 6 | 8 |
| Diamond | 5 | 5 | 7 | 10 |

### High Score Save — Race Condition Fixed

A subtle bug was caught during development: calling `getTopHighScore()` after
`addHighScore()` would compare the new score against itself (since it is now
the top of the file), causing `isNewBest` to nearly always be true. The correct
order is:

1. Call `getTopHighScore()` — snapshot the previous best before the save.
2. Call `addHighScore()` — write the new score to disk.
3. Compare `state.score > prevTop.score` using the pre-save snapshot.

This ensures `"NEW ALL-TIME HIGH SCORE"` only appears when the score genuinely
exceeds the previous stored record. Tied scores do not trigger it (`>` not `>=`).

Partial scores from a death mid-fight are fully eligible. `saveFinalScore()` is
always called regardless of win or loss, and `defeatedDragon` is set to `false`
on death so the leaderboard records it honestly.

---

#### Function Reference — `score.cpp`

### `addScore(state, rawPoints)`

The sole function that increments `state.score` anywhere in the codebase during
gameplay. First checks `if (rawPoints < 0) return` — a defensive guard
preventing any negative call from modifying the score (zero is allowed through,
which is harmless). Then computes:

```cpp
state.score += static_cast<int>(rawPoints * state.settings.scoreMultiplier)
```

The `static_cast<int>` truncates the fractional remainder consistently — for
example, 10 × 1.5 = 15 (no remainder); 7 × 1.5 = 10.5 → 10. This truncation
pattern is consistent with the original inline calculation previously in
`player.cpp` and with the rest of the codebase.

### `saveFinalScore(state, defeatedDragon)`

End-of-run save flow:

1. Reads `state.player.name` directly — no second prompt since the player
   entered their name at game start via `main.cpp`'s `getPlayerName()`. Falls
   back to `"Anonymous"` if empty; truncates to 16 characters if too long.
2. Builds a `HighScore` struct (from `types.h`) with: `playerName` (the
   sanitised name), `score = state.score` (full run total: mining + boss),
   `difficulty = state.difficulty`, `timestamp = time(nullptr)` (Unix timestamp
   for leaderboard sorting), `defeatedDragon` (false on death or flee).
3. Calls `getTopHighScore()` to snapshot `prevTop` **before** the save to avoid
   the race condition described above.
4. Calls `addHighScore(hs)` — `fileio.cpp` loads the current top-10 list from
   `termicraft_highscores.dat`, appends the new record, sorts all entries
   descending by score, trims to 10, writes back to file, and returns `true` if
   the new score made the cut.
5. Computes `isNewBest = (state.score > prevTop.score)` using the pre-save
   snapshot.
6. Prints one of three confirmation messages depending on the outcome:
   `"** NEW ALL-TIME HIGH SCORE! **"` in green if `isNewBest`,
   `"Score saved — you made the top 10!"` in green if `madeList` but not the
   new best, or `"Score recorded."` in plain text if the score did not make
   the top 10.

---

### Non-Standard Libraries

#### `ncurses` (`<ncurses.h>`)

ncurses is the only non-standard library used across these modules. It is
pre-installed on Ubuntu Linux and on the HKU CS academy server
(`academy11.cs.hku.hk`, `academy21.cs.hku.hk`) — no additional installation is
required by the grader. `score.cpp` uses no external libraries.

**Features supported by ncurses in `final_fight.cpp`:**

- **Flicker-free rendering** — `erase()` clears the virtual screen buffer each
  tick, then `refresh()` flushes it to the terminal in one pass. This prevents
  the visible tearing that occurs when clearing and redrawing with `printf` or
  `cout`.
- **Colour output** — 14 colour pairs give the dragon, fireballs, arrows, HP
  bars, and HUD each a distinct colour. HP bars shift green → yellow → red as HP
  falls below 50% and 20%.
- **Non-blocking input** — `nodelay(stdscr, TRUE)` makes `getch()` return `ERR`
  immediately when no key is pressed, so the loop runs at a fixed tick rate via
  `usleep()`. Re-enabled blocking (`nodelay FALSE`) only inside `showIntro()`
  where the game needs to wait for a keypress.
- **Terminal resize handling** — `keypad(stdscr, TRUE)` enables `KEY_RESIZE`
  detection. When received in the input drain loop, `computeLayout()` is called
  immediately so the arena reflows to the new dimensions without restarting.
- **Dynamic layout** — `getmaxyx(stdscr, NC_ROWS, NC_COLS)` inside
  `computeLayout()` reads live terminal dimensions. All row anchors and dragon
  movement bounds are recalculated from these values, so the arena works at any
  terminal size above the minimum (60×24).
- **Box drawing** — `box(stdscr, 0, 0)` draws the outer border. `mvhline()` with
  `ACS_HLINE` and `mvaddch()` with `ACS_LTEE`/`ACS_RTEE` draw the two internal
  separators (╠ and ╣ corners).
- **Text attributes** — `A_BOLD` for all active elements; `A_BLINK` on the
  Phase 3 dragon; `A_REVERSE` for hit-flash and phase/enrage banners; `A_DIM`
  for the dimmed controls row.

All other headers (`<unistd.h>`, `<locale.h>`, `<algorithm>`, `<cstdio>`,
`<sstream>`, `<iomanip>`) are standard C/C++ headers requiring no installation.

### Crafting & Equipment System (`crafting.h`, `crafting.cpp`)

Provides a full-screen terminal UI for the crafting bench with 10 tiered recipes (Wood through Diamond for pickaxes and armor). Implements strict progression gating where pickaxe upgrades beyond Stone trigger a "Rite of Passage" minigame before the upgrade is confirmed. Features real-time resource validation, color-coded availability indicators, and detailed inventory display with health bonuses for armor upgrades.

**How coding elements are met:**

- **Data structures (Element 2):** Uses a static constant array of `CraftingRecipe` structs (10 entries) defining resource costs, prerequisites, and display metadata. Validates against `Inventory` and `Equipment` structs from `types.h` to determine craftability. Equipment tiers are enforced via the `MaterialTier` enum.

- **Multiple files (Element 5):** Modular design with `crafting.h` exposing the menu interface and `crafting.cpp` containing UI rendering and validation logic. Integrates with `player.h` for progression checks (`checkCraftingProgression`) and healing (`healPlayer`), `colors.h` for ANSI color coding, and `menu.h` for terminal input handling (`getch`, `clearScreen`).

- **Difficulty levels (Element 6):** Equipment crafting follows strict tier progression (Wood→Stone→Iron→Gold→Diamond). Pickaxe upgrades beyond Stone trigger a randomized minigame (Wordle, Minesweeper, 24 Game, or Sudoku) via the player module before the upgrade is confirmed, effectively gating high-tier content behind skill-based challenges. Armor upgrades provide incremental max health bonuses (+5 to +25 HP) that aid survival on higher difficulties.

---

### Non-Standard Libraries

None. All libraries used by `final_fight.cpp` and `score.cpp` are standard
on Linux and require no additional installation:

| Header       | Purpose                                      |
|--------------|----------------------------------------------|
| `<fcntl.h>`  | Non-blocking terminal input via `fcntl()`    |
| `<unistd.h>` | `usleep()` for game tick timing, `read()`    |
| `<ctime>`    | `time()` for high score timestamps           |

Terminal rendering uses ANSI escape codes via the team's `colors.h`

### Wordle Minigame (`wordle.cpp`)

A terminal-based word deduction puzzle integrated into TermiCraft. The player must guess a hidden word within a fixed number of attempts, receiving feedback after each guess in the form of color-coded tiles. The objective is to infer the correct word using letter position logic and elimination strategy.

The system follows a strict evaluation model identical to the original Wordle ruleset, including duplicate-letter handling and position-sensitive scoring. Each guess is validated against a predefined dictionary list to ensure only valid words are accepted.

The UI is fully rendered using ANSI escape codes from `colors.h`, with a structured grid layout that redraws the full board each turn to maintain consistency and alignment. A persistent alphabet tracking system is maintained internally to accumulate knowledge of letter states across guesses.

Word selection is performed from static word lists grouped by difficulty, ensuring consistent vocabulary constraints and reproducible gameplay behavior.

---

#### How Coding Elements Are Met

**Random events (Element 1):**  
The target word is selected using `rand()` from a predefined vector of valid words corresponding to the chosen difficulty. Each execution produces a different hidden word, assuming a non-fixed seed.

---

**Data structures (Element 2):**  

- `std::vector<std::string>` for word banks and guess history  
- A per-guess evaluation grid storing tile states  
- A 26-element array tracking cumulative letter states (correct, present, absent)

Each guess is stored immutably and re-rendered each frame for display.

---

**Algorithmic logic:**  
Evaluation uses a two-pass system:

1. First pass: mark correct letter + correct position (green)
2. Second pass: mark correct letter but wrong position (yellow), ensuring each letter in the target word is only consumed once

This prevents incorrect duplicate scoring for repeated letters.

---

**Multiple files (Element 5):**  
The module is encapsulated and exposed through a single entry function. It integrates with:

- `colors.h` for ANSI rendering  
- `menu.h` for input handling consistency  

No external global state is required.

---

**Difficulty levels (Element 6):**  

- Easy: 4-letter words (small vocabulary, high frequency words)  
- Normal: 5-letter words (balanced difficulty)  
- Hard: 6-letter words (lower frequency vocabulary, higher deduction complexity)

Word length directly scales the entropy of the solution space.

---

---

### Sudoku Minigame (`sudoku.cpp`)

A terminal-based constraint satisfaction puzzle integrated into TermiCraft. The player completes a partially filled Sudoku grid while respecting strict row, column, and subgrid constraints.

The system dynamically generates a valid Sudoku board at runtime using recursive backtracking, then removes values according to difficulty to create a playable puzzle. Each board is guaranteed to be solvable.

The rendering system uses ANSI formatting to visually separate:

- Fixed clues (non-editable)
- Player inputs
- Empty cells

The board is fully re-rendered after every move to maintain alignment and consistency.

---

#### Board Generation

1. Generate a fully valid completed Sudoku grid using backtracking
2. Randomly shuffle candidate values for variability
3. Remove cells based on difficulty level while preserving solvability

---

#### How Coding Elements Are Met

**Random events (Element 1):**  

- Backtracking solution generation combined with shuffled candidate order  
- Randomized cell removal when creating the puzzle  

Each run produces a unique valid board configuration.

---

**Data structures (Element 2):**  

- `std::vector<std::vector<int>>` for the Sudoku grid  
- `std::vector<std::vector<bool>>` mask for fixed cells  

This ensures separation between immutable clues and mutable player input.

---

**Algorithmic logic:**  
A move is valid only if:

- The number is not already present in the row  
- The number is not present in the column  
- The number is not present in the subgrid  

Subgrid size depends on difficulty (2×3 or 3×3).

---

**Multiple files (Element 5):**  
The module is structured as:

- `sudoku.h` (interface)
- `sudoku.cpp` (implementation)

It integrates with shared UI and input systems via `menu.h`.

---

**Difficulty levels (Element 6):**  

- Easy / Normal:
  - 6×6 grid  
  - 2×3 subgrid structure  
  - Higher clue density  

- Hard:
  - 9×9 grid  
  - 3×3 subgrid structure  
  - Lower clue density  

Increasing difficulty reduces initial information and increases constraint complexity.

---

### Minesweeper Minigame (`minesweeper.cpp`)

A classic logic-based minigame triggered when players try to mine ores. The game dynamically generates a solvable minefield using random placement and provides real-time feedback through an ASCII board with numbered hints on adjacent mines, flagging, and flood-fill reveal mechanics. Players interact with the puzzle by typing an action (F - flag, R - reveal, Q - quit) and corresponding x y coordinates of the cell.

**How coding elements are met:**

- **Random events (Element 1):** Puzzle generation uses srand() seeded with current time and random mine placement to ensure every board is unique and fair. The number and positions of mines vary each playthrough.

- **Storing data (Element 2):** Board states are managed with three parallel 2D std::vector<std::vector<...>> grids (mineGrid, solutionGrid, and revealedGrid). High scores (time + player name) are stored persistently using file I/O in showHighScore() / saveHighScore(), allowing leaderboard persistence across game sessions.

- **Dynamic memory management (Element 3):** All board grids are allocated dynamically at runtime using std::vector with sizes determined by the chosen difficulty. Memory is automatically managed by the vector destructors when the Minesweeper object goes out of scope, preventing leaks even on early game exit.

- **Multiple difficulty levels (Element 6):** Difficulty is scaled by changing grid size and mine number:
  - **Easy:** 6x6 grid with 7 mines
  - **Medium:** 8x8 grid with 12 mines
  - **Hard:** 10x10 grid with 20 mines
  
---

### Twentyfour Minigame (`twentyfour.cpp`, `evaluator.cpp`)

A logic and arithmetic-based minigame used during equipment progression in TermiCraft. The player receives four cards and must use each value exactly once with +, -, *, / and parentheses to reach exactly 24.

**How coding elements are met:**

- **Random events (Element 1):** Puzzle selection uses rand() seeded with current time to pick a random valid 4-number puzzle from the bank each time, ensuring every session feels unique.

- **Storing data (Element 2):** The TwentyFour class uses std::vector containers to manage game-related data. The card data for each round is stored in picked, and the puzzles are kept in allPuzzles, a 2D vector populated from twentyfourpuzzles.csv.

- **Dynamic memory management (Element 3):** All puzzle storage and card vectors are allocated dynamically with std::vector. Memory is automatically cleaned up when the TwentyFour object is destroyed.

- **File input (Element 4):** loadPuzzleNumbers() and parseNumbers() read and parse the twentyfourpuzzles.csv file, skipping the header and parsing the puzzle to be in an easy-to-process format.

- **Program codes in multiple files (Element 5):** Core game logic is in twentyfour.cpp / twentyfour.h, while expression parsing and evaluation are separated into evaluator.cpp / evaluator.h for clean modularity.

- **Multiple difficulty levels (Elemen 6):** Difficulty is controlled by attempts and timeLimit parameters passed to playGame():
  - **Easy:** 5 attempts, 180 seconds
  - **Medium:** 3 attempts, 90 seconds
  - **Hard:** 1 attempt, 30 seconds
