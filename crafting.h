// =============================================================================
// crafting.h
// TermiCraft — Crafting & Inventory System Module Header
//
// Declares the crafting and inventory system for TermiCraft. This module
// handles all equipment progression from Wood to Diamond, including the
// strict recipe validation, resource management, and minigame triggering
// mechanics required by the game's lore.
//
// The crafting system enforces a linear progression: Wood → Stone → Iron →
// Gold → Diamond, where each tier upgrade after Stone triggers a "rite of
// passage" minigame (Wordle for Iron, Minesweeper for Gold, random for Diamond).
// Crafting recipes are defined as a constant array of CraftingRecipe structs,
// ensuring compile-time validation and easy balancing.
//
// Integration with the player module is handled through the pendingUpgrade
// flag in GameState, which signals the main loop to launch the appropriate
// minigame when a player crafts beyond the Stone tier.
//
// Author:       Koki
// Dependencies: types.h, <string>
// =============================================================================

#ifndef CRAFTING_H
#define CRAFTING_H

#include <string>

#include "types.h"

// Main entry point for the crafting menu.
//
// Inputs:
// - state (GameState&): GameState to read for inventory/equipment and to update
//   when crafting occurs or a minigame is triggered.
//
// Effects:
// - Runs the full UI loop until the player exits (Q) or a minigame is triggered.
// - Updates state.phase and state.minigameActive when a rite-of-passage begins.
void openCraftingMenu(GameState& state);

// Display the full-screen inventory UI.
//
// Inputs:
// - state (const GameState&): GameState to read inventory, equipment, and health.
//
// Effects:
// - Clears the screen, renders inventory, and waits for a keypress to return.
void showInventory(const GameState& state);

// Check whether the player can craft a specific recipe right now.
//
// Inputs:
// - state (const GameState&): GameState containing inventory and equipment tiers.
// - recipe (const CraftingRecipe&): Recipe to evaluate.
//
// Returns:
// - true if prerequisites and resource costs are satisfied; false otherwise.
bool canCraft(const GameState& state, const CraftingRecipe& recipe);

// Get a user-facing error string that explains why crafting failed.
//
// Inputs:
// - state (const GameState&): GameState with current inventory/equipment.
// - recipe (const CraftingRecipe&): Recipe to evaluate.
//
// Returns:
// - A specific failure message (missing prereq or missing resources).
std::string getCraftingError(const GameState& state, const CraftingRecipe& recipe);

// Execute crafting and apply the recipe's effects.
//
// Inputs:
// - state (GameState&): GameState to mutate (inventory, equipment, and messages).
// - recipe (const CraftingRecipe&): Recipe to craft.
//
// Returns:
// - true if a minigame was triggered by the new tier; false otherwise.
//
// Effects:
// - Deducts resources, upgrades equipment, adjusts health on armor upgrades,
//   and sets a success message.
bool performCrafting(GameState& state, const CraftingRecipe& recipe);

#endif