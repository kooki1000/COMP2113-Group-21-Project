// =============================================================================
// crafting.cpp
// TermiCraft — Crafting & Inventory System Module Implementation
//
// Implements the crafting and inventory system for TermiCraft. This module
// provides a full-screen terminal UI for the crafting bench and detailed
// inventory display, with real-time validation of resource requirements and
// equipment prerequisites.
//
// The implementation features:
// - A scrolling menu of 10 recipes (5 tiers × 2 equipment types)
// - Color-coded availability (green=craftable, yellow=visible, red=locked)
// - Sidebar showing current inventory and equipment status
// - Detailed cost breakdown and error messages for failed crafts
// - Health bonuses when upgrading armor (+5 to +25 HP per tier)
//
// The crafting loop uses single-character input (W/S/ENTER/Q) for navigation
// and integrates with the existing termios-based getch() function from menu.h.
// When a recipe triggers a minigame (Iron, Gold, or Diamond tiers), the function
// returns immediately, allowing the main loop to transition to PHASE_MINIGAME.
//
// Compilation requires linking with the existing colors.h and menu.h modules.
// No external libraries beyond the standard C++ library are needed.
//
// Author:       Koki
// Dependencies: crafting.h, colors.h, menu.h, player.h, <iostream>, <iomanip>,
//               <algorithm>, <unistd.h> for usleep()
// =============================================================================

#include "crafting.h"

#include <unistd.h>

#include <algorithm>
#include <iomanip>
#include <iostream>

#include "colors.h"
#include "menu.h"    // For getch() and clearScreen()
#include "player.h"  // For checkCraftingProgression() and healPlayer()

// Recipe definitions matching the progression chart
const CraftingRecipe RECIPES[] = {
    // Tier 1: Wood (1 wood each, no prereqs)
    {MATERIAL_WOOD, false, 1, 0, 0, 0, 0, MATERIAL_NONE, MATERIAL_NONE,
     "Wooden Pickaxe", "Basic tool for mining stone"},
    {MATERIAL_WOOD, true, 1, 0, 0, 0, 0, MATERIAL_NONE, MATERIAL_NONE,
     "Wooden Armor", "Crude protection (+5 Max HP)"},

    // Tier 2: Stone (2 stone, requires Wood tools)
    {MATERIAL_STONE, false, 0, 2, 0, 0, 0, MATERIAL_WOOD, MATERIAL_WOOD,
     "Stone Pickaxe", "Mines iron ore efficiently"},
    {MATERIAL_STONE, true, 0, 2, 0, 0, 0, MATERIAL_WOOD, MATERIAL_WOOD,
     "Stone Armor", "Solid protection (+10 Max HP)"},

    // Tier 3: Iron (3 iron, requires Stone tools, triggers Wordle)
    {MATERIAL_IRON, false, 0, 0, 3, 0, 0, MATERIAL_STONE, MATERIAL_STONE,
     "Iron Pickaxe", "Mines gold ore. Rite of Passage required."},
    {MATERIAL_IRON, true, 0, 0, 3, 0, 0, MATERIAL_STONE, MATERIAL_STONE,
     "Iron Armor", "Heavy plating (+20 Max HP)"},

    // Tier 4: Gold (4 gold, requires Iron tools, triggers Minesweeper)
    {MATERIAL_GOLD, false, 0, 0, 0, 4, 0, MATERIAL_IRON, MATERIAL_IRON,
     "Gold Pickaxe", "Mines diamond. Another trial awaits."},
    {MATERIAL_GOLD, true, 0, 0, 0, 4, 0, MATERIAL_IRON, MATERIAL_IRON,
     "Gold Armor", "Gilded defense (+25 Max HP)"},

    // Tier 5: Diamond (5 diamond, requires Gold tools, triggers final trial)
    {MATERIAL_DIAMOND, false, 0, 0, 0, 0, 5, MATERIAL_GOLD, MATERIAL_GOLD,
     "Diamond Pickaxe", "Ultimate mining power"},
    {MATERIAL_DIAMOND, true, 0, 0, 0, 0, 5, MATERIAL_GOLD, MATERIAL_GOLD,
     "Diamond Armor", "Legendary protection (+40 Max HP)"}};

const int NUM_RECIPES = sizeof(RECIPES) / sizeof(RECIPES[0]);
const int INNER_WIDTH = 78;  // Width between the ║ and ║ borders

// Helper: Strip ANSI escape codes to calculate visible width
std::string stripAnsi(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size();) {
        if (text[i] == '\033' && i + 1 < text.size() && text[i + 1] == '[') {
            i += 2;
            while (i < text.size() && !(text[i] >= 0x40 && text[i] <= 0x7E)) {
                i++;
            }
            if (i < text.size()) i++;  // Skip the final letter
        } else {
            out.push_back(text[i]);
            i++;
        }
    }
    return out;
}

// Helper: Calculate display width (visible characters only)
int displayWidth(const std::string& text) {
    return stripAnsi(text).length();
}

// Helper: Check if recipe is visible (prereqs met) vs locked
bool isRecipeVisible(const GameState& state, const CraftingRecipe& recipe) {
    return state.player.equipment.pickaxe >= recipe.requiredPickaxe &&
           state.player.equipment.armor >= recipe.requiredArmor;
}

// Check if player can craft right now
bool canCraft(const GameState& state, const CraftingRecipe& recipe) {
    if (!isRecipeVisible(state, recipe)) return false;

    const Inventory& inv = state.player.inventory;
    return inv.wood >= recipe.woodCost &&
           inv.stone >= recipe.stoneCost &&
           inv.iron >= recipe.ironCost &&
           inv.gold >= recipe.goldCost &&
           inv.diamond >= recipe.diamondCost;
}

// Get error message for display
std::string getCraftingError(const GameState& state, const CraftingRecipe& recipe) {
    if (!isRecipeVisible(state, recipe)) {
        if (recipe.tier == MATERIAL_WOOD) return "Gather wood first";
        return "Requires " + getMaterialName(recipe.requiredPickaxe) + " tools";
    }

    const Inventory& inv = state.player.inventory;
    if (inv.wood < recipe.woodCost) return "Need " + std::to_string(recipe.woodCost - inv.wood) + " more wood";
    if (inv.stone < recipe.stoneCost) return "Need " + std::to_string(recipe.stoneCost - inv.stone) + " more stone";
    if (inv.iron < recipe.ironCost) return "Need " + std::to_string(recipe.ironCost - inv.iron) + " more iron";
    if (inv.gold < recipe.goldCost) return "Need " + std::to_string(recipe.goldCost - inv.gold) + " more gold";
    if (inv.diamond < recipe.diamondCost) return "Need " + std::to_string(recipe.diamondCost - inv.diamond) + " more diamond";

    return "Unknown error";
}

// Execute crafting
bool performCrafting(GameState& state, const CraftingRecipe& recipe) {
    if (!canCraft(state, recipe)) return false;

    // Consume resources
    state.player.inventory.wood -= recipe.woodCost;
    state.player.inventory.stone -= recipe.stoneCost;
    state.player.inventory.iron -= recipe.ironCost;
    state.player.inventory.gold -= recipe.goldCost;
    state.player.inventory.diamond -= recipe.diamondCost;

    // Apply upgrade
    if (recipe.isArmor) {
        state.player.equipment.armor = recipe.tier;
        // Heal bonus for new armor
        state.player.maxHealth += (recipe.tier * 5);  // +5, +10, +15, +20, +25 incremental
        healPlayer(state, recipe.tier * 5);           // Heal the bonus amount
    } else {
        state.player.equipment.pickaxe = recipe.tier;
    }

    state.lastMessage = std::string("Crafted ") + recipe.displayName + "!";

    // Check for minigame trigger (handled by player.cpp logic)
    if (checkCraftingProgression(state)) {
        return true;  // Minigame was triggered
    }
    return false;
}

// Render the crafting interface
void renderCraftingUI(const GameState& state, int selectedIndex) {
    clearScreen();

    std::cout << COLOR_BOLD_CYAN;
    std::cout << "╔══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  " << COLOR_BOLD_WHITE << "CRAFTING BENCH" << COLOR_BOLD_CYAN << "                              "
              << COLOR_BOLD_WHITE << "INVENTORY" << COLOR_BOLD_CYAN
              << "                       ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════════════════════╣\n";
    std::cout << COLOR_RESET;

    const int NAME_WIDTH = 20;

    // Print recipe list with inventory on the right
    for (int i = 0; i < NUM_RECIPES; i++) {
        const CraftingRecipe& r = RECIPES[i];
        bool isSelected = (i == selectedIndex);
        bool visible = isRecipeVisible(state, r);
        bool craftable = canCraft(state, r);

        // Start line: ║ + space
        std::cout << COLOR_BOLD_CYAN << "║ " << COLOR_RESET;

        // Selection cursor (2 chars: symbol + space, or 2 spaces)
        if (isSelected) {
            std::cout << COLOR_BOLD_YELLOW << "▶ " << COLOR_RESET;
        } else {
            std::cout << "  ";
        }

        // Recipe name with appropriate color - ALWAYS width NAME_WIDTH for alignment
        if (!visible) {
            std::cout << COLOR_DIM << std::left << std::setw(NAME_WIDTH) << "[Locked]" << COLOR_RESET;
        } else if (craftable) {
            std::cout << COLOR_GREEN << std::left << std::setw(NAME_WIDTH) << r.displayName << COLOR_RESET;
        } else {
            std::cout << COLOR_YELLOW << std::left << std::setw(NAME_WIDTH) << r.displayName << COLOR_RESET;
        }

        // Gap
        std::cout << COLOR_BOLD_CYAN << "  " << COLOR_RESET;

        // Inventory column content
        std::string invContent;
        switch (i) {
            case 0:
                invContent = std::string("Wood:    ") + COLOR_WOOD + std::to_string(state.player.inventory.wood) + COLOR_RESET;
                break;
            case 1:
                invContent = std::string("Stone:   ") + COLOR_STONE + std::to_string(state.player.inventory.stone) + COLOR_RESET;
                break;
            case 2:
                invContent = std::string("Iron:    ") + COLOR_IRON + std::to_string(state.player.inventory.iron) + COLOR_RESET;
                break;
            case 3:
                invContent = std::string("Gold:    ") + COLOR_GOLD_ORE + std::to_string(state.player.inventory.gold) + COLOR_RESET;
                break;
            case 4:
                invContent = std::string("Diamond: ") + COLOR_DIAMOND + std::to_string(state.player.inventory.diamond) + COLOR_RESET;
                break;
            case 6:
                invContent = std::string("Equipped Pickaxe: ") + getMaterialColor(state.player.equipment.pickaxe) + getMaterialName(state.player.equipment.pickaxe) + COLOR_RESET;
                break;
            case 7:
                invContent = std::string("Equipped Armor:   ") + getMaterialColor(state.player.equipment.armor) + getMaterialName(state.player.equipment.armor) + COLOR_RESET;
                break;
            case 8:
                invContent = std::string("Health:   ") + COLOR_HEALTH + std::to_string(state.player.health) + "/" + std::to_string(state.player.maxHealth) + COLOR_RESET;
                break;
            default:
                invContent = "                ";
        }

        std::cout << invContent;

        // Calculate padding to align right border
        // Used: 1 (space after ║) + 2 (cursor) + 20 (name) + 2 (gap) + invContent width
        int used = 1 + 2 + NAME_WIDTH + 2 + displayWidth(invContent);
        int padding = INNER_WIDTH - used;
        if (padding < 0) padding = 0;

        std::cout << COLOR_BOLD_CYAN << std::string(padding, ' ') << "║\n"
                  << COLOR_RESET;
    }

    // Details box separator
    std::cout << COLOR_BOLD_CYAN;
    std::cout << "╠══════════════════════════════════════════════════════════════════════════════╣\n";
    std::cout << COLOR_RESET;

    const CraftingRecipe& selected = RECIPES[selectedIndex];
    bool visible = isRecipeVisible(state, selected);
    bool craftable = canCraft(state, selected);

    // Helper lambda to print a detail line with proper alignment
    auto printDetailLine = [&](const std::string& label, const std::string& content) {
        std::cout << COLOR_BOLD_CYAN << "║ " << COLOR_RESET;  // 1 char inner used (space)
        std::cout << label;
        std::cout << content;

        int used = 1 + displayWidth(label) + displayWidth(content);
        int padding = INNER_WIDTH - used;
        if (padding < 0) padding = 0;

        std::cout << COLOR_BOLD_CYAN << std::string(padding, ' ') << "║\n"
                  << COLOR_RESET;
    };

    // Selected line
    std::string selectedName = visible ? selected.displayName : "??? (Complete previous tier)";
    std::string selectedColor = visible ? getMaterialColor(selected.tier) : COLOR_DIM;
    printDetailLine("Selected: ", selectedColor + selectedName + COLOR_RESET);

    // Cost line
    std::string costStr;
    if (!visible) {
        costStr = std::string(COLOR_DIM) + "Unknown" + COLOR_RESET;
    } else {
        bool first = true;
        auto addCost = [&](int cost, const char* name, const char* color) {
            if (cost > 0) {
                if (!first) costStr += ", ";
                costStr += std::string(color) + std::to_string(cost) + " " + name + COLOR_RESET;
                first = false;
            }
        };

        addCost(selected.woodCost, "Wood", COLOR_WOOD);
        addCost(selected.stoneCost, "Stone", COLOR_STONE);
        addCost(selected.ironCost, "Iron", COLOR_IRON);
        addCost(selected.goldCost, "Gold", COLOR_GOLD_ORE);
        addCost(selected.diamondCost, "Diamond", COLOR_DIAMOND);

        if (costStr.empty()) costStr = std::string(COLOR_DIM) + "None" + COLOR_RESET;
    }

    printDetailLine("Cost: ", costStr);

    // Description line
    std::string descStr = visible ? selected.description : "Complete previous tier to unlock";
    printDetailLine("Description: ", std::string(COLOR_DIM) + descStr + COLOR_RESET);

    // Status line
    std::string statusStr;
    std::string statusColor;
    if (craftable) {
        statusStr = "✓ Ready to craft!";
        statusColor = COLOR_GREEN;
    } else if (!visible) {
        statusStr = "🔒 Locked - craft previous tier first";
        statusColor = COLOR_DIM;
    } else {
        statusStr = "✗ " + getCraftingError(state, selected);
        statusColor = COLOR_WARNING;
    }
    printDetailLine("Status: ", statusColor + statusStr + COLOR_RESET);

    // Footer
    std::cout << COLOR_BOLD_CYAN;
    std::cout << "╠══════════════════════════════════════════════════════════════════════════════╣\n";

    std::string footerText = "[W/S] Navigate  [ENTER] Craft  [Q] Back";
    std::cout << "║ " << COLOR_WHITE << footerText << COLOR_BOLD_CYAN;

    int footerUsed = 1 + displayWidth(footerText);  // 1 for space after ║
    int footerPad = INNER_WIDTH - footerUsed;
    if (footerPad < 0) footerPad = 0;

    std::cout << std::string(footerPad, ' ') << "║\n";

    std::cout << "╚══════════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << COLOR_RESET;
}

// Main crafting menu loop
void openCraftingMenu(GameState& state) {
    int selected = 0;

    // Find first visible recipe to start selection there
    for (int i = 0; i < NUM_RECIPES; i++) {
        if (isRecipeVisible(state, RECIPES[i])) {
            selected = i;
            break;
        }
    }

    while (true) {
        renderCraftingUI(state, selected);

        char input = getch();

        switch (input) {
            case 'w':
            case 'W':
                // Move up to previous visible recipe, wrap around
                do {
                    selected = (selected - 1 + NUM_RECIPES) % NUM_RECIPES;
                } while (!isRecipeVisible(state, RECIPES[selected]) && selected > 0);
                break;

            case 's':
            case 'S':
                // Move down to next visible recipe, stop at last visible
                do {
                    selected = (selected + 1) % NUM_RECIPES;
                } while (!isRecipeVisible(state, RECIPES[selected]) && selected < NUM_RECIPES - 1);
                // If we wrapped to invisible ones, go back to first visible
                if (!isRecipeVisible(state, RECIPES[selected])) {
                    for (int i = 0; i < NUM_RECIPES; i++) {
                        if (isRecipeVisible(state, RECIPES[i])) {
                            selected = i;
                            break;
                        }
                    }
                }
                break;

            case '\n':
            case '\r':
            case ' ':
                // Attempt craft
                if (canCraft(state, RECIPES[selected])) {
                    bool minigameTriggered = performCrafting(state, RECIPES[selected]);
                    if (minigameTriggered) {
                        return;
                    }
                    usleep(500000);
                } else {
                    std::cout << "\a";
                }

                break;

            case 'q':
            case 'Q':
                return;
        }
    }
}

// Detailed inventory display (full screen)
void showInventory(const GameState& state) {
    clearScreen();

    std::cout << COLOR_BOLD_CYAN;
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    " << COLOR_BOLD_YELLOW << "📦 INVENTORY 📦" << COLOR_BOLD_CYAN
              << "                      ║\n";
    std::cout << "╠════════════════════════════════════════════════════════════════╣\n";
    std::cout << COLOR_RESET;

    // Resources section
    std::cout << COLOR_BOLD_CYAN << "║ " << COLOR_BOLD_WHITE << "RESOURCES" << COLOR_BOLD_CYAN
              << std::string(58, ' ') << "║\n"
              << COLOR_RESET;

    auto printResource = [&](const char* emoji, const char* color, const char* name, int count, int width = 60) {
        std::cout << COLOR_BOLD_CYAN << "║ " << COLOR_RESET;
        std::cout << color << emoji << " " << std::left << std::setw(12) << name << COLOR_RESET;
        std::cout << ": " << std::setw(4) << count;
        std::cout << COLOR_BOLD_CYAN << std::string(width - 25, ' ') << "║\n"
                  << COLOR_RESET;
    };

    printResource("🪵", COLOR_WOOD, "Wood", state.player.inventory.wood);
    printResource("🪨", COLOR_STONE, "Stone", state.player.inventory.stone);
    printResource("⚙️ ", COLOR_IRON, "Iron", state.player.inventory.iron);
    printResource("🪙", COLOR_GOLD_ORE, "Gold", state.player.inventory.gold);
    printResource("💎", COLOR_DIAMOND, "Diamond", state.player.inventory.diamond);

    std::cout << COLOR_BOLD_CYAN;
    std::cout << "╠════════════════════════════════════════════════════════════════╣\n";
    std::cout << COLOR_RESET;

    // Equipment section
    std::cout << COLOR_BOLD_CYAN << "║ " << COLOR_BOLD_WHITE << "EQUIPMENT" << COLOR_BOLD_CYAN
              << std::string(58, ' ') << "║\n"
              << COLOR_RESET;

    std::cout << COLOR_BOLD_CYAN << "║ " << COLOR_RESET;
    std::cout << "⛏️  Pickaxe: " << getMaterialColor(state.player.equipment.pickaxe)
              << getMaterialName(state.player.equipment.pickaxe) << COLOR_RESET;
    std::cout << COLOR_BOLD_CYAN << std::string(37, ' ') << "║\n"
              << COLOR_RESET;

    std::cout << COLOR_BOLD_CYAN << "║ " << COLOR_RESET;
    std::cout << "🛡️  Armor:   " << getMaterialColor(state.player.equipment.armor)
              << getMaterialName(state.player.equipment.armor) << COLOR_RESET;
    std::cout << COLOR_BOLD_CYAN << std::string(37, ' ') << "║\n"
              << COLOR_RESET;

    std::cout << COLOR_BOLD_CYAN << "║ " << COLOR_RESET;
    std::cout << "❤️  Health:  " << COLOR_HEALTH << state.player.health << "/"
              << state.player.maxHealth << COLOR_RESET;
    std::cout << COLOR_BOLD_CYAN << std::string(40, ' ') << "║\n"
              << COLOR_RESET;

    // Special message for full diamond
    if (state.player.equipment.armor == MATERIAL_DIAMOND &&
        state.player.equipment.pickaxe == MATERIAL_DIAMOND) {
        std::cout << COLOR_BOLD_CYAN << "╠════════════════════════════════════════════════════════════════╣\n";
        std::cout << COLOR_RESET;
        std::cout << COLOR_BOLD_CYAN << "║ " << COLOR_BOLD_YELLOW << "⭐ FULL DIAMOND ACHIEVED! ⭐"
                  << COLOR_BOLD_CYAN << std::string(32, ' ') << "║\n"
                  << COLOR_RESET;
        std::cout << COLOR_BOLD_CYAN << "║ " << COLOR_DIM << "The elder awaits at the dragon carcass..."
                  << COLOR_BOLD_CYAN << std::string(25, ' ') << "║\n"
                  << COLOR_RESET;
    }

    std::cout << COLOR_BOLD_CYAN;
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    std::cout << COLOR_RESET;

    std::cout << COLOR_DIM << "\n    Press any key to continue..." << COLOR_RESET;
    getch();
}