/*
 * main.cpp
 *
 * Main entry point and game loop for TermiCraft.
 * This file ties everything together - menus, world, player, minigames, boss.
 *
 * The stub functions below are placeholders. Team members replace them
 * with their actual implementations.
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
#include "menu.h"
#include "types.h"

// ----- FORWARD DECLARATIONS -----
// These are stubs - you guys will replace them with real code

// Mohit - world generation and rendering
void initWorld(GameState &state);
void generateWorld(GameState &state);
void renderWorld(const GameState &state);
void updateWorldVisibility(GameState &state);

// Koki - player movement, mining, crafting
void initPlayer(GameState &state);
void handlePlayerInput(GameState &state, char input);
bool canMineBlock(const GameState &state, int x, int y);
void mineBlock(GameState &state, int x, int y);
void updateCrafting(GameState &state);
bool canCraftTier(const GameState &state, MaterialTier tier);
void craftEquipment(GameState &state, MaterialTier tier, bool isPickaxe);

// Sohan - enemies and boss fight
void spawnEnemies(GameState &state);
void updateEnemies(GameState &state);
void playerAttack(GameState &state);
bool runBossFight(GameState &state);

// Aryan & Nan - minigames
bool runWordle(int wordLength);
bool runMinesweeper(int gridSize);

// ----- GLOBALS -----

static GameState gameState;
static bool gameRunning = true;
static struct termios originalTermios;

// Random number generator - way better than rand()
static std::mt19937 rng;

// ----- TERMINAL SETUP -----

void setupTerminal() {
  // Save current terminal settings so we can restore later
  tcgetattr(STDIN_FILENO, &originalTermios);

  // Raw mode - get keypresses immediately without waiting for Enter
  struct termios raw = originalTermios;
  raw.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);

  // Hide the cursor
  std::cout << CURSOR_HIDE;

  clearScreen();
}

void restoreTerminal() {
  // Put everything back the way it was
  tcsetattr(STDIN_FILENO, TCSANOW, &originalTermios);
  std::cout << CURSOR_SHOW;
  std::cout << COLOR_RESET;
  clearScreen();
}

void signalHandler(int signal) {
  // Clean exit on Ctrl+C
  restoreTerminal();
  exit(signal);
}

// ----- STUB IMPLEMENTATIONS -----
// TEMPORARY - each member replaces these with their own .cpp file
// These empty stubs only exist so the project compiles before everyone's code
// is merged Mohit
void initWorld(GameState &state) {}
void generateWorld(GameState &state) {}
void renderWorld(const GameState &state) {}
void updateWorldVisibility(GameState &state) {}
// Koki
void initPlayer(GameState &state) {}
void handlePlayerInput(GameState &state, char input) {}
bool canMineBlock(const GameState &state, int x, int y) { return false; }
void mineBlock(GameState &state, int x, int y) {}
void updateCrafting(GameState &state) {}
bool canCraftTier(const GameState &state, MaterialTier tier) { return false; }
void craftEquipment(GameState &state, MaterialTier tier, bool isPickaxe) {}
// Sohan
void spawnEnemies(GameState &state) {}
void updateEnemies(GameState &state) {}
void playerAttack(GameState &state) {}
bool runBossFight(GameState &state) { return false; }
// Aryan
bool runWordle(int wordLength) { return false; }
// Nan
bool runMinesweeper(int gridSize) { return false; }

// ----- GAME INIT/CLEANUP -----

void initGame(GameState &state, Difficulty difficulty, bool isNewGame) {
  state.difficulty = difficulty;
  state.settings = getDifficultySettings(difficulty);
  state.phase = PHASE_PLAYING;
  state.gameOver = false;
  state.victory = false;
  state.score = 0;
  state.oresMined = 0;
  state.enemiesKilled = 0;
  state.dragonCaveFound = false;
  state.dragonDefeated = false;
  state.minigameActive = false;
  state.enemies.clear();

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

  state.enemies.clear();
}

// ----- GAME LOOP -----

void runGameLoop() {
  while (gameRunning && !gameState.gameOver && !gameState.victory) {

    /* minigame trigger when everyone completes their part #DONT TOUCH */

    // final fight trigger #DONT TOUCH

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
      default:
        handlePlayerInput(gameState, input);
        break;
      }
    }

    // Update enemies
    updateEnemies(gameState);
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

  while (!exitGame) {
    setupTerminal();

    HighScore topScore = getTopHighScore();
    int choice = showMainMenu(topScore);

    switch (choice) {
    case 1: { // New Game
      Difficulty diff = selectDifficulty();

      // Get player name (need normal terminal mode for this)
      restoreTerminal();
      std::string name = getPlayerName("Enter your name");
      setupTerminal();

      initGame(gameState, diff, true);
      gameState.player.name = name;

      gameRunning = true;
      runGameLoop();

      // Game ended
      if (gameState.gameOver || gameState.victory) {
        // NOTE: score multiplier handled per-block in Koki's player.cpp
        // gameState.score = static_cast<int>(gameState.score *
        // gameState.settings.scoreMultiplier);

        showGameOver(gameState, gameState.victory);

        //show high score (sohan score.cpp)
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

           //show score by sohans file
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
