#ifndef CRAFTING_H
#define CRAFTING_H

#include <string>

#include "types.h"

// Main entry point for the crafting menu
// Handles the full UI loop and returns when player presses Q or when a
// minigame is triggered (sets state.minigameActive and state.phase)
void openCraftingMenu(GameState& state);

// Detailed inventory display (called when player presses 'I')
void showInventory(const GameState& state);

// Check if player meets requirements for a recipe
bool canCraft(const GameState& state, const CraftingRecipe& recipe);

// Get specific error message for why crafting failed
std::string getCraftingError(const GameState& state, const CraftingRecipe& recipe);

// Execute crafting. Returns true if a minigame was triggered (Iron/Gold/Diamond)
bool performCrafting(GameState& state, const CraftingRecipe& recipe);

#endif