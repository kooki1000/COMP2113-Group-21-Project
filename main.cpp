/*
 * main.cpp
 *
 * Main entry point and game loop for TermiCraft.
 * This file ties everything together - menus, world, player, minigames, boss.
 *
 * The stub functions below are placeholders. Team members replace them
 * with their actual implementations.
 */

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <sstream>

#include "colors.h"
#include "menu.h"
#include "types.h"

using namespace std;

// ----- FORWARD DECLARATIONS -----
// These are stubs - you guys will replace them with real code

// Mohit - world generation and rendering
void initWorld(GameState& state);
void generateWorld(GameState& state);
void renderWorld(const GameState& state);
void updateWorldVisibility(GameState& state);

// Koki - player movement, mining, crafting
void initPlayer(GameState& state);
void handlePlayerInput(GameState& state, char input);
bool canMineBlock(const GameState& state, int x, int y);
void mineBlock(GameState& state, int x, int y);
void updateCrafting(GameState& state);
bool canCraftTier(const GameState& state, MaterialTier tier);
void craftEquipment(GameState& state, MaterialTier tier, bool isPickaxe);

// Sohan - enemies and boss fight
void spawnEnemies(GameState& state);
void updateEnemies(GameState& state);
void playerAttack(GameState& state);
bool runBossFight(GameState& state);

// Aryan & Nan - minigames
bool runWordle(int wordLength);
bool runMinesweeper(int gridSize);

// ----- GLOBALS -----

static GameState gameState;
static bool gameRunning = true;
static struct termios originalTermios;

// Random number generator - way better than rand()
static mt19937 rng;

// ----- TERMINAL SETUP -----

void setupTerminal() {
    // Save current terminal settings so we can restore later
    tcgetattr(STDIN_FILENO, &originalTermios);

    // Raw mode - get keypresses immediately without waiting for Enter
    struct termios raw = originalTermios;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    // Hide the cursor
    cout << CURSOR_HIDE;

    clearScreen();
}

void restoreTerminal() {
    // Put everything back the way it was
    tcsetattr(STDIN_FILENO, TCSANOW, &originalTermios);
    cout << CURSOR_SHOW;
    cout << COLOR_RESET;
    clearScreen();
}

void signalHandler(int signal) {
    // Clean exit on Ctrl+C
    restoreTerminal();
    exit(signal);
}

// ----- STUB IMPLEMENTATIONS -----
// Team members: replace these with your actual code!

// ===== MOHIT: World generation =====

void initWorld(GameState& state) {}
void generateWorld(GameState& state) {}
void renderWorld(const GameState& state) {}
void updateWorldVisibility(GameState& state) {}

// ===== KOKI: Player stuff =====
// WAITING ON KOKI: replace these stubs with player.cpp
// Pending: confirm function names + initPlayer signature + facingX/facingY in types.h

void initPlayer(GameState& state) {}
void handlePlayerInput(GameState& state, char input) {}
void mineBlock(GameState& state, int x, int y) {}
void updateCrafting(GameState& state) {}
bool canCraftTier(const GameState& state, MaterialTier tier) {}

void craftEquipment(GameState& state, MaterialTier tier, bool isPickaxe) {}

// ===== SOHAN: Combat stuff =====

void spawnEnemies(GameState& state) {}
void updateEnemies(GameState& state) {}
void playerAttack(GameState& state) {}
bool runBossFight(GameState& state) {}

// ===== ARYAN & NAN: Minigames =====

bool runWordle(int wordLength) {}

bool runMinesweeper(int gridSize) {}

// ----- GAME INIT/CLEANUP -----

void initGame(GameState& state, Difficulty difficulty, bool isNewGame) {
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

void cleanupGame(GameState& state) {
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

        // HighScore topScore = getTopHighScore();
        // int choice = showMainMenu(topScore);
        int choice = 1;

        switch (choice) {
            // New Game
            case 1: {  // New Game
                break;
            }

            case 2: {  // Load Game
                break;
            }

            case 3: {  // High Scores
                break;
            }

            case 4: {  // How to Play
                break;
            }

            case 5: {  // Quit
                break;
            }
        }
    }

    restoreTerminal();

    cout << "\n  Thanks for playing TermiCraft!\n\n";

    return 0;
}