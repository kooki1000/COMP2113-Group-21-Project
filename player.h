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

// Initialize the player state at game start.
//
// Inputs:
// - state (GameState&): GameState to populate with a fresh player, inventory, and camera.
// - playerName (const std::string&): Display name stored on the player for UI messages.
//
// Effects:
// - Resets health, equipment, inventory, facing direction, mining flags,
//   and positions the player at the default spawn.
// - Updates the camera to follow the new position and sets a welcome message.
void initPlayer(GameState& state, const std::string& playerName);

// Process a single keypress from the main loop.
//
// Inputs:
// - state (GameState&): GameState to mutate based on input.
// - input (char): One character command (WASD, space, C, I, P, Q).
//
// Effects:
// - Moves player, initiates mining, opens crafting/inventory, or changes phase.
// - Updates facing direction and clears non-critical status messages on movement.
void handleInput(GameState& state, char input);

// Move the player by a delta in grid coordinates.
//
// Inputs:
// - state (GameState&): GameState containing world grid and player position.
// - dx (int), dy (int): Signed step for horizontal/vertical movement (typically -1, 0, 1).
//
// Returns:
// - true if the move succeeded and the position was updated.
// - false if the move was blocked by bounds or solid, unmined blocks.
//
// Effects:
// - Updates position, facing direction, and camera on success.
bool movePlayer(GameState& state, int dx, int dy);

// Start mining the block the player is facing.
//
// Inputs:
// - state (GameState&): GameState with player facing direction and world blocks.
//
// Effects:
// - Validates target, checks tool requirements, and records pending mine.
// - Triggers a minigame or resolves instantly based on random chance.
// - Updates status messages and phase when a minigame is started.
void initiateMining(GameState& state);

// Resolve a pending mining attempt after the minigame ends.
//
// Inputs:
// - state (GameState&): GameState that includes the pending mining target.
// - minigameWon (bool): true if the player succeeded the minigame.
//
// Effects:
// - On success, grants resources, updates score, and clears the block.
// - On failure, applies damage and possibly ends the game.
// - Clears mining/minigame flags either way.
void resolveMiningAttempt(GameState& state, bool minigameWon);

// Check if the player's current pickaxe tier can mine a block type.
//
// Inputs:
// - state (const GameState&): GameState with current equipment tier.
// - block (BlockType): Block type to evaluate.
//
// Returns:
// - true if the pickaxe tier meets or exceeds the required tier.
bool canPlayerMine(const GameState& state, BlockType block);

// Determine the minimum tool tier required to mine a block.
//
// Inputs:
// - block (BlockType): Block type to evaluate.
//
// Returns:
// - The minimum MaterialTier needed, or MATERIAL_NONE if hands suffice.
MaterialTier getRequiredTierForBlock(BlockType block);

// Check if a block type is solid and should block movement.
//
// Inputs:
// - type (BlockType): Block type to evaluate.
//
// Returns:
// - true for solid blocks; false for pass-through types like air/sky/leaves.
bool isSolidBlock(BlockType type);

// Update the camera to keep the player centered within the viewport.
//
// Inputs:
// - state (GameState&): GameState with player position and viewport size.
//
// Effects:
// - Clamps the camera to world bounds so the viewport stays valid.
void updateCamera(GameState& state);

// Apply damage to the player, clamping health and setting game-over flags.
//
// Inputs:
// - state (GameState&): GameState to update.
// - amount (int): Damage to subtract from health.
//
// Effects:
// - Decreases health, sets alive/gameOver/phase when health reaches zero.
void damagePlayer(GameState& state, int amount);
// Heal the player without exceeding max health.
//
// Inputs:
// - state (GameState&): GameState to update.
// - amount (int): Health to add.
//
// Effects:
// - Increases health up to maxHealth.
void healPlayer(GameState& state, int amount);

// Check whether the inventory satisfies the resource requirements for a tier.
//
// Inputs:
// - state (const GameState&): GameState containing inventory and current pickaxe tier.
// - targetTier (MaterialTier): Desired tool tier.
//
// Returns:
// - true if the player has enough resources and prerequisite tier.
bool hasResourcesForTier(const GameState& state, MaterialTier targetTier);
// Consume resources from the inventory for a crafting tier.
//
// Inputs:
// - state (GameState&): GameState containing inventory to mutate.
// - tier (MaterialTier): Crafting tier to pay for.
//
// Effects:
// - Decrements the corresponding resource counts.
void consumeResourcesForTier(GameState& state, MaterialTier tier);
// Check and trigger minigame progression for higher-tier crafting.
//
// Inputs:
// - state (GameState&): GameState containing equipment and pending upgrade info.
//
// Returns:
// - true if a new minigame was triggered; false otherwise.
//
// Effects:
// - Sets pending upgrade, starts minigame phase, and updates messages.
bool checkCraftingProgression(GameState& state);
// Confirm an upgrade after a successful minigame.
//
// Inputs:
// - state (GameState&): GameState with pendingUpgrade set.
//
// Effects:
// - Clears pending upgrade, updates status messages, and checks milestones.
void confirmUpgrade(GameState& state);

// Roll and spawn an enemy at a mined position.
//
// Inputs:
// - state (GameState&): GameState containing spawn settings and enemy list.
// - minedPos (Position): Block position that was mined.
//
// Effects:
// - Appends a new Enemy to state.enemies if the random roll succeeds.
void trySpawnEnemy(GameState& state, Position minedPos);

// Get the display color for the player's armor tier.
//
// Inputs:
// - state (const GameState&): GameState containing equipment tier.
//
// Returns:
// - ANSI color code string for rendering the player sprite.
const char* getPlayerArmorColor(const GameState& state);

#endif