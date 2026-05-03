# COMP2113-Group-21-Project

## Team Members

| Name | Student ID | Role |
| :--- | :--- | :--- |
| Sohan | 3036636025 | Boss fight, score system |
| Aryan | 3036484587 | Wordle Implementation, Sudoku Implementation |
| Koki | 3036505795 | Player controller, mining system, crafting system |
| Nan | 3036475225 | Minesweeper implementation, Twentyfour implementation |
| Saarim | 3036520068 | Main game logic, menu, integration of minigames, display, types, save file, makefile|
| Mohit | 3036517750 | World generation, fog of war, day and night(setting) |

---

## Features Implemented

### Player Controller & Mining System (`player.h`, `player.cpp`)

Implements the core player entity with real-time keyboard input handling (WASD movement, SPACE mining), physics simulation (gravity fall when standing over air), and combat mechanics. Manages the strict equipment progression system, triggers minigames during mining attempts based on calculated probability, and handles dynamic enemy spawning from disturbed blocks.

**How coding elements are met:**

- **Random events (Element 1):** Enemy spawning uses `rand() % 100` against `enemySpawnChance` settings after each successful mine. Mining minigame triggers use a calculated probability formula (`baseChance * (MINIGAME_COUNT / NUM_DISTINCT_ORES)`) with random rolls in the range [10%, 16%] to determine if a challenge occurs; dragon caves always trigger challenges regardless of roll.

- **Data structures (Element 2):** Defines and manipulates `GameState` references containing `Position`, `Inventory`, `Equipment`, and `Enemy` structs. Uses `std::vector<Enemy>` to dynamically track active cave enemies with their health, position, and damage stats.

- **Dynamic memory management (Element 3):** Enemy entities are dynamically added to the game world via `state.enemies.push_back(e)` when spawn conditions are met after mining. The vector automatically handles memory allocation for the enemy pool; no manual `new`/`delete` is required.

- **File I/O (Element 4):** Integrates with the score system by calling `addScore()` from `score.h` upon successful mining, which delegates to `fileio` for persistent high score storage. Does not perform direct file operations, maintaining clean separation of concerns.

- **Multiple files (Element 5):** Split across `player.h` (interface) and `player.cpp` (implementation). Integrates with `types.h` (shared state), `colors.h` (rendering), `menu.h` (UI utilities), `crafting.h` (equipment checks), and `score.h` (persistence). The minigame initialization uses `std::shuffle` from `<algorithm>` on a static array of minigame types.

- **Difficulty levels (Element 6):** Reads difficulty settings from `GameState` to scale enemy health (`enemyHealthMult`), minigame damage (`minigameDamage`), and spawn rates. Tool requirements for mining blocks create a soft difficulty curve (hands → wood → stone → iron → gold → diamond), while dragon cave blocks remain accessible regardless of tier, providing risk/reward choices on higher difficulties.

---

### Final Boss Fight (`final_fight.h`, `final_fight.cpp`)

A Space Invaders-style dragon boss fight triggered when the player enters
the dragon cave. The dragon moves as a full ASCII art block across a 100x35
terminal arena, bouncing off walls and accelerating through three phases as
its HP drops. The player fires rapid-fire arrows upward while dodging
fireballs that spread wider in later phases. The fight ends in either victory
or death, both of which trigger a score save.

**How coding elements are met:**

- **Random events (Element 1):** Fireballs are spawned at positions derived
  from the dragon's current x position plus a random horizontal drift offset
  (using `rand()` seeded at fight start), ensuring no two volleys land in
  exactly the same pattern.

- **Data structures (Element 2):** Three custom structs — `Dragon`,
  `Fireball`, and `Arrow` — store all live fight state. `BossConfig` holds
  all difficulty parameters. Fixed-size pool arrays (`fireballs[FF_MAX_FIREBALLS]`,
  `arrows[FF_MAX_ARROWS]`) manage projectiles without heap allocation during
  the fight loop.

- **Dynamic memory management (Element 3):** The static screen buffer
  `screenBuf[FF_ARENA_HEIGHT][FF_ARENA_WIDTH+1]` is allocated once as a
  module-level static array, avoiding repeated heap allocation on every tick.
  All projectile slots are reused in place via the `active` flag rather than
  allocating and freeing each shot.

- **File I/O (Element 4):** At the end of every run (win or death),
  `saveFinalScore()` in `score.cpp` builds a `HighScore` record and delegates
  to `fileio`'s `addHighScore()`, which writes the top-10 leaderboard to
  `termicraft_highscores.dat`. The fight reads the previous best via
  `getTopHighScore()` before saving to correctly detect a new record.

- **Multiple files (Element 5):** The boss fight is split across
  `final_fight.h` (constants, structs, public declarations) and
  `final_fight.cpp` (full implementation). It integrates with `fileio.cpp`
  for persistence, `score.cpp` for score calculation, `colors.h` for ANSI
  rendering, and `types.h` for shared game state — no logic is duplicated
  across files.

- **Difficulty levels (Element 6):** `initBossConfig()` reads the
  `Difficulty` enum from `GameState` and sets dragon HP (50/80/120),
  fireball damage (2/5/10 HP per hit), fire rate (~2.0s/1.2s/0.7s), and
  dragon movement speed (1/2/3 cols per tick). All values are sourced from
  named constants in `final_fight.h` — no magic numbers in the fight loop.

---

### Score System (`score.h`, `score.cpp`)

Centralised score calculation module used by both `player.cpp` (mining) and
`final_fight.cpp` (boss fight). Ensures the difficulty score multiplier
(1.0x / 1.5x / 2.0x) is applied identically and in one place across the
entire codebase.

`addScore(state, rawPoints)` — increments `state.score` by
`rawPoints * state.settings.scoreMultiplier`. Called on every block mined
and on every arrow hit during the boss fight. Guards against negative input.

`saveFinalScore(state, defeatedDragon)` — called once at run end. Uses
`state.player.name` (set at game start) to build a `HighScore` struct and
delegates to `fileio`'s `addHighScore()` for top-10 sorting and disk write.
Snapshots the previous top score *before* saving to avoid a race condition
where `getTopHighScore()` would otherwise return the score just written.

**How coding elements are met:**

- **Data structures (Element 2):** Uses the shared `HighScore` struct from
  `types.h` — no duplicate struct definitions.

- **File I/O (Element 4):** Delegates all file operations to `fileio.cpp`.
  `score.cpp` does not open or write any files directly, keeping I/O
  ownership clear.

- **Multiple files (Element 5):** `score.h` exposes only the two public
  functions. `player.cpp` and `final_fight.cpp` both include `score.h` and
  call `addScore()` — the multiplier logic exists in exactly one place.

### Crafting & Equipment System (`crafting.h`, `crafting.cpp`)

Provides a full-screen terminal UI for the crafting bench with 10 tiered recipes (Wood through Diamond for pickaxes and armor). Implements strict progression gating where Iron, Gold, and Diamond upgrades trigger "Rite of Passage" minigames before completion. Features real-time resource validation, color-coded availability indicators, and detailed inventory display with health bonuses for armor upgrades.

**How coding elements are met:**

- **Data structures (Element 2):** Uses a static constant array of `CraftingRecipe` structs (10 entries) defining resource costs, prerequisites, and display metadata. Validates against `Inventory` and `Equipment` structs from `types.h` to determine craftability. Equipment tiers are enforced via the `MaterialTier` enum.

- **Multiple files (Element 5):** Modular design with `crafting.h` exposing the menu interface and `crafting.cpp` containing UI rendering and validation logic. Integrates with `player.h` for progression checks (`checkCraftingProgression`, `performCrafting`), `colors.h` for ANSI color coding, and `menu.h` for terminal input handling (`getch`, `clearScreen`).

- **Difficulty levels (Element 6):** Equipment crafting follows strict tier progression (Wood→Stone→Iron→Gold→Diamond). Iron, Gold, and Diamond upgrades require completing minigame challenges (Wordle/Minesweeper) via the player module before the upgrade applies, effectively gating high-tier content behind skill-based challenges that scale with the desired equipment level. Armor upgrades provide incremental max health bonuses (+5 to +40 HP) that aid survival on higher difficulties.

---

## Non-Standard Libraries

None. All libraries used by `final_fight.cpp` and `score.cpp` are standard
on Linux and require no additional installation:

| Header       | Purpose                                      |
|--------------|----------------------------------------------|
| `<fcntl.h>`  | Non-blocking terminal input via `fcntl()`    |
| `<unistd.h>` | `usleep()` for game tick timing, `read()`    |
| `<ctime>`    | `time()` for high score timestamps           |

Terminal rendering uses ANSI escape codes via the team's `colors.h` 

### Wordle Minigame (wordle.cpp)

A terminal-based logic puzzle integrated into TermiCraft. The player has 5 attempts to identify a hidden word, with feedback provided via high-contrast color-coded tiles: Green (correct position), Yellow (wrong position), and Gray (not in word). The game features a live "Letters Used" keyboard tracker that updates in real-time to show the best-known status of each letter in the alphabet. This tracker is important for strategy, as it encodes global information across all previous guesses rather than just the current attempt.

The system is designed to behave similarly to the official Wordle game logic, including strict validation of input length, rejection of invalid words, and per-letter feedback computation that respects duplicate letter rules. The algorithm ensures correctness by marking already-used target letters to prevent over-counting yellow tiles.

The UI is fully terminal-rendered using ANSI escape codes from colors.h, with a structured grid layout that maintains alignment across varying word lengths. Each row is dynamically rendered based on guess history, ensuring consistent spacing and visual clarity.

The word pool is split into three difficulty tiers (WORDS_4, WORDS_5, WORDS_6), each containing a curated dictionary of valid English words. These are used both for answer selection and guess validation, ensuring that gameplay remains constrained to meaningful vocabulary rather than arbitrary strings.

How coding elements are met:

**Random events (Element 1):** The target word is selected randomly using rand() from a predefined vector of valid words based on difficulty. This ensures that every run of the minigame produces a different hidden word, with uniform probability across the word list. The randomness is deterministic only if the same seed is reused, allowing reproducibility for debugging.

**Data structures (Element 2): ** Uses std::vector<std::string> for storing word banks and std::vector<int> / std::vector<bool> structures to track letter states and match evaluation. A per-guess evaluation array stores tile states (0 = gray, 1 = yellow, 2 = green), ensuring deterministic rendering. Additionally, a 26-length alphabet state array maintains cumulative letter knowledge across guesses for the on-screen keyboard.

**Algorithmic logic: **Implements a two-pass matching algorithm identical to Wordle’s official rules:
First pass identifies correct-position (green) matches.
Second pass assigns partial matches (yellow) while respecting consumed letters.
This prevents incorrect duplication of yellow tiles when letters appear multiple times.

**Multiple files (Element 5):** The module is fully encapsulated and integrates with colors.h for rendering and menu.h for UI consistency. It exposes a single entry point runWordle(int wordLength) that allows seamless invocation from the main game loop without exposing internal state.

**Difficulty levels (Element 6):** Difficulty is directly mapped to word length:
Easy: 4-letter words (high frequency vocabulary, faster solving time)
Normal: 5-letter words (balanced difficulty and vocabulary range)
Hard: 6-letter words (lower frequency words, higher cognitive load)

This scaling affects both the solution space size and the cognitive difficulty of pattern recognition.

---

### Sudoku Minigame (`sudoku.cpp`)

An advanced logic-based minigame used for unlocking high-tier rewards. The game dynamically generates a solvable Sudoku grid using a recursive backtracking algorithm. It features a custom board-rendering engine that distinguishes between **Fixed Numbers** and **Player Moves** using color-coded ANSI output.

**How coding elements are met:**

- **Random events (Element 1):** Puzzle generation utilizes the `<random>` library’s `std::mt19937` and `std::shuffle` from `<algorithm>` to ensure every board is unique and mathematically valid.

- **Data structures (Element 2):** Board states are managed via a 2D `std::vector<std::vector<int>>`. A parallel boolean grid tracks "fixed" cells to prevent players from overwriting initial clues.

- **Multiple files (Element 5):** Encapsulated within a `SudokuGame` class, the module uses `<sstream>` to parse complex user input strings (`Row Col Value`) and leverages `<unistd.h>` for consistent frame timing across the project.

- **Difficulty levels (Element 6):** Difficulty is scaled through both grid dimensions and clue density:
    - **Easy/Normal:** 6x6 grid with 2x3 subgrids.
    - **Hard:** 9x9 grid with 3x3 subgrids.
    The number of cells removed to create the puzzle is dynamically adjusted based on the player's chosen difficulty level.

---

### Minesweeper Minigame ('minesweeper.cpp')

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

### Twentyfour Minigame ('twentyfour.cpp', 'evaluator.cpp')

A logic and arithmetic-based minigame used during equipment progression in TermiCraft. The player receives four cards and must use each value exactly once with +, -, *, / and parentheses to reach exactly 24.

**How coding elements are met:**
- **Random events (Element 1):** Puzzle selection uses rand() seeded with current time to pick a random valid 4-number puzzle from the bank each time, ensuring every session feels unique.

- **Storing data (Element 2):**

- **Dynamic memory management (Element 3):** All puzzle storage and card vectors are allocated dynamically with std::vector. Memory is automatically cleaned up when the TwentyFour object is destroyed.

- **File input (Element 4):** loadPuzzleNumbers() and parseNumbers() read and parse the twentyfourpuzzles.csv file, skipping the header and parsing the puzzle to be in an easy-to-process format.

- **Program codes in multiple files (Element 5):** Core game logic is in twentyfour.cpp / twentyfour.h, while expression parsing and evaluation are separated into evaluator.cpp / evaluator.h for clean modularity.

- **Multiple difficulty levels (Elemen 6):** Difficulty is controlled by attempts and timeLimit parameters passed to playGame():
    - **Easy:** 5 attempts, 180 seconds
    - **Medium:** 3 attempts, 90 seconds
    - **Hard:** 1 attempt, 30 seconds
