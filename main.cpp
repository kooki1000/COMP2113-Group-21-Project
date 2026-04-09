/*
 * main.cpp
 * 
 * Main entry point and game loop for TermiCraft.
 * This file ties everything together - menus, world, player, minigames, boss.
 * 
 * The stub functions below are placeholders. Team members replace them
 * with their actual implementations.
 */

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <csignal>
#include <algorithm>
#include <random>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include "types.h"
#include "colors.h"
#include "menu.h"
#include "fileio.h"

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

void initWorld(GameState& state) {
    // Allocate the 2D world grid
    state.world = new Block*[state.worldHeight];
    for (int y = 0; y < state.worldHeight; y++) {
        state.world[y] = new Block[state.worldWidth];
    }
}

void generateWorld(GameState& state) {
    // Seed the RNG with our game seed
    rng.seed(state.seed);
    uniform_int_distribution<int> dist100(0, 99);
    uniform_int_distribution<int> dist10(0, 9);
    
    for (int y = 0; y < state.worldHeight; y++) {
        for (int x = 0; x < state.worldWidth; x++) {
            Block& block = state.world[y][x];
            
            if (y < SURFACE_LEVEL - 2) {
                // Sky
                block.type = BLOCK_SKY;
            } else if (y == SURFACE_LEVEL - 2) {
                // Tree tops (10% chance)
                if (dist10(rng) == 0) {
                    block.type = BLOCK_LEAVES;
                } else {
                    block.type = BLOCK_SKY;
                }
            } else if (y == SURFACE_LEVEL - 1) {
                // Tree trunks below leaves, otherwise sky
                if (state.world[y-1][x].type == BLOCK_LEAVES) {
                    block.type = BLOCK_WOOD;
                } else {
                    block.type = BLOCK_SKY;
                }
            } else if (y == SURFACE_LEVEL) {
                // Grass surface
                block.type = BLOCK_GRASS;
            } else if (y < STONE_LEVEL) {
                // Dirt layer
                block.type = BLOCK_DIRT;
            } else if (y >= state.worldHeight - 1) {
                // Bedrock at bottom
                block.type = BLOCK_BEDROCK;
            } else {
                // Stone layer with random ores
                int oreRoll = dist100(rng);
                
                if (y > DEEP_LEVEL && oreRoll < 2) {
                    block.type = BLOCK_DIAMOND;
                } else if (y > DEEP_LEVEL - 5 && oreRoll < 5) {
                    block.type = BLOCK_GOLD;
                } else if (y > STONE_LEVEL + 3 && oreRoll < 10) {
                    block.type = BLOCK_IRON;
                } else if (oreRoll < 15) {
                    block.type = BLOCK_COAL;
                } else {
                    block.type = BLOCK_STONE;
                }
            }
            
            block.mined = false;
            block.visible = false;
        }
    }
    
    // Put dragon cave somewhere deep
    uniform_int_distribution<int> dragonXDist(WORLD_WIDTH/2 - 10, WORLD_WIDTH/2 + 10);
    int dragonX = dragonXDist(rng);
    int dragonY = state.worldHeight - 5;
    state.dragonCavePos = Position(dragonX, dragonY);
    state.world[dragonY][dragonX].type = BLOCK_DRAGON_CAVE;
    
    state.lastMessage = "World generated! Use WASD to move, SPACE to mine.";
}

void renderWorld(const GameState& state) {
    // Build the entire frame in a buffer first, then print it all at once
    // This prevents screen flicker
    stringstream frame;
    
    // HUD at the top
    // (We'll render HUD separately since it uses cout directly)
    
    // Figure out what part of the world to show
    int viewX = state.player.pos.x - state.viewportWidth / 2;
    int viewY = state.player.pos.y - state.viewportHeight / 2;
    
    // Clamp viewport to world bounds
    // Do upper bound first, then lower bound, so we don't go negative
    int maxX = max(0, state.worldWidth - state.viewportWidth);
    int maxY = max(0, state.worldHeight - state.viewportHeight);
    viewX = max(0, min(viewX, maxX));
    viewY = max(0, min(viewY, maxY));
    
    // Top border
    frame << COLOR_CYAN << " ┌";
    for (int x = 0; x < state.viewportWidth; x++) frame << "─";
    frame << "┐\n" << COLOR_RESET;
    
    // Draw the visible portion of the world
    for (int vy = 0; vy < state.viewportHeight; vy++) {
        int worldY = viewY + vy;
        frame << COLOR_CYAN << " │" << COLOR_RESET;
        
        for (int vx = 0; vx < state.viewportWidth; vx++) {
            int worldX = viewX + vx;
            
            // Is this where the player is?
            if (worldX == state.player.pos.x && worldY == state.player.pos.y) {
                frame << COLOR_PLAYER << "@" << COLOR_RESET;
                continue;
            }
            
            // Is there an enemy here?
            bool isEnemy = false;
            for (const Enemy& enemy : state.enemies) {
                if (enemy.alive && enemy.pos.x == worldX && enemy.pos.y == worldY) {
                    frame << COLOR_RED << enemy.symbol << COLOR_RESET;
                    isEnemy = true;
                    break;
                }
            }
            if (isEnemy) continue;
            
            // Just a regular block
            const Block& block = state.world[worldY][worldX];
            frame << getBlockColor(block.type) << getBlockChar(block.type) << COLOR_RESET;
        }
        
        frame << COLOR_CYAN << "│" << COLOR_RESET << "\n";
    }
    
    // Bottom border
    frame << COLOR_CYAN << " └";
    for (int x = 0; x < state.viewportWidth; x++) frame << "─";
    frame << "┘\n" << COLOR_RESET;
    
    // Status message
    if (!state.lastMessage.empty()) {
        frame << COLOR_YELLOW << " " << state.lastMessage << COLOR_RESET << "\n";
    }
    
    // Controls reminder
    frame << COLOR_DIM << " [WASD] Move  [SPACE] Mine  [C] Craft  [I] Inventory  [P] Pause  [Q] Quit" << COLOR_RESET << "\n";
    
    // Now actually draw everything
    clearScreen();
    renderHUD(state);
    cout << frame.str();
    cout.flush();
}

void updateWorldVisibility(GameState& state) {
    // TODO: Mohit - implement fog of war if you want
    // For now just make everything visible
    for (int y = 0; y < state.worldHeight; y++) {
        for (int x = 0; x < state.worldWidth; x++) {
            state.world[y][x].visible = true;
        }
    }
}

// ===== KOKI: Player stuff =====
// WAITING ON KOKI: replace these stubs with player.cpp
// Pending: confirm function names + initPlayer signature + facingX/facingY in types.h

void initPlayer(GameState& state) {
    state.player.name = "Player";
    state.player.pos = Position(state.worldWidth / 2, SURFACE_LEVEL - 1);
    state.player.health = state.settings.playerHealth;
    state.player.maxHealth = state.settings.playerHealth;
    state.player.alive = true;
    state.player.inventory = Inventory();
    state.player.equipment = Equipment();
}

void handlePlayerInput(GameState& state, char input) {
    int newX = state.player.pos.x;
    int newY = state.player.pos.y;
    
    switch (input) {
        case 'w': case 'W': newY--; break;
        case 's': case 'S': newY++; break;
        case 'a': case 'A': newX--; break;
        case 'd': case 'D': newX++; break;
        case ' ':
            // Mine the block below us
            mineBlock(state, state.player.pos.x, state.player.pos.y + 1);
            return;
        case 'c': case 'C':
            showCraftingMenu(state);
            return;
        case 'i': case 'I':
            showInventory(state);
            return;
        default:
            return;
    }
    
    // Don't walk off the map
    if (newX < 0 || newX >= state.worldWidth) return;
    if (newY < 0 || newY >= state.worldHeight) return;
    
    // Can only walk into air/sky
    BlockType targetType = state.world[newY][newX].type;
    if (targetType == BLOCK_AIR || targetType == BLOCK_SKY) {
        state.player.pos.x = newX;
        state.player.pos.y = newY;
    } else if (targetType == BLOCK_DRAGON_CAVE) {
        // Enter dragon cave - diamond armor recommended but not required
        state.dragonCaveFound = true;
        state.phase = PHASE_BOSS;
        if (state.player.equipment.armor < MATERIAL_DIAMOND) {
            state.lastMessage = "Warning: Diamond armor recommended for the Dragon!";
        }
    } else {
        // Try to mine whatever we walked into
        mineBlock(state, newX, newY);
    }
}

bool canMineBlock(const GameState& state, int x, int y) {
    if (x < 0 || x >= state.worldWidth || y < 0 || y >= state.worldHeight) return false;
    
    BlockType type = state.world[y][x].type;
    MaterialTier pickaxe = state.player.equipment.pickaxe;
    
    switch (type) {
        case BLOCK_WOOD:
        case BLOCK_LEAVES:
        case BLOCK_GRASS:
        case BLOCK_DIRT:
            return true;  // can always get these
        case BLOCK_STONE:
        case BLOCK_COAL:
            return pickaxe >= MATERIAL_WOOD;
        case BLOCK_IRON:
            return pickaxe >= MATERIAL_STONE;
        case BLOCK_GOLD:
            return pickaxe >= MATERIAL_IRON;
        case BLOCK_DIAMOND:
            return pickaxe >= MATERIAL_GOLD;
        case BLOCK_BEDROCK:
        case BLOCK_DRAGON_CAVE:
            return false;  // can never mine these
        default:
            return false;
    }
}

void mineBlock(GameState& state, int x, int y) {
    if (!canMineBlock(state, x, y)) {
        state.lastMessage = "You can't mine that with your current pickaxe!";
        return;
    }
    
    BlockType type = state.world[y][x].type;
    
    // Add to inventory and score
    switch (type) {
        case BLOCK_WOOD:
        case BLOCK_LEAVES:
            state.player.inventory.wood++;
            state.score += getBlockScore(type);
            break;
        case BLOCK_STONE:
            state.player.inventory.stone++;
            state.score += getBlockScore(type);
            break;
        case BLOCK_COAL:
            state.player.inventory.coal++;
            state.score += getBlockScore(type);
            break;
        case BLOCK_IRON:
            state.player.inventory.iron++;
            state.score += getBlockScore(type);
            // TODO: Koki - trigger minigames from the crafting menu instead
            // Don't auto-trigger here, it's annoying if you fail and have to
            // keep finding more iron just to retry
            break;
        case BLOCK_GOLD:
            state.player.inventory.gold++;
            state.score += getBlockScore(type);
            // TODO: Koki - same deal, trigger from crafting menu
            break;
        case BLOCK_DIAMOND:
            state.player.inventory.diamond++;
            state.score += getBlockScore(type);
            break;
        default:
            break;
    }
    
    // Replace block with air
    state.world[y][x].type = BLOCK_AIR;
    state.world[y][x].mined = true;
    state.oresMined++;
    
    state.lastMessage = "Mined " + string(1, getBlockChar(type)) + "!";
    
    // Random chance to spawn an enemy when mining underground
    if (y > STONE_LEVEL) {
        uniform_int_distribution<int> dist100(0, 99);
        if (dist100(rng) < state.settings.enemySpawnChance) {
            spawnEnemies(state);
            state.lastMessage = "A creature appeared!";
        }
    }
}

void updateCrafting(GameState& state) {
    // TODO: Koki - implement crafting logic
    (void)state;
}

bool canCraftTier(const GameState& state, MaterialTier tier) {
    // TODO: Koki - check if player has enough materials
    (void)state;
    (void)tier;
    return false;
}

void craftEquipment(GameState& state, MaterialTier tier, bool isPickaxe) {
    // TODO: Koki - actually craft stuff
    (void)state;
    (void)tier;
    (void)isPickaxe;
}

// ===== SOHAN: Combat stuff =====

void spawnEnemies(GameState& state) {
    Enemy bug;
    bug.name = "Cave Bug";
    
    // Spawn near the player, but clamp to world bounds
    uniform_int_distribution<int> offsetX(-2, 2);
    uniform_int_distribution<int> offsetY(-1, 1);
    
    int spawnX = state.player.pos.x + offsetX(rng);
    int spawnY = state.player.pos.y + offsetY(rng);
    
    // Keep spawn position inside the world
    spawnX = max(0, min(spawnX, state.worldWidth - 1));
    spawnY = max(0, min(spawnY, state.worldHeight - 1));
    
    bug.pos = Position(spawnX, spawnY);
    bug.health = 20 * state.settings.enemyHealthMult / 100;
    bug.maxHealth = bug.health;
    bug.damage = 5;
    bug.symbol = 'B';
    bug.alive = true;
    
    state.enemies.push_back(bug);
}

void updateEnemies(GameState& state) {
    for (Enemy& enemy : state.enemies) {
        if (!enemy.alive) continue;
        
        // Simple chase AI - move toward player
        int nextX = enemy.pos.x;
        int nextY = enemy.pos.y;
        
        if (enemy.pos.x < state.player.pos.x) nextX++;
        else if (enemy.pos.x > state.player.pos.x) nextX--;
        
        if (enemy.pos.y < state.player.pos.y) nextY++;
        else if (enemy.pos.y > state.player.pos.y) nextY--;
        
        // Make sure we don't walk into walls or other enemies
        bool canMove = true;
        
        // Check world bounds and block type
        if (nextX < 0 || nextX >= state.worldWidth || nextY < 0 || nextY >= state.worldHeight) {
            canMove = false;
        } else if (state.world[nextY][nextX].type != BLOCK_AIR && 
                   state.world[nextY][nextX].type != BLOCK_SKY) {
            canMove = false;
        }
        
        // Check for other enemies already on that tile
        if (canMove) {
            for (const Enemy& other : state.enemies) {
                if (other.alive && &other != &enemy && 
                    other.pos.x == nextX && other.pos.y == nextY) {
                    canMove = false;
                    break;
                }
            }
        }
        
        if (canMove) {
            enemy.pos.x = nextX;
            enemy.pos.y = nextY;
        }
        
        // Hurt player if touching
        if (enemy.pos.x == state.player.pos.x && enemy.pos.y == state.player.pos.y) {
            state.player.health -= enemy.damage;
            state.lastMessage = enemy.name + " hit you for " + to_string(enemy.damage) + " damage!";
            
            if (state.player.health <= 0) {
                state.player.alive = false;
                state.gameOver = true;
            }
        }
    }
    
    // Clean up dead enemies so the vector doesn't grow forever
    for (int i = state.enemies.size() - 1; i >= 0; i--) {
        if (!state.enemies[i].alive) {
            state.enemies.erase(state.enemies.begin() + i);
        }
    }
}

void playerAttack(GameState& state) {
    // TODO: Sohan - implement attacking
    (void)state;
}

bool runBossFight(GameState& state) {
    // TODO: Sohan - implement Space Invaders style boss fight
    // Make sure to use non-blocking input! See setupTerminal notes.
    
    showBossIntro();
    
    // For now just auto-win so we can test the game flow
    state.lastMessage = "You defeated the Dragon! (Boss fight coming soon)";
    state.dragonDefeated = true;
    state.victory = true;
    state.score += static_cast<int>(1000 * state.settings.scoreMultiplier);
    state.enemiesKilled++;
    
    return true;
}

// ===== ARYAN & NAN: Minigames =====

bool runWordle(int wordLength) {
    // TODO: Aryan - implement Wordle
    // The terminal will be in normal mode when this is called,
    // so you can use regular cin/cout
    
    clearScreen();
    cout << "\n\n    Wordle minigame coming soon! (Word length: " << wordLength << ")\n";
    cout << "    Press Enter to auto-win for testing...\n";
    
    string input;
    getline(cin, input);
    
    return true;  // auto-win for testing
}

bool runMinesweeper(int gridSize) {
    // TODO: Nan - implement Minesweeper
    // Same deal, terminal is in normal mode
    
    clearScreen();
    cout << "\n\n    Minesweeper minigame coming soon! (Grid: " << gridSize << "x" << gridSize << ")\n";
    cout << "    Press Enter to auto-win for testing...\n";
    
    string input;
    getline(cin, input);
    
    return true;  // auto-win for testing
}

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
    while (gameRunning && !gameState.gameOver && !gameState.victory) {
        
        // Handle minigames
        if (gameState.minigameActive) {
            bool success = false;
            
            // Put terminal back to normal mode so minigames can use cin
            restoreTerminal();
            
            switch (gameState.currentMinigame) {
                case MINIGAME_WORDLE:
                    showMinigameIntro(MINIGAME_WORDLE, gameState.pendingUpgrade);
                    success = runWordle(gameState.settings.wordleWordLength);
                    break;
                case MINIGAME_MINESWEEPER:
                    showMinigameIntro(MINIGAME_MINESWEEPER, gameState.pendingUpgrade);
                    success = runMinesweeper(gameState.settings.minesweeperSize);
                    break;
                default:
                    break;
            }
            
            // Back to raw mode for the main game
            setupTerminal();
            
            if (success) {
                gameState.player.equipment.pickaxe = gameState.pendingUpgrade;
                gameState.player.equipment.armor = gameState.pendingUpgrade;
                showUpgradeNotification(gameState.pendingUpgrade);
            } else {
                gameState.lastMessage = "Minigame failed! Try again from the crafting menu.";
            }
            
            gameState.minigameActive = false;
            gameState.currentMinigame = MINIGAME_NONE;
            gameState.pendingUpgrade = MATERIAL_NONE;
            continue;
        }
        
        // Handle boss fight
        if (gameState.phase == PHASE_BOSS) {
            restoreTerminal();              // Sohan uses ncurses - needs normal terminal mode
            bool won = runBossFight(gameState);
            setupTerminal();               // reclaim terminal after boss fight
            if (won) {
                gameState.victory = true;
            } else {
                gameState.gameOver = true;
            }
            continue;
        }
        
        // Draw the world
        renderWorld(gameState);
        
        // Get input (blocking - game is turn-based)
        char input;
        if (read(STDIN_FILENO, &input, 1) == 1) {
            switch (input) {
                case 'q': case 'Q':
                    if (showConfirmation("Quit to menu?")) {
                        gameRunning = false;
                    }
                    break;
                case 'p': case 'P':
                    // Pause menu
                    clearScreen();
                    cout << "\n\n    PAUSED\n\n";
                    cout << "    [S] Save Game\n";
                    cout << "    [R] Resume\n";
                    cout << "    [Q] Quit to Menu\n\n";
                    
                    char pauseInput;
                    if (read(STDIN_FILENO, &pauseInput, 1) == 1) {
                        if (pauseInput == 's' || pauseInput == 'S') {
                            if (saveGame(gameState)) {
                                cout << "    Game saved!\n";
                            } else {
                                cout << "    Failed to save!\n";
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
            case 1: {  // New Game
                Difficulty diff = selectDifficulty();
                
                // Get player name (need normal terminal mode for this)
                restoreTerminal();
                string name = getPlayerName("Enter your name");
                setupTerminal();
                
                initGame(gameState, diff, true);
                gameState.player.name = name;
                
                gameRunning = true;
                runGameLoop();
                
                // Game ended
                if (gameState.gameOver || gameState.victory) {
                    // NOTE: score multiplier handled per-block in Koki's player.cpp
                    // gameState.score = static_cast<int>(gameState.score * gameState.settings.scoreMultiplier);

                    showGameOver(gameState, gameState.victory);
                    
                    if (isHighScore(gameState.score)) {
                        HighScore hs;
                        hs.playerName = gameState.player.name;
                        hs.score = gameState.score;
                        hs.difficulty = gameState.difficulty;
                        hs.timestamp = time(nullptr);
                        hs.defeatedDragon = gameState.dragonDefeated;
                        addHighScore(hs);
                    }
                }
                
                cleanupGame(gameState);
                break;
            }
            
            case 2: {  // Load Game
                if (!saveFileExists()) {
                    clearScreen();
                    cout << "\n\n    No save file found!\n";
                    waitForKeypress();
                } else {
                    if (loadGame(gameState)) {
                        gameRunning = true;
                        runGameLoop();
                        
                        if (gameState.gameOver || gameState.victory) {
                            // NOTE: score multiplier handled per-block in Koki's player.cpp
                            // gameState.score = static_cast<int>(gameState.score * gameState.settings.scoreMultiplier);
                            showGameOver(gameState, gameState.victory);
                            
                            if (isHighScore(gameState.score)) {
                                HighScore hs;
                                hs.playerName = gameState.player.name;
                                hs.score = gameState.score;
                                hs.difficulty = gameState.difficulty;
                                hs.timestamp = time(nullptr);
                                hs.defeatedDragon = gameState.dragonDefeated;
                                addHighScore(hs);
                            }
                        }
                        
                        cleanupGame(gameState);
                    } else {
                        clearScreen();
                        cout << "\n\n    Failed to load save file!\n";
                        waitForKeypress();
                    }
                }
                break;
            }
            
            case 3: {  // High Scores
                vector<HighScore> scores = loadHighScores();
                showHighScores(scores);
                break;
            }
            
            case 4: {  // How to Play
                showHowToPlay();
                break;
            }
            
            case 5: {  // Quit
                exitGame = true;
                break;
            }
        }
    }
    
    restoreTerminal();
    
    cout << "\n  Thanks for playing TermiCraft!\n\n";
    
    return 0;
}
