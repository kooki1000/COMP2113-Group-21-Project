# COMP2113-Group-21-Project

## Team Members

| Name | Student ID | Role |
| :--- | :--- | :--- |
| Sohan | 3036636025 | Boss fight, score system |
| Aryan | 3036484587 | Wordle Implementation, Sudoku Implementation|
---

## Features Implemented

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

### Wordle Minigame (`wordle.cpp`)

A terminal-based logic puzzle integrated into TermiCraft. The player has 5 attempts to identify a hidden word, with feedback provided via high-contrast color-coded tiles: **Green** (correct position), **Yellow** (wrong position), and **Gray** (not in word). The game features a live "Letters Used" keyboard tracker that updates in real-time to show the best-known status of each letter in the alphabet.

**How coding elements are met:**

- **Random events (Element 1):** The game utilizes `rand()` to select a target word from three categorized dictionaries (`WORDS_4`, `WORDS_5`, or `WORDS_6`). This ensures the hidden word is different every time the minigame is triggered.

- **Data structures (Element 2):** Uses `std::vector<std::string>` to store categorized word lists and `std::vector<int>` status arrays to track the state of the board and the keyboard. The algorithm handles duplicate letters by tracking character usage in a boolean vector, ensuring yellow/green hints are technically accurate (e.g., not over-counting letters).

- **Multiple files (Element 5):** The module is designed as a standalone component that integrates with the project-wide `colors.h` for ANSI rendering and `menu.h` for screen management. It uses a clean functional interface (`runWordle`) to be called from the main game state.

- **Difficulty levels (Element 6):** Difficulty is mechanically enforced through word length. The `runWordle` function accepts a `wordLength` parameter (4, 5, or 6), which switches the game logic between "Easy," "Normal," and "Hard" modes, respectively, by referencing different pointer-based dictionaries.

### Wordle Minigame (`wordle.cpp`)

An integrated terminal-based logic puzzle where players must identify a hidden word within 5 attempts. The game features a dynamic UI with color-coded feedback: **Green** (correct position), **Yellow** (wrong position), and **Gray** (not in word), alongside a "Letters Used" keyboard tracker to help players narrow down possibilities.

**How coding elements are met:**

- **Random events (Element 1):** The target word is selected randomly from a pool of hundreds of words using `rand()` and `<cstdlib>` functions. This ensures a fresh challenge for each encounter.
  
- **Data structures (Element 2):** The game uses `std::vector<std::string>` for the word dictionaries (`WORDS_4`, `WORDS_5`, `WORDS_6`) and a `std::vector<int>` status array to track the state of all 26 letters to render the live keyboard interface.

- **Multiple files (Element 5):** The module integrates seamlessly with `colors.h` for ANSI rendering and uses `<termios.h>` to toggle **ICANON** and **ECHO** modes, allowing for validated terminal input without interfering with the main game's raw input settings.

- **Difficulty levels (Element 6):** The game scales difficulty by varying the word length:
    - **Easy:** 4-letter words.
    - **Normal:** 5-letter words.
    - **Hard:** 6-letter words.

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
