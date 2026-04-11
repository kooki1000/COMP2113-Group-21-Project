// =============================================================================
// player.cpp
// TermiCraft — Player & Crafting System Module Implementation
//
// Implements the player controller and crafting system for TermiCraft. Handles
// real-time keyboard input for movement (WASD), mining (SPACE), and crafting (C),
// with full collision detection against the procedurally generated world grid.
//
// The module enforces the strict progression system: Wood → Stone → Iron → Gold
// -> Diamond, where each tier upgrade after Stone requires completing a minigame
// (Wordle for Iron, Minesweeper for Gold) to simulate the "rite of passage"
// described in the game lore. Mining triggers enemy spawns based on difficulty
// settings and depth (Zombies above ground, Cave Bugs below).
//
// Physics includes simple gravity simulation causing the player to fall when
// standing over air blocks. The camera system tracks player movement with
// clamping to world boundaries.
//
// Author:       Koki
// Dependencies: player.h, colors.h, menu.h, <algorithm>, <cstdlib>, <ctime>,
//               <iostream>
// =============================================================================

#include "player.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>

#include "colors.h"
#include "menu.h"

// Initialize player at the dragon carcass spawn point
void initPlayer(GameState& state, const std::string& playerName) {
    // Reset player stats
    state.player.name = playerName;
    state.player.health = state.settings.playerHealth;
    state.player.maxHealth = state.settings.playerHealth;
    state.player.alive = true;
    state.player.facingX = 0;
    state.player.facingY = 1;  // Face down by default

    // Reset inventory and equipment
    state.player.inventory = Inventory();
    state.player.equipment = Equipment();

    // Find dragon carcass (BLOCK_DRAGON_CAVE) to spawn player
    bool spawnFound = false;
    for (int y = 0; y < state.worldHeight && !spawnFound; y++) {
        for (int x = 0; x < state.worldWidth && !spawnFound; x++) {
            if (state.world[y][x].type == BLOCK_DRAGON_CAVE) {
                // Spawn player on the carcass (or above it if it's somehow solid)
                state.player.pos = Position(x, y);

                // If there's a solid block above the carcass, find air above
                while (state.player.pos.y > 0 &&
                       isSolidBlock(state.world[state.player.pos.y][state.player.pos.x].type)) {
                    state.player.pos.y--;
                }
                spawnFound = true;
            }
        }
    }

    // Fallback: if no dragon cave found, spawn at surface center
    if (!spawnFound) {
        state.player.pos = Position(state.worldWidth / 2, SURFACE_LEVEL - 1);
    }

    // Initialize camera to center player
    updateCamera(state);

    state.lastMessage = "Welcome, " + playerName + "! The elder awaits your descent.";
}

// Check if block type is solid (impassable)
bool isSolidBlock(BlockType type) {
    switch (type) {
        case BLOCK_AIR:
        case BLOCK_SKY:
        case BLOCK_LEAVES:
            return false;
        default:
            return true;
    }
}

// Get required tool tier to mine a block
MaterialTier getRequiredTierForBlock(BlockType block) {
    switch (block) {
        case BLOCK_DIRT:
        case BLOCK_GRASS:
        case BLOCK_WOOD:
        case BLOCK_LEAVES:
            return MATERIAL_NONE;  // Can mine with hands

        case BLOCK_STONE:
        case BLOCK_COAL:
            return MATERIAL_WOOD;

        case BLOCK_IRON:
            return MATERIAL_STONE;

        case BLOCK_GOLD:
            return MATERIAL_IRON;

        case BLOCK_DIAMOND:
            return MATERIAL_GOLD;

        case BLOCK_BEDROCK:
            return MATERIAL_DIAMOND;  // Or unmineable, but lore says diamond tools needed

        case BLOCK_DRAGON_CAVE:
            return MATERIAL_DIAMOND;  // Can excavate carcass with diamond tools

        default:
            return MATERIAL_NONE;
    }
}

// Check if player can mine a specific block
bool canPlayerMine(const GameState& state, BlockType block) {
    MaterialTier required = getRequiredTierForBlock(block);
    return state.player.equipment.pickaxe >= required;
}

// Attempt to move player
bool movePlayer(GameState& state, int dx, int dy) {
    int newX = state.player.pos.x + dx;
    int newY = state.player.pos.y + dy;

    // Bounds checking
    if (newX < 0 || newX >= state.worldWidth || newY < 0 || newY >= state.worldHeight) {
        return false;
    }

    Block& targetBlock = state.world[newY][newX];

    // Collision detection
    if (isSolidBlock(targetBlock.type) && !targetBlock.mined) {
        return false;  // Can't walk through solid unmined blocks
    }

    // Update position
    state.player.pos.x = newX;
    state.player.pos.y = newY;

    // Update facing direction
    if (dx != 0 || dy != 0) {
        state.player.facingX = dx;
        state.player.facingY = dy;
    }

    // Update camera to follow
    updateCamera(state);

    return true;
}

// Update camera to center on player
void updateCamera(GameState& state) {
    // Calculate ideal camera position (top-left corner) to center player
    int idealX = state.player.pos.x - state.viewportWidth / 2;
    int idealY = state.player.pos.y - state.viewportHeight / 2;

    // Clamp to world bounds
    idealX = std::max(0, std::min(idealX, state.worldWidth - state.viewportWidth));
    idealY = std::max(0, std::min(idealY, state.worldHeight - state.viewportHeight));

    state.camera.x = idealX;
    state.camera.y = idealY;
}

// Mine the block player is facing
void mineFacingBlock(GameState& state) {
    int targetX = state.player.pos.x + state.player.facingX;
    int targetY = state.player.pos.y + state.player.facingY;

    // Bounds check
    if (targetX < 0 || targetX >= state.worldWidth || targetY < 0 || targetY >= state.worldHeight) {
        state.lastMessage = "Cannot mine here!";
        return;
    }

    Block& block = state.world[targetY][targetX];
    BlockType originalType = block.type;

    // Check if already mined/air
    if (block.mined || block.type == BLOCK_AIR || block.type == BLOCK_SKY) {
        state.lastMessage = "Nothing to mine here.";
        return;
    }

    // Check tool requirement
    if (!canPlayerMine(state, originalType)) {
        MaterialTier required = getRequiredTierForBlock(originalType);
        std::string toolName = getMaterialName(required) + " Pickaxe";
        if (required == MATERIAL_NONE) toolName = "hands";

        state.lastMessage = "Need " + toolName + " to mine this!";
        showStatusMessage(state.lastMessage, true);
        return;
    }

    // Mine the block
    block.mined = true;
    block.type = BLOCK_AIR;  // Turn to air after mining

    // Add resources based on type
    int points = 0;
    switch (originalType) {
        case BLOCK_WOOD:
            state.player.inventory.wood++;
            points = 1;
            break;
        case BLOCK_STONE:
            state.player.inventory.stone++;
            points = 2;
            break;
        case BLOCK_COAL:
            state.player.inventory.coal++;
            points = 2;
            break;
        case BLOCK_IRON:
            state.player.inventory.iron++;
            points = 3;
            break;
        case BLOCK_GOLD:
            state.player.inventory.gold++;
            points = 4;
            break;
        case BLOCK_DIAMOND:
            state.player.inventory.diamond++;
            points = 5;
            break;
        default:
            break;
    }

    // Update score with difficulty multiplier
    if (points > 0) {
        state.score += static_cast<int>(points * state.settings.scoreMultiplier);
        state.oresMined++;
        state.lastMessage = "Mined " + std::string(1, getBlockChar(originalType)) +
                            " +" + std::to_string(points) + " pts";
    }

    // Try to spawn enemy
    trySpawnEnemy(state, Position(targetX, targetY));
}

// Try to spawn enemy after mining
void trySpawnEnemy(GameState& state, Position minedPos) {
    int chance = state.settings.enemySpawnChance;  // 10-25 based on difficulty

    if ((rand() % 100) < chance) {
        Enemy e;
        // Spawn in the mined location or adjacent
        e.pos = minedPos;

        // Determine enemy type based on depth
        if (minedPos.y < SURFACE_LEVEL) {
            e.name = "Zombie";
            e.symbol = 'Z';
            e.health = 30;
            e.damage = 8;
        } else {
            e.name = "Cave Bug";
            e.symbol = 'B';
            e.health = 20;
            e.damage = 5;
        }

        // Apply difficulty multiplier to enemy health
        e.maxHealth = (e.health * state.settings.enemyHealthMult) / 100;
        e.health = e.maxHealth;
        e.alive = true;

        state.enemies.push_back(e);
        state.lastMessage = "A " + e.name + " appeared! Press SPACE to attack!";
    }
}

// Physics update (gravity)
void updatePhysics(GameState& state) {
    if (!state.player.alive) return;

    int belowY = state.player.pos.y + 1;

    // Check if we can fall
    if (belowY < state.worldHeight) {
        Block& below = state.world[belowY][state.player.pos.x];

        // If below is not solid or is mined, fall
        if (!isSolidBlock(below.type) || below.mined) {
            state.player.pos.y++;
            updateCamera(state);
            state.lastMessage = "Falling...";
        }
    }
}

// Handle single keypress
void handleInput(GameState& state, char input) {
    bool moved = false;

    switch (input) {
        case 'w':
        case 'W':
            moved = movePlayer(state, 0, -1);
            break;
        case 's':
        case 'S':
            moved = movePlayer(state, 0, 1);
            break;
        case 'a':
        case 'A':
            moved = movePlayer(state, -1, 0);
            break;
        case 'd':
        case 'D':
            moved = movePlayer(state, 1, 0);
            break;

        case ' ':  // Mine/Attack
            // Check if enemy is adjacent first (combat)
            // If not, mine facing block
            mineFacingBlock(state);
            break;

        case 'c':
        case 'C': {  // Crafting
            if (showCraftingMenu(state)) {
                // If crafting succeeded, check for minigame trigger
                if (checkCraftingProgression(state)) {
                    state.phase = PHASE_MINIGAME;
                    state.minigameActive = true;
                }
            }
            break;
        }

        case 'i':
        case 'I':  // Inventory
            showInventory(state);
            break;

        case 'p':
        case 'P':  // Pause/Save (main loop handles actual saving)
            state.phase = PHASE_MENU;
            break;

        case 'q':
        case 'Q':  // Quit to menu
            state.phase = PHASE_MENU;
            break;
    }

    if (moved) {
        state.lastMessage = "";  // Clear message on move
    }
}

// Damage player
void damagePlayer(GameState& state, int amount) {
    state.player.health -= amount;
    if (state.player.health <= 0) {
        state.player.health = 0;
        state.player.alive = false;
        state.gameOver = true;
        state.phase = PHASE_GAMEOVER;
    }
}

// Heal player
void healPlayer(GameState& state, int amount) {
    state.player.health = std::min(state.player.health + amount, state.player.maxHealth);
}

// Check if player has resources to craft target tier
bool hasResourcesForTier(const GameState& state, MaterialTier targetTier) {
    const Inventory& inv = state.player.inventory;

    switch (targetTier) {
        case MATERIAL_WOOD:
            return inv.wood >= 1;  // 1 wood for wooden tools

        case MATERIAL_STONE:
            // Need wooden pickaxe first, then 2 stone
            return state.player.equipment.pickaxe >= MATERIAL_WOOD && inv.stone >= 2;

        case MATERIAL_IRON:
            // Need stone tools, then 3 iron
            return state.player.equipment.pickaxe >= MATERIAL_STONE && inv.iron >= 3;

        case MATERIAL_GOLD:
            // Need iron tools, then 4 gold
            return state.player.equipment.pickaxe >= MATERIAL_IRON && inv.gold >= 4;

        case MATERIAL_DIAMOND:
            // Need gold tools, then 5 diamond
            return state.player.equipment.pickaxe >= MATERIAL_GOLD && inv.diamond >= 5;

        default:
            return false;
    }
}

// Consume resources for crafting
void consumeResourcesForTier(GameState& state, MaterialTier tier) {
    Inventory& inv = state.player.inventory;

    switch (tier) {
        case MATERIAL_WOOD:
            inv.wood -= 1;
            break;
        case MATERIAL_STONE:
            inv.stone -= 2;
            break;
        case MATERIAL_IRON:
            inv.iron -= 3;
            break;
        case MATERIAL_GOLD:
            inv.gold -= 4;
            break;
        case MATERIAL_DIAMOND:
            inv.diamond -= 5;
            break;
        default:
            break;
    }
}

// Check if crafting should trigger minigame (Iron, Gold, Diamond)
bool checkCraftingProgression(GameState& state) {
    MaterialTier newPick = state.player.equipment.pickaxe;

    // Only trigger for Iron (3) and above
    if (newPick > MATERIAL_STONE) {
        // Check if we haven't already triggered for this tier
        if (newPick != state.pendingUpgrade) {
            state.pendingUpgrade = newPick;

            // Alternate minigames based on tier
            if (newPick == MATERIAL_IRON) {
                state.currentMinigame = MINIGAME_WORDLE;
                state.lastMessage = "Rite of Passage: Prove your wit (Wordle)";
            } else if (newPick == MATERIAL_GOLD) {
                state.currentMinigame = MINIGAME_MINESWEEPER;
                state.lastMessage = "Rite of Passage: Navigate the depths (Minesweeper)";
            } else if (newPick == MATERIAL_DIAMOND) {
                state.currentMinigame = MINIGAME_WORDLE;  // Or random
                state.lastMessage = "Rite of Passage: The final trial";
            }
            return true;
        }
    }
    return false;
}

// Called by main.cpp after minigame victory to confirm the upgrade
void confirmUpgrade(GameState& state) {
    if (state.pendingUpgrade != MATERIAL_NONE) {
        state.minigameActive = false;
        state.lastMessage = "Upgrade to " + getMaterialName(state.pendingUpgrade) + " confirmed!";

        // Check if full diamond armor achieved
        if (state.pendingUpgrade == MATERIAL_DIAMOND &&
            state.player.equipment.armor == MATERIAL_DIAMOND) {
            state.lastMessage = "Full Diamond Armor achieved! Find the portal beneath the carcass!";
        }

        state.pendingUpgrade = MATERIAL_NONE;
    }
}

// Get color based on armor tier for player rendering
const char* getPlayerArmorColor(const GameState& state) {
    return getMaterialColor(state.player.equipment.armor);
}