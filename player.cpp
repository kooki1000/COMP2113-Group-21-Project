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
// described in the game lore.
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
#include <random>

#include "colors.h"
#include "crafting.h"
#include "menu.h"
#include "score.h"

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

    // Reset mining state
    state.miningPending = false;
    // Removed: state.minigameDamage = state.settings.minigameDamage;

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

    state.lastMessage = "Welcome, " + playerName + "! Mine resources to survive.";
}

// Initialize ore minigame assignments (Stone, Iron, Gold, Diamond)
void initializeOreMinigames(GameState& state) {
    if (state.minigameSlotsInitialized) return;
    
    // Array of 4 minigame types
    MinigameType games[] = {MINIGAME_WORDLE, MINIGAME_MINESWEEPER, MINIGAME_SUDOKU, MINIGAME_PLACEHOLDER_4TH};
    
    // Shuffle using rng
    std::shuffle(games, games + 4, rng);
    
    // Assign to ores: Stone[0], Iron[1], Gold[2], Diamond[3]
    for (int i = 0; i < 4; i++) {
        state.oreMinigameSlots[i] = games[i];
        state.oreMinigameTriggered[i] = false;
    }
    
    state.minigameSlotsInitialized = true;
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
            return MATERIAL_DIAMOND;

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
    
    // Flight height limit (cannot fly above MAX_FLIGHT_HEIGHT)
    if (newY < MAX_FLIGHT_HEIGHT) {
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

// Initiate mining attempt - validates target then triggers minigame or mines immediately
void initiateMining(GameState& state) {
    // Calculate target position based on facing direction
    int targetX = state.player.pos.x + state.player.facingX;
    int targetY = state.player.pos.y + state.player.facingY;

    // Bounds check
    if (targetX < 0 || targetX >= state.worldWidth || targetY < 0 || targetY >= state.worldHeight) {
        state.lastMessage = "Cannot mine here!";
        return;
    }

    Block& block = state.world[targetY][targetX];
    BlockType targetType = block.type;

    // Check if already mined/air/sky
    if (block.mined || block.type == BLOCK_AIR || block.type == BLOCK_SKY) {
        state.lastMessage = "Nothing to mine here.";
        return;
    }

    // Check tool requirement
    if (!canPlayerMine(state, targetType)) {
        MaterialTier required = getRequiredTierForBlock(targetType);
        std::string toolName = getMaterialName(required) + " Pickaxe";
        if (required == MATERIAL_NONE) toolName = "hands";

        state.lastMessage = "Need " + toolName + " to mine this!";
        showStatusMessage(state.lastMessage, true);
        return;
    }

    // Store mining attempt details
    state.pendingMinePos = Position(targetX, targetY);
    state.pendingMineType = targetType;
    state.miningPending = true;
    
    // Handle different block types
    switch (targetType) {
        case BLOCK_WOOD:
        case BLOCK_LEAVES:
            // Wood: increment counter and mine immediately (no minigame)
            state.woodMinedCount++;
            resolveMiningAttempt(state, MINIGAME_WIN);
            break;
            
        case BLOCK_STONE:
        case BLOCK_IRON:
        case BLOCK_GOLD:
        case BLOCK_DIAMOND: {
            // Map ore type to index: Stone=0, Iron=1, Gold=2, Diamond=3
            int oreIndex;
            if (targetType == BLOCK_STONE) oreIndex = 0;
            else if (targetType == BLOCK_IRON) oreIndex = 1;
            else if (targetType == BLOCK_GOLD) oreIndex = 2;
            else oreIndex = 3; // DIAMOND
            
            // Increment respective counter immediately (tracks even if escape/fail)
            switch(targetType) {
                case BLOCK_STONE: state.stoneMinedCount++; break;
                case BLOCK_IRON: state.ironMinedCount++; break;
                case BLOCK_GOLD: state.goldMinedCount++; break;
                case BLOCK_DIAMOND: state.diamondMinedCount++; break;
                default: break;
            }
            
            // Check if this ore's minigame has been triggered before
            if (!state.oreMinigameTriggered[oreIndex]) {
                // First time mining this ore - trigger minigame
                state.oreMinigameTriggered[oreIndex] = true;
                state.minigameActive = true;
                state.currentMinigame = state.oreMinigameSlots[oreIndex];
                state.phase = PHASE_MINIGAME;
                
                // Don't resolve yet - wait for minigame result
                std::string gameName;
                switch(state.currentMinigame) {
                    case MINIGAME_WORDLE: gameName = "Wordle"; break;
                    case MINIGAME_MINESWEEPER: gameName = "Minesweeper"; break;
                    case MINIGAME_SUDOKU: gameName = "Sudoku"; break;
                    case MINIGAME_PLACEHOLDER_4TH: gameName = "Challenge"; break;
                    default: gameName = "Unknown"; break;
                }
                state.lastMessage = "Mining challenge: " + gameName + "!";
            } else {
                // Already triggered before, mine immediately
                resolveMiningAttempt(state, MINIGAME_WIN);
            }
            break;
        }
            
        default:
            // Other blocks (dirt, grass, coal, etc): immediate mining
            resolveMiningAttempt(state, MINIGAME_WIN);
            break;
    }
}

// Resolve mining attempt after minigame completes
void resolveMiningAttempt(GameState& state, MinigameResult result) {
    if (!state.miningPending) {
        return;
    }

    switch (result) {
        case MINIGAME_WIN: {
            // Success - grant resources
            BlockType minedType = state.pendingMineType;
            int points = 0;

            switch (minedType) {
                case BLOCK_WOOD:
                case BLOCK_LEAVES:
                    points = 1;
                    break;
                case BLOCK_STONE:
                    points = 2;
                    break;
                case BLOCK_COAL:
                    points = 2;
                    break;
                case BLOCK_IRON:
                    points = 3;
                    break;
                case BLOCK_GOLD:
                    points = 4;
                    break;
                case BLOCK_DIAMOND:
                    points = 5;
                    break;
                default:
                    break;
            }

            // Mark block as mined
            int tx = state.pendingMinePos.x;
            int ty = state.pendingMinePos.y;
            state.world[ty][tx].mined = true;
            state.world[ty][tx].type = BLOCK_AIR;

            // Update score
            if (points > 0) {
                addScore(state, points);
                state.oresMined++;
            }

            state.lastMessage = "Success! Mined " + std::string(1, getBlockChar(minedType)) +
                                " (+" + std::to_string(points) + " pts)";
            break;
        }
        
        case MINIGAME_LOSE: {
            // Failure - take normal damage
            int damage = state.settings.minigameDamage;
            damagePlayer(state, damage);

            if (state.player.alive) {
                state.lastMessage = "Failed! Took " + std::to_string(damage) + " damage! Try again.";
            } else {
                state.lastMessage = "Mining accident... you perished!";
            }
            break;
        }
        
        case MINIGAME_ESCAPE: {
            // Escape - take double damage
            int damage = state.settings.minigameDamage * 2;
            damagePlayer(state, damage);

            if (state.player.alive) {
                state.lastMessage = "You fled! Took " + std::to_string(damage) + " damage!";
            } else {
                state.lastMessage = "Fled but succumbed to injuries...";
            }
            break;
        }
    }

    // Reset mining state
    state.miningPending = false;
    state.minigameActive = false;
    state.currentMinigame = MINIGAME_NONE;
}

// Physics update (gravity removed - player can fly)
void updatePhysics(GameState& state) {
    // No gravity - player can fly freely within bounds
    if (!state.player.alive) return;
    // Empty - no automatic physics updates needed for flying player
}

// Handle single keypress
void handleInput(GameState& state, char input) {
    bool moved = false;

    switch (input) {
        case 'w':
        case 'W':
            moved = movePlayer(state, 0, -1);  // Up
            break;
        case 's':
        case 'S':
            moved = movePlayer(state, 0, 1);   // Down
            break;
        case 'a':
        case 'A':
            moved = movePlayer(state, -1, 0);  // Left
            break;
        case 'd':
        case 'D':
            moved = movePlayer(state, 1, 0);   // Right
            break;

        case ' ':  // Mine - faces direction and initiates mining
            initiateMining(state);
            break;

        case 'c':
        case 'C': {  // Crafting
            openCraftingMenu(state);
            break;
        }

        case 'i':
        case 'I':  // Inventory
            showInventory(state);
            break;

        case 'p':
        case 'P':  // Pause/Save
            state.phase = PHASE_MENU;
            break;

        case 'q':
        case 'Q':  // Quit to menu
            state.phase = PHASE_MENU;
            break;
    }

    if (moved) {
        // Clear message on move unless it was important
        if (state.lastMessage.find("Failed") == std::string::npos &&
            state.lastMessage.find("Success") == std::string::npos &&
            state.lastMessage.find("fled") == std::string::npos) {
            state.lastMessage = "";
        }
    }
}

// Damage player (now used for minigame failure and escape)
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

            // For crafting upgrades, use random selection
            int r = rand() % MINIGAME_COUNT;
            if (r == 0) {
                state.currentMinigame = MINIGAME_WORDLE;
            } else if (r == 1) {
                state.currentMinigame = MINIGAME_MINESWEEPER;
            } else if (r == 2) {
                state.currentMinigame = MINIGAME_SUDOKU;
            } else {
                state.currentMinigame = MINIGAME_PLACEHOLDER_4TH;
            }

            std::string tierName = getMaterialName(newPick);
            state.lastMessage = "Rite of Passage: Craft " + tierName + " tools!";
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
