/*
 * types.h
 *
 * Shared data structures for TermiCraft - everyone includes this.
 * If you're adding new structs or enums, put them here so the whole team can use them.
 *
 * DON'T modify existing structs without telling the group chat first.
 */

#ifndef TYPES_H
#define TYPES_H

#include <ctime>
#include <string>
#include <vector>

// ----- CONSTANTS -----

// World size stuff
const int WORLD_WIDTH = 80;
const int WORLD_HEIGHT = 40;
const int SURFACE_LEVEL = 8;  // ground starts here, sky above
const int STONE_LEVEL = 12;   // stone layer begins
const int DEEP_LEVEL = 25;    // rare ores spawn below this

// Block types - used in the world grid
enum BlockType {
    BLOCK_AIR = 0,
    BLOCK_SKY = 1,
    BLOCK_GRASS = 2,
    BLOCK_DIRT = 3,
    BLOCK_STONE = 4,
    BLOCK_COAL = 5,
    BLOCK_IRON = 6,
    BLOCK_GOLD = 7,
    BLOCK_DIAMOND = 8,
    BLOCK_WOOD = 9,
    BLOCK_LEAVES = 10,
    BLOCK_BEDROCK = 11,
    BLOCK_DRAGON_CAVE = 12
};

// Tool and armor tiers - higher = better
enum MaterialTier {
    MATERIAL_NONE = 0,
    MATERIAL_WOOD = 1,
    MATERIAL_STONE = 2,
    MATERIAL_IRON = 3,
    MATERIAL_GOLD = 4,
    MATERIAL_DIAMOND = 5
};

// Difficulty settings
enum Difficulty {
    DIFF_EASY = 0,
    DIFF_NORMAL = 1,
    DIFF_HARD = 2
};

// Minigame types - Aryan does Wordle, Nan does Minesweeper
enum MinigameType {
    MINIGAME_NONE = 0,
    MINIGAME_WORDLE = 1,
    MINIGAME_MINESWEEPER = 2
};

// What phase the game is in
enum GamePhase {
    PHASE_MENU = 0,
    PHASE_PLAYING = 1,
    PHASE_MINIGAME = 2,
    PHASE_BOSS = 3,
    PHASE_GAMEOVER = 4,
    PHASE_VICTORY = 5
};

// ----- DATA STRUCTURES -----

// Simple x,y position
struct Position {
    int x;
    int y;

    Position() : x(0), y(0) {}
    Position(int _x, int _y) : x(_x), y(_y) {}
};

// Single block in the world
struct Block {
    BlockType type;
    bool mined;    // has this been dug out?
    bool visible;  // can player see it? (fog of war)

    Block() : type(BLOCK_AIR), mined(false), visible(false) {}
};

// Player's collected stuff
struct Inventory {
    int wood;
    int stone;
    int coal;
    int iron;
    int gold;
    int diamond;

    Inventory() : wood(0), stone(0), coal(0), iron(0), gold(0), diamond(0) {}

    int total() const {
        return wood + stone + coal + iron + gold + diamond;
    }
};

// What the player has equipped
struct Equipment {
    MaterialTier pickaxe;
    MaterialTier armor;

    Equipment() : pickaxe(MATERIAL_NONE), armor(MATERIAL_NONE) {}
};

// The player
struct Player {
    std::string name;
    Position pos;
    int health;
    int maxHealth;
    Inventory inventory;
    Equipment equipment;
    bool alive;
    int facingX;  // -1, 0, or 1
    int facingY;  // -1, 0, or 1 (default 1 for facing down)

    Player() : name("Player"), health(100), maxHealth(100), alive(true) {}
};

// Enemies - bugs underground, zombies on surface
struct Enemy {
    std::string name;
    Position pos;
    int health;
    int maxHealth;
    int damage;
    bool alive;
    char symbol;

    Enemy() : name("Bug"), health(20), maxHealth(20), damage(5), alive(true), symbol('B') {}
};

// Settings that change based on difficulty
struct DifficultySettings {
    std::string name;
    int playerHealth;
    int enemyHealthMult;   // percentage, 100 = normal
    int oreSpawnRate;      // percentage, 100 = normal
    int enemySpawnChance;  // % chance per mine action
    float scoreMultiplier;
    int wordleWordLength;  // for Aryan's minigame
    int minesweeperSize;   // grid size for Nan's minigame

    DifficultySettings() : name("Normal"), playerHealth(100), enemyHealthMult(100), oreSpawnRate(100), enemySpawnChance(15), scoreMultiplier(1.5f), wordleWordLength(5), minesweeperSize(8) {}
};

// High score entry
struct HighScore {
    std::string playerName;
    int score;
    Difficulty difficulty;
    std::time_t timestamp;
    bool defeatedDragon;

    HighScore() : playerName("---"), score(0), difficulty(DIFF_EASY), timestamp(0), defeatedDragon(false) {}
};

// The big one - entire game state lives here
// Used for save/load too
struct GameState {
    // Core state
    GamePhase phase;
    Difficulty difficulty;
    DifficultySettings settings;

    // World - dynamically allocated, see initWorld()
    Block** world;
    int worldWidth;
    int worldHeight;

    // Player
    Player player;

    // Enemies
    std::vector<Enemy> enemies;

    // Dragon boss stuff
    bool dragonCaveFound;
    Position dragonCavePos;
    bool dragonDefeated;

    // Score tracking
    int score;
    int oresMined;
    int enemiesKilled;

    // Minigame state
    MinigameType currentMinigame;
    bool minigameActive;
    MaterialTier pendingUpgrade;

    // Camera position (top-left corner of what we're showing)
    Position camera;
    int viewportWidth;
    int viewportHeight;

    // Game over flags
    bool gameOver;
    bool victory;
    std::string lastMessage;  // status text at bottom of screen
    unsigned int seed;        // world gen seed for reproducibility

    GameState() : phase(PHASE_MENU), difficulty(DIFF_NORMAL), world(nullptr), worldWidth(WORLD_WIDTH), worldHeight(WORLD_HEIGHT), dragonCaveFound(false), dragonDefeated(false), score(0), oresMined(0), enemiesKilled(0), currentMinigame(MINIGAME_NONE), minigameActive(false), pendingUpgrade(MATERIAL_NONE), viewportWidth(60), viewportHeight(20), gameOver(false), victory(false), seed(0) {}

    // IMPORTANT: Don't copy GameState by value!
    // The world pointer will get double-freed and crash everything.
    // Always pass by reference: void doStuff(GameState& state)
    GameState(const GameState&) = delete;
    GameState& operator=(const GameState&) = delete;
};

// ----- HELPER FUNCTIONS -----

// Get the character to display for each block type
inline char getBlockChar(BlockType type) {
    switch (type) {
        case BLOCK_AIR:
            return ' ';
        case BLOCK_SKY:
            return ' ';
        case BLOCK_GRASS:
            return '"';
        case BLOCK_DIRT:
            return '.';
        case BLOCK_STONE:
            return '#';
        case BLOCK_COAL:
            return 'C';
        case BLOCK_IRON:
            return 'I';
        case BLOCK_GOLD:
            return 'G';
        case BLOCK_DIAMOND:
            return 'D';
        case BLOCK_WOOD:
            return 'T';
        case BLOCK_LEAVES:
            return '*';
        case BLOCK_BEDROCK:
            return 'X';
        case BLOCK_DRAGON_CAVE:
            return '!';
        default:
            return '?';
    }
}

// Get readable name for material tier
inline std::string getMaterialName(MaterialTier tier) {
    switch (tier) {
        case MATERIAL_NONE:
            return "None";
        case MATERIAL_WOOD:
            return "Wood";
        case MATERIAL_STONE:
            return "Stone";
        case MATERIAL_IRON:
            return "Iron";
        case MATERIAL_GOLD:
            return "Gold";
        case MATERIAL_DIAMOND:
            return "Diamond";
        default:
            return "Unknown";
    }
}

// Points you get for mining each block type
inline int getBlockScore(BlockType type) {
    switch (type) {
        case BLOCK_WOOD:
            return 1;
        case BLOCK_STONE:
            return 2;
        case BLOCK_COAL:
            return 2;
        case BLOCK_IRON:
            return 3;
        case BLOCK_GOLD:
            return 4;
        case BLOCK_DIAMOND:
            return 5;
        default:
            return 0;
    }
}

// Get all the settings for a difficulty level
inline DifficultySettings getDifficultySettings(Difficulty diff) {
    DifficultySettings s;
    switch (diff) {
        case DIFF_EASY:
            s.name = "Easy";
            s.playerHealth = 150;
            s.enemyHealthMult = 75;
            s.oreSpawnRate = 130;
            s.enemySpawnChance = 10;
            s.scoreMultiplier = 1.0f;
            s.wordleWordLength = 4;
            s.minesweeperSize = 6;
            break;
        case DIFF_NORMAL:
            s.name = "Normal";
            s.playerHealth = 100;
            s.enemyHealthMult = 100;
            s.oreSpawnRate = 100;
            s.enemySpawnChance = 15;
            s.scoreMultiplier = 1.5f;
            s.wordleWordLength = 5;
            s.minesweeperSize = 8;
            break;
        case DIFF_HARD:
            s.name = "Hard";
            s.playerHealth = 75;
            s.enemyHealthMult = 150;
            s.oreSpawnRate = 70;
            s.enemySpawnChance = 25;
            s.scoreMultiplier = 2.0f;
            s.wordleWordLength = 6;
            s.minesweeperSize = 10;
            break;
    }
    return s;
}

#endif
