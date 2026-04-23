# COMP2113-Group-21-Project

## Team Members

| Name  | Student ID | Role                              |
|-------|------------|-----------------------------------|
| Sohan | 3036636025    | Boss fight, score system          |
| Saarim| 3036520068    | Integration, menu, load/save, etc |
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
