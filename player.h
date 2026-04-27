// =============================================================================
// player.h
// TermiCraft — Player & Crafting System Module Header
//
// Declares all functions, constants, and interfaces for the player entity
// and crafting progression system. This module manages player state including
// position, health, inventory, and equipped tools/armor across the game world.
//
// Key responsibilities:
// - Player initialization and spawn point location (dragon carcass)
// - Movement and collision detection against world blocks
// - Mining mechanics with tier-based tool requirements
// - Crafting resource validation and consumption
// - Minigame trigger logic for equipment progression (Iron/Gold/Diamond)
//
// Author:       Koki
// Dependencies: types.h, string
// =============================================================================

#ifndef PLAYER_H
#define PLAYER_H

#include <string>

#include "types.h"

// Initialize player at game start (spawns at dragon carcass)
void initPlayer(GameState& state, const std::string& playerName);

// Initialize ore minigame assignments (called once per new game)
void initializeOreMinigames(GameState& state);

// Main entry point: processes a single keypress from main loop
void handleInput(GameState& state, char input);

// Physics update (called every tick/frame, independent of input)
void updatePhysics(GameState& state);

// Attempt to move player by (dx, dy). Returns true if move succeeded.
bool movePlayer(GameState& state, int dx, int dy);

// Mine the block the player is facing (using facingX/facingY)
void initiateMining(GameState& state);

// Check if current tool can mine a specific block type
bool canPlayerMine(const GameState& state, BlockType block);

// Calculate required material tier to mine a block
MaterialTier getRequiredTierForBlock(BlockType block);

// Check if a block type is solid (collision)
bool isSolidBlock(BlockType type);

// Adjust camera position to center on player (clamped to world bounds)
void updateCamera(GameState& state);

// Damage/healing handlers
void damagePlayer(GameState& state, int amount);
void healPlayer(GameState& state, int amount);

// Progression helpers (called by crafting system)
bool hasResourcesForTier(const GameState& state, MaterialTier targetTier);
void consumeResourcesForTier(GameState& state, MaterialTier tier);

// Check if player just crafted something that triggers a minigame
// Returns true if minigame should start, sets state.currentMinigame appropriately
bool checkCraftingProgression(GameState& state);

// Confirm upgrade after minigame victory (called by main.cpp)
void confirmUpgrade(GameState& state);

// Resolve mining attempt after minigame completes (or immediate mining)
void resolveMiningAttempt(GameState& state, MinigameResult result);

// Enemy spawn roll (called after successful mine)
void trySpawnEnemy(GameState& state, Position minedPos);

// Get color code for player based on armor tier
const char* getPlayerArmorColor(const GameState& state);

#endif