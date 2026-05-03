```cpp
/*
 * sudoku.cpp
 *
 * Sudoku minigame for TermiCraft.
 *
 * CORE IDEA:
 * This module implements a fully playable terminal-based Sudoku game.
 * The game generates a valid Sudoku grid using backtracking, removes
 * some cells to create a puzzle, and allows the player to fill them.
 *
 * DIFFICULTY MODES:
 * - Easy / Normal → 6x6 grid (values 1–6, subgrid 2x3)
 * - Hard          → 9x9 grid (values 1–9, subgrid 3x3)
 *
 * DESIGN PHILOSOPHY:
 * - Keep logic self-contained (single class)
 * - Use recursive backtracking for generation
 * - Use terminal rendering with ANSI colors for UX
 * - Keep interaction simple (text input)
 *
 * LIMITATIONS (IMPORTANT):
 * - Does NOT guarantee a unique solution
 * - Solved state only checks if all cells are filled (not correctness)
 * - Random removal is naive (no difficulty heuristics)
 *
 * Despite these, this is a solid baseline Sudoku engine.
 */

#include "colors.h"   // ANSI color codes for styling terminal output
#include "menu.h"     // Provides clearScreen() and menu-related utilities
#include "types.h"    // Defines Difficulty enum (DIFF_EASY, etc.)

#include <algorithm>  // std::shuffle
#include <cstdlib>    // rand(), srand()
#include <ctime>      // time() for seeding RNG
#include <iostream>   // std::cout, std::cin
#include <random>     // modern random utilities (mt19937)
#include <sstream>    // parsing input strings
#include <string>     // std::string
#include <termios.h>  // terminal control (raw/cooked mode switching)
#include <unistd.h>   // STDIN_FILENO
#include <vector>     // 2D grid storage

class SudokuGame {
private:
    /*
     * === CORE GAME PARAMETERS ===
     */

    int size;   // Board dimension (6 or 9)
    int boxR;   // Number of rows in each subgrid
    int boxC;   // Number of columns in each subgrid

    /*
     * === GAME STATE ===
     */

    // Main Sudoku grid:
    // 0 represents an empty cell
    std::vector<std::vector<int>> grid;

    // Tracks which cells are fixed (part of original puzzle)
    // true  → cannot be changed by player
    // false → player-editable
    std::vector<std::vector<bool>> fixed;

    int cellsToRemove;   // Number of cells removed to create puzzle
    std::string lastMessage; // Stores last error/info message for UI

    /*
     * === UI HELPER ===
     * Prints horizontal divider between subgrid rows.
     */
    void printDivider() {
        std::cout << "    " << COLOR_CYAN << "+";

        // Print 3 dashes per column for visual spacing
        for (int c = 0; c < size; c++) {
            std::cout << "---";

            // Add thicker divider at subgrid boundaries
            if ((c + 1) % boxC == 0) {
                std::cout << "+";
            }
        }

        std::cout << COLOR_RESET << "\n";
    }

    /*
     * === VALIDATION FUNCTION ===
     * Checks whether placing 'num' at (r, c) is valid.
     *
     * RULES CHECKED:
     * 1. Row uniqueness
     * 2. Column uniqueness
     * 3. Subgrid uniqueness
     *
     * Returns true if safe, false otherwise.
     */
    bool isSafe(int r, int c, int num) {
        // Check row and column
        for (int i = 0; i < size; i++) {
            if (grid[r][i] == num || grid[i][c] == num)
                return false;
        }

        // Compute top-left corner of subgrid
        int startRow = r - r % boxR;
        int startCol = c - c % boxC;

        // Check subgrid
        for (int i = 0; i < boxR; i++) {
            for (int j = 0; j < boxC; j++) {
                if (grid[i + startRow][j + startCol] == num)
                    return false;
            }
        }

        return true;
    }

    /*
     * === FIND EMPTY CELL ===
     * Finds the next unassigned cell (value 0).
     *
     * Returns true and sets (r, c) if found.
     * Returns false if grid is full.
     */
    bool findUnassigned(int& r, int& c) {
        for (r = 0; r < size; r++) {
            for (c = 0; c < size; c++) {
                if (grid[r][c] == 0)
                    return true;
            }
        }
        return false;
    }

    /*
     * === BACKTRACKING SOLVER ===
     *
     * Recursively fills the grid using:
     * - Try all numbers in random order
     * - Place if valid
     * - Recurse
     * - Backtrack if needed
     *
     * This is used BOTH for:
     * - Generating a complete valid board
     * - (Potentially) solving puzzles
     */
    bool solveGrid() {
        int r, c;

        // If no empty cell → solved
        if (!findUnassigned(r, c))
            return true;

        // Generate list of candidate numbers
        std::vector<int> nums;
        for (int i = 1; i <= size; i++)
            nums.push_back(i);

        // Shuffle for randomness → different boards each run
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(nums.begin(), nums.end(), g);

        // Try each candidate
        for (int num : nums) {
            if (isSafe(r, c, num)) {
                grid[r][c] = num;

                if (solveGrid())
                    return true;

                // Backtrack
                grid[r][c] = 0;
            }
        }

        return false;
    }

    /*
     * === BOARD GENERATION ===
     *
     * Steps:
     * 1. Create empty grid
     * 2. Fill completely using solver
     * 3. Remove cells randomly
     * 4. Mark remaining cells as fixed
     */
    void generateBoard() {
        // Initialize empty grid
        grid.assign(size, std::vector<int>(size, 0));
        fixed.assign(size, std::vector<bool>(size, false));
        lastMessage = "";

        // Fill entire grid with valid solution
        solveGrid();

        // Randomly remove cells
        int removed = 0;
        while (removed < cellsToRemove) {
            int r = rand() % size;
            int c = rand() % size;

            // Only remove non-empty cells
            if (grid[r][c] != 0) {
                grid[r][c] = 0;
                removed++;
            }
        }

        // Mark remaining cells as fixed
        for (int r = 0; r < size; r++) {
            for (int c = 0; c < size; c++) {
                if (grid[r][c] != 0) {
                    fixed[r][c] = true;
                }
            }
        }
    }

    /*
     * === COMPLETION CHECK ===
     *
     * NOTE:
     * This ONLY checks if all cells are filled.
     * It does NOT verify correctness.
     */
    bool isSolved() {
        for (int r = 0; r < size; r++) {
            for (int c = 0; c < size; c++) {
                if (grid[r][c] == 0)
                    return false;
            }
        }
        return true;
    }

public:
    /*
     * === CONSTRUCTOR ===
     *
     * Sets parameters based on difficulty:
     * - Grid size
     * - Subgrid dimensions
     * - Number of cells removed
     */
    SudokuGame(Difficulty diff) {
        srand(static_cast<unsigned int>(time(nullptr)));

        if (diff == DIFF_EASY) {
            size = 6; boxR = 2; boxC = 3;
            cellsToRemove = 15;
        }
        else if (diff == DIFF_NORMAL) {
            size = 6; boxR = 2; boxC = 3;
            cellsToRemove = 22;
        }
        else {
            size = 9; boxR = 3; boxC = 3;
            cellsToRemove = 45;
        }

        generateBoard();
    }

    /*
     * === RENDER BOARD ===
     *
     * Displays:
     * - Title UI
     * - Column/row indices
     * - Grid with color coding
     *
     * Color legend:
     * - Yellow → fixed cells
     * - Green  → user-filled
     * - Dim    → empty cells
     */
    void displayBoard() {
        clearScreen();

        // Title banner
        std::cout << COLOR_BOLD_CYAN;
        std::cout << "\n    ╔══════════════════════════════════╗\n";
        std::cout <<   "    ║      🔢 SUDOKU MINIGAME 🔢       ║\n";
        std::cout <<   "    ║  Fill the grid so every row,     ║\n";
        std::cout <<   "    ║  column, and box is unique!      ║\n";
        std::cout <<   "    ╚══════════════════════════════════╝\n\n";
        std::cout << COLOR_RESET;

        // Column labels
        std::cout << "      ";
        for (int c = 0; c < size; c++) {
            std::cout << COLOR_DIM << (c + 1) << "  " << COLOR_RESET;
            if ((c + 1) % boxC == 0)
                std::cout << "  ";
        }
        std::cout << "\n";

        printDivider();

        // Print rows
        for (int r = 0; r < size; r++) {
            std::cout << COLOR_DIM << "  " << (r + 1) << COLOR_RESET
                      << " " << COLOR_CYAN << "|" << COLOR_RESET;

            for (int c = 0; c < size; c++) {
                if (grid[r][c] == 0) {
                    std::cout << COLOR_DIM << " . " << COLOR_RESET;
                }
                else if (fixed[r][c]) {
                    std::cout << COLOR_BOLD_YELLOW
                              << " " << grid[r][c] << " "
                              << COLOR_RESET;
                }
                else {
                    std::cout << COLOR_BOLD_GREEN
                              << " " << grid[r][c] << " "
                              << COLOR_RESET;
                }

                if ((c + 1) % boxC == 0) {
                    std::cout << COLOR_CYAN << "|" << COLOR_RESET;
                }
            }

            std::cout << "\n";

            if ((r + 1) % boxR == 0) {
                printDivider();
            }
        }

        // Show last message (errors, etc.)
        if (!lastMessage.empty()) {
            std::cout << "\n    " << COLOR_RED
                      << lastMessage << COLOR_RESET;
            lastMessage = "";
        }
        else {
            std::cout << "\n";
        }

        std::cout << "\n";
    }

    /*
     * === MAIN GAME LOOP ===
     *
     * Handles:
     * - Input parsing
     * - Validation
     * - Updating grid
     * - Rendering
     */
    bool playGame() {
        while (!isSolved()) {
            displayBoard();

            // Instructions
            std::cout << COLOR_WHITE
                      << "    Enter move (Row Col Value)\n"
                      << COLOR_RESET;

            std::cout << COLOR_DIM
                      << "    Example: 1 3 5\n"
                      << "    Enter '0 0 0' to quit.\n"
                      << COLOR_RESET;

            std::cout << COLOR_WHITE
                      << "\n    Your move: "
                      << COLOR_RESET;

            /*
             * TERMINAL MODE HANDLING:
             * Ensures proper input behavior (canonical mode + echo)
             */
            struct termios cooked, raw;
            tcgetattr(STDIN_FILENO, &cooked);
            raw = cooked;

            cooked.c_lflag |= (ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &cooked);

            std::string input;
            std::getline(std::cin, input);

            tcsetattr(STDIN_FILENO, TCSANOW, &raw);

            /*
             * INPUT PARSING
             */
            std::stringstream ss(input);
            int r, c, val;

            if (ss >> r >> c >> val) {

                // Quit condition
                if (r == 0 && c == 0 && val == 0)
                    return false;

                // Convert to 0-based indexing
                r--; c--;

                // Bounds check
                if (r < 0 || r >= size || c < 0 || c >= size) {
                    lastMessage = "Invalid row or column!";
                    continue;
                }

                // Value check
                if (val < 0 || val > size) {
                    lastMessage =
                        "Value must be between 1 and "
                        + std::to_string(size)
                        + " (or 0 to clear).";
                    continue;
                }

                // Prevent editing fixed cells
                if (fixed[r][c]) {
                    lastMessage =
                        "You can't change a fixed number!";
                    continue;
                }

                // Clear cell
                if (val == 0) {
                    grid[r][c] = 0;
                }
                else {
                    // Temporarily clear to check validity
                    int temp = grid[r][c];
                    grid[r][c] = 0;

                    if (isSafe(r, c, val)) {
                        grid[r][c] = val;
                    }
                    else {
                        grid[r][c] = temp;
                        lastMessage =
                            "Invalid move! Conflict detected.";
                    }
                }
            }
            else {
                lastMessage =
                    "Enter 3 numbers separated by spaces.";
            }
        }

        /*
         * WIN SCREEN
         */
        lastMessage = "";
        displayBoard();

        std::cout << "\n" << COLOR_BOLD_GREEN
                  << "    ╔══════════════════════════════════╗\n"
                  << "    ║  🎉 Board Filled!               ║\n"
                  << "    ╚══════════════════════════════════╝\n"
                  << COLOR_RESET;

        std::cout << "\n"
                  << COLOR_DIM
                  << "    Press ENTER to continue..."
                  << COLOR_RESET;

        // Wait for user input
        struct termios cooked, raw;
        tcgetattr(STDIN_FILENO, &cooked);
        raw = cooked;
        cooked.c_lflag |= (ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &cooked);

        std::string dummy;
        std::getline(std::cin, dummy);

        tcsetattr(STDIN_FILENO, TCSANOW, &raw);

        return true;
    }
};

/*
 * === EXTERNAL ENTRY POINT ===
 *
 * This function allows integration with the rest of TermiCraft.
 * Simply call runSudoku(diff) to start the game.
 */
bool runSudoku(Difficulty diff) {
    SudokuGame game(diff);
    return game.playGame();
}
```
