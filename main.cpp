/*
 * main.cpp
 */

#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <random>
#include <sstream>
#include <termios.h>
#include <unistd.h>

#include "colors.h"
#include "fileio.h"
#include "final_fight.h"
#include "fog_of_war.h"
#include "menu.h"
#include "minesweeper.h"
#include "player.h"
#include "types.h"
#include "world_gen.h"

// ----- FORWARD DECLARATIONS -----

// Mohit - world generation and rendering
void initWorld(GameState &state);
void generateWorld(GameState &state);
void renderWorld(const GameState &state);
void updateWorldVisibility(GameState &state);

// Koki - player movement, mining, crafting
void initPlayer(GameState &state);
void handlePlayerInput(GameState &state, char input);
void resolveMiningAttempt(GameState &state, bool minigameWon);

// Sohan - boss fight
bool runBossFight(GameState &state);

// Aryan & Nan - minigames
bool runWordle(int wordLength);
bool runMinesweeper(int gridSize);
bool runSudoku(Difficulty diff);

// ----- GLOBALS -----

static GameState gameState;
static bool gameRunning = true;
static struct termios originalTermios;
static bool terminalStateCaptured = false;

// Random number generator - way better than rand()
static std::mt19937 rng;

// ----- TERMINAL SETUP -----

void setupTerminal() {
  if (!terminalStateCaptured) {
    // Save current terminal settings once so we always restore true original
    // state
    tcgetattr(STDIN_FILENO, &originalTermios);
    terminalStateCaptured = true;
  }

  // Raw mode - get keypresses immediately without waiting for Enter
  struct termios raw = originalTermios;
  raw.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);

  // Hide the cursor
  std::cout << CURSOR_HIDE;

  clearScreen();
}

void restoreTerminal() {
  if (terminalStateCaptured) {
    // Put everything back the way it was
    tcsetattr(STDIN_FILENO, TCSANOW, &originalTermios);
  }
  std::cout << CURSOR_SHOW;
  std::cout << COLOR_RESET;
  clearScreen();
}

void signalHandler(int signal) {
  // Clean exit on Ctrl+C
  restoreTerminal();
  exit(signal);
}

// ----- COMPAT BRIDGES -----
void initPlayer(GameState &state) {
  const std::string playerName =
      state.player.name.empty() ? "Player" : state.player.name;
  initPlayer(state, playerName);
}

void handlePlayerInput(GameState &state, char input) {
  handleInput(state, input);
}

// ----- TEMP STUBS FOR YET-TO-BE-FINALIZED MODULES -----

static bool runPendingMinigame(GameState &state) {
  switch (state.currentMinigame) {
  case MINIGAME_WORDLE:
    return runWordle(state.settings.wordleWordLength);
  case MINIGAME_MINESWEEPER:
    return runMinesweeper(state.settings.minesweeperSize);
  default:
    return false;
  }
}

// ----- GAME INIT/CLEANUP -----

void initGame(GameState &state, Difficulty difficulty, bool isNewGame) {
  state.difficulty = difficulty;
  state.settings = getDifficultySettings(difficulty);
  state.phase = PHASE_PLAYING;
  state.gameOver = false;
  state.victory = false;
  state.score = 0;
  state.oresMined = 0;
  state.dragonDefeated = false;
  state.minigameActive = false;

  if (isNewGame) {
    state.seed = static_cast<unsigned int>(time(nullptr));
    initWorld(state);
    generateWorld(state);
    initPlayer(state);
  }

  updateWorldVisibility(state);
}

void cleanupGame(GameState &state) {
  // Free the world array
  if (state.world != nullptr) {
    for (int y = 0; y < state.worldHeight; y++) {
      delete[] state.world[y];
    }
    delete[] state.world;
    state.world = nullptr;
  }
}

// ----- GAME LOOP -----

void runGameLoop() {
  while (gameRunning && !gameState.gameOver && !gameState.victory) {

    /* minigame trigger when everyone completes their part #DONT TOUCH */
    if (gameState.phase == PHASE_MINIGAME && gameState.minigameActive) {
      restoreTerminal();
      bool minigameWon = runPendingMinigame(gameState);
      setupTerminal();

      if (gameState.miningPending) {
        resolveMiningAttempt(gameState, minigameWon);
      } else {
        if (minigameWon) {
          confirmUpgrade(gameState);
        } else {
          gameState.minigameActive = false;
          gameState.currentMinigame = MINIGAME_NONE;
          gameState.pendingUpgrade = MATERIAL_NONE;
          gameState.lastMessage = "upgrade challenge failed.";
        }
      }

      gameState.phase = gameState.player.alive ? PHASE_PLAYING : PHASE_GAMEOVER;
      if (!gameState.player.alive) {
        gameState.gameOver = true;
      }
      continue;
    }

    // final fight trigger #DONT TOUCH
    if (gameState.phase == PHASE_BOSS) {
      restoreTerminal();
      bool bossWon = runBossFight(gameState);
      setupTerminal();

      gameState.phase = bossWon ? PHASE_VICTORY : PHASE_GAMEOVER;
      if (!bossWon) {
        gameState.gameOver = true;
      } else {
        gameState.victory = true;
      }
      continue;
    }
    if (gameState.phase == PHASE_MENU) {
      gameRunning = false;
      break;
    }

    updateWorldVisibility(gameState);

    // Draw the world
    renderWorld(gameState);

    // Get input (blocking - game is turn-based)
    char input;
    if (read(STDIN_FILENO, &input, 1) == 1) {
      switch (input) {
      case 'q':
      case 'Q':
        if (showConfirmation("Quit to menu?")) {
          gameRunning = false;
        }
        break;
      case 'p':
      case 'P':
        // Pause menu
        clearScreen();
        std::cout << "\n\n    PAUSED\n\n";
        std::cout << "    [S] Save Game\n";
        std::cout << "    [R] Resume\n";
        std::cout << "    [Q] Quit to Menu\n\n";

        char pauseInput;
        if (read(STDIN_FILENO, &pauseInput, 1) == 1) {
          if (pauseInput == 's' || pauseInput == 'S') {
            if (saveGame(gameState)) {
              std::cout << "    Game saved!\n";
            } else {
              std::cout << "    Failed to save!\n";
            }
            waitForKeypress();
          } else if (pauseInput == 'q' || pauseInput == 'Q') {
            gameRunning = false;
          }
        }
        break;
      case ' ': {
        int tx = gameState.player.pos.x + gameState.player.facingX;
        int ty = gameState.player.pos.y + gameState.player.facingY;
        if (tx >= 0 && tx < gameState.worldWidth && ty >= 0 &&
            ty < gameState.worldHeight) {
          if (gameState.world[ty][tx].type == BLOCK_DRAGON_CAVE) {
            bool enter = true;
            if (gameState.player.equipment.armor < MATERIAL_DIAMOND) {
              enter = showConfirmation(
                  "WARNING: Low armor! Enter Dragon Cave anyway?");
            } else {
              enter = showConfirmation("Enter the Dragon Cave?");
            }

            if (enter) {
              gameState.phase = PHASE_BOSS;
            }
            break;
          }
        }
        handlePlayerInput(gameState, input);
        break;
      }
      default:
        handlePlayerInput(gameState, input);
        break;
      }
    }

    updatePhysics(gameState);
  }
}

// ----- MAIN -----

int main() {
  // Set up signal handlers so Ctrl+C doesn't brick the terminal
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  // Seed random (we'll re-seed with game seed later)
  rng.seed(static_cast<unsigned int>(time(nullptr)));

  bool exitGame = false;
  setupTerminal();

  while (!exitGame) {
    HighScore topScore = getTopHighScore();
    int choice = showMainMenu(topScore);

    switch (choice) {
    case 1: { // New Game
      Difficulty diff = selectDifficulty();

      // Get player name (need normal terminal mode for this)
      restoreTerminal();
      std::string name = getPlayerName("Enter your name");
      setupTerminal();

      gameState.player.name = name;
      initGame(gameState, diff, true);

      gameRunning = true;
      runGameLoop();

      // Game ended
      if (gameState.gameOver || gameState.victory) {
        // NOTE: score multiplier handled per-block in Koki's player.cpp
        // gameState.score = static_cast<int>(gameState.score *
        // gameState.settings.scoreMultiplier);

        showGameOver(gameState, gameState.victory);

        // show high score (sohan score.cpp)
      }

      cleanupGame(gameState);
      break;
    }

    case 2: { // Load Game
      if (!saveFileExists()) {
        clearScreen();
        std::cout << "\n\n    No save file found!\n";
        waitForKeypress();
      } else {
        if (loadGame(gameState)) {
          gameRunning = true;
          runGameLoop();

          if (gameState.gameOver || gameState.victory) {
            // NOTE: score multiplier handled per-block in Koki's player.cpp
            // gameState.score = static_cast<int>(gameState.score *
            // gameState.settings.scoreMultiplier);
            showGameOver(gameState, gameState.victory);

            // show score by sohans file
          }

          cleanupGame(gameState);
        } else {
          clearScreen();
          std::cout << "\n\n    Failed to load save file!\n";
          waitForKeypress();
        }
      }
      break;
    }

    case 3: { // High Scores
      std::vector<HighScore> scores = loadHighScores();
      showHighScores(scores);
      break;
    }

    case 4: { // How to Play
      showHowToPlay();
      break;
    }

    case 5: { // Quit
      exitGame = true;
      break;
    }
    }
  }

  restoreTerminal();

  std::cout << "\n  Thanks for playing TermiCraft!\n\n";

  return 0;
}
