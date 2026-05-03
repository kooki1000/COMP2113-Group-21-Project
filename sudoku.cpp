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
 * ARCHITECTURE OVERVIEW:
 * - SudokuGame class owns all state (grid, UI state, constraints)
 * - generateBoard() builds puzzle from solved grid
 * - solveGrid() is a full backtracking generator
 * - playGame() handles user interaction loop
 * - displayBoard() renders terminal UI
 *
 * CORE DATA STRUCTURES:
 * - grid[r][c]   → Sudoku board values (0 = empty)
 * - fixed[r][c]  → original puzzle clues (immutable cells)
 * - size         → board dimension (6 or 9)
 * - boxR / boxC  → subgrid structure
 *
 * GAME LOOP FLOW:
 * 1. Constructor initializes difficulty parameters
 * 2. generateBoard() builds a full valid solution
 * 3. Cells are removed randomly to form puzzle
 * 4. Player interacts via playGame()
 * 5. Board updates until completion or exit
 *
 */




#include "colors.h"
#include "menu.h"
#include "types.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>




/*
 * CLASS: SudokuGame
 *
 * Encapsulates:
 * - Board generation
 * - Constraint checking
 * - Rendering logic
 * - Player input handling
 */
class SudokuGame {
private:

    // ----------------------------
    // BOARD CONFIGURATION STATE
    // ----------------------------

    int size;                 // board dimension (6 or 9)
    int boxR;                 // subgrid rows
    int boxC;                 // subgrid columns

    // ----------------------------
    // CORE GAME STATE
    // ----------------------------

    std::vector<std::vector<int>> grid;     // current board state
    std::vector<std::vector<bool>> fixed;    // immutable clue cells

    int cellsToRemove;       // difficulty-based puzzle sparsity

    // ----------------------------
    // UI STATE
    // ----------------------------

    std::string lastMessage;  // feedback message shown after move
    std::string boardP;       // padding used for alignment

    /*
     * waitForKey()
     *
     * PURPOSE:
     * Blocks execution until a single key is pressed.
     *
     * IMPLEMENTATION NOTE:
     * Uses termios to temporarily disable canonical input buffering
     * so input is captured immediately.
     */
    void waitForKey() {
        const char* prompt = "Press any key to continue...";
        std::cout << "\n" << hpad(28) << COLOR_DIM << prompt << COLOR_RESET;
        std::cout.flush();

        struct termios oldt, raw;
        tcgetattr(STDIN_FILENO, &oldt);

        raw = oldt;
        raw.c_lflag &= ~(ICANON | ECHO);

        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        tcflush(STDIN_FILENO, TCIFLUSH);

        char dummy;
        read(STDIN_FILENO, &dummy, 1);

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }

    /*
     * showEndScreen()
     *
     * PURPOSE:
     * Displays win/lose ASCII screen and pauses.
     */



    void showEndScreen(bool won) {
        clearAndCenterV(won ? 12 : 11);

        std::string Aw = hpad(59);
        std::string Ag = hpad(80);

        if (won) {
            // WIN CONDITION UI
            std::cout << COLOR_BOLD_GREEN << "\n"
                << Aw << "   ██╗   ██╗ ██████╗ ██╗   ██╗    ██╗    ██╗██╗███╗   ██╗\n"
                << Aw << "   ╚██╗ ██╔╝██╔═══██╗██║   ██║    ██║    ██║██║████╗  ██║\n"
                << Aw << "    ╚████╔╝ ██║   ██║██║   ██║    ██║ █╗ ██║██║██╔██╗ ██║\n"
                << Aw << "     ╚██╔╝  ██║   ██║██║   ██║    ██║███╗██║██║██║╚██╗██║\n"
                << Aw << "      ██║   ╚██████╔╝╚██████╔╝    ╚███╔███╔╝██║██║ ╚████║\n"
                << Aw << "      ╚═╝    ╚═════╝  ╚═════╝      ╚══╝╚══╝ ╚═╝╚═╝  ╚═══╝\n"
                << COLOR_RESET << "\n";

            std::cout << "\n" << hpad(24) << "Sudoku board solved.\n";
        } else {
            // EXIT / LOSS SCREEN UI
            std::cout << COLOR_BOLD_RED << "\n"
                << Ag << "   ██████╗  █████╗ ███╗   ███╗███████╗     ██████╗ ██╗   ██╗███████╗██████╗\n"
                << Ag << "  ██╔════╝ ██╔══██╗████╗ ████║██╔════╝    ██╔═══██╗██║   ██║██╔════╝██╔══██╗\n"
                << Ag << "  ██║  ███╗███████║██╔████╔██║█████╗      ██║   ██║██║   ██║█████╗  ██████╔╝\n"
                << Ag << "  ██║   ██║██╔══██║██║╚██╔╝██║██╔══╝      ██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██╗\n"
                << Ag << "  ╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗    ╚██████╔╝ ╚████╔╝ ███████╗██║  ██║\n"
                << Ag << "   ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝     ╚═════╝   ╚═══╝  ╚══════╝╚═╝  ╚═╝\n"
                << COLOR_RESET << "\n";

            std::cout << "\n" << hpad(17) << "You fled Sudoku.\n";
        }

        // Always pause before returning control
        waitForKey();
    }

    /*
     * printDivider()
     *
     * PURPOSE:
     * Draws horizontal separator lines between subgrids.
     */



    void printDivider() {
        std::cout << boardP << "    " << COLOR_CYAN << "+";

        for (int c = 0; c < size; c++) {
            std::cout << "---";

            // Subgrid boundary marker
            if ((c + 1) % boxC == 0) {
                std::cout << "+";
            }
        }

        std::cout << COLOR_RESET << "\n";
    }

    /*
     * isSafe()
     *
     * PURPOSE:
     * Validates Sudoku constraints for a candidate move.
     *
     * RULES:
     * - No duplicates in row
     * - No duplicates in column
     * - No duplicates in subgrid
     */


    bool isSafe(int r, int c, int num) {

        // Row & column validation
        for (int i = 0; i < size; i++) {
            if (grid[r][i] == num || grid[i][c] == num)
                return false;
        }

        // Subgrid calculation
        int startRow = r - r % boxR;
        int startCol = c - c % boxC;

        // Subgrid validation
        for (int i = 0; i < boxR; i++) {
            for (int j = 0; j < boxC; j++) {
                if (grid[i + startRow][j + startCol] == num)
                    return false;
            }
        }

        return true;
    }

    /*
     * findUnassigned()
     *
     * PURPOSE:
     * Finds first empty cell (0) for backtracking solver.
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
     * solveGrid()
     *
     * PURPOSE:
     * Generates a full valid Sudoku solution using backtracking.
     *
     * TECHNIQUE:
     * - DFS recursion
     * - randomized candidate order
     * - backtracking on invalid states
     */
    bool solveGrid() {
        int r, c;

        if (!findUnassigned(r, c))
            return true;

        std::vector<int> nums;
        for (int i = 1; i <= size; i++)
            nums.push_back(i);

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(nums.begin(), nums.end(), g);

        for (int num : nums) {
            if (isSafe(r, c, num)) {
                grid[r][c] = num;

                if (solveGrid())
                    return true;

                grid[r][c] = 0;
            }
        }

        return false;
    }

    /*
     * generateBoard()
     *
     * PURPOSE:
     * Builds playable Sudoku puzzle from full solution.
     *
     * STEPS:
     * 1. Generate full solved grid
     * 2. Randomly remove cells
     * 3. Mark remaining cells as fixed clues
     */


    void generateBoard() {
        grid.assign(size, std::vector<int>(size, 0));
        fixed.assign(size, std::vector<bool>(size, false));

        solveGrid();

        int removed = 0;
        while (removed < cellsToRemove) {
            int r = rand() % size;
            int c = rand() % size;

            if (grid[r][c] != 0) {
                grid[r][c] = 0;
                removed++;
            }
        }

        // Mark clues
        for (int r = 0; r < size; r++) {
            for (int c = 0; c < size; c++) {
                if (grid[r][c] != 0)
                    fixed[r][c] = true;
            }
        }
    }

    /*
     * isSolved()
     *
     * NOTE:
     * This only checks completeness, not correctness.
     */
    bool isSolved() {
        for (int r = 0; r < size; r++)
            for (int c = 0; c < size; c++)
                if (grid[r][c] == 0)
                    return false;

        return true;
    }


public:
    /*
     * Constructor:
     * Initializes difficulty parameters and generates puzzle.
     */
    SudokuGame(Difficulty diff) {
        srand(time(nullptr));

        if (diff == DIFF_EASY) {
            size = 6; boxR = 2; boxC = 3; cellsToRemove = 15;
        } else if (diff == DIFF_NORMAL) {
            size = 6; boxR = 2; boxC = 3; cellsToRemove = 22;
        } else {
            size = 9; boxR = 3; boxC = 3; cellsToRemove = 45;
        }

        generateBoard();
    }

    /*
     * displayBoard()
     *
     * PURPOSE:
     * Renders full Sudoku grid in terminal UI.
     */
    void displayBoard() {

        int totalLines = 5 + 1 + (size + (size / boxR) + 1) + 5;
        clearAndCenterV(totalLines);

        std::string titleP = hpad(36);
        boardP = hpad(size * 3 + (size / boxC) + 4);

        std::cout << COLOR_BOLD_CYAN;

        std::cout << "\n" << titleP << "╔══════════════════════════════════╗\n";
        std::cout << titleP << "║      🔢 SUDOKU MINIGAME 🔢      ║\n";
        std::cout << titleP << "║  Fill the grid so every row,     ║\n";
        std::cout << titleP << "║  column, and box is unique!      ║\n";
        std::cout << titleP << "╚══════════════════════════════════╝\n\n";

        std::cout << COLOR_RESET;

        // Column headers
        std::cout << boardP << "     ";
        for (int c = 0; c < size; c++) {
            std::cout << COLOR_DIM << " " << (c + 1) << " " << COLOR_RESET;
            if ((c + 1) % boxC == 0) std::cout << " ";
        }
        std::cout << "\n";



        
        printDivider();

        // Grid rendering loop
        for (int r = 0; r < size; r++) {
            std::cout << boardP << COLOR_DIM << "  " << (r + 1)
                      << COLOR_RESET << " " << COLOR_CYAN << "|" << COLOR_RESET;

            for (int c = 0; c < size; c++) {
                if (grid[r][c] == 0)
                    std::cout << COLOR_DIM << " . " << COLOR_RESET;
                else if (fixed[r][c])
                    std::cout << COLOR_BOLD_YELLOW << " " << grid[r][c] << " " << COLOR_RESET;
                else
                    std::cout << COLOR_BOLD_GREEN << " " << grid[r][c] << " " << COLOR_RESET;

                if ((c + 1) % boxC == 0)
                    std::cout << COLOR_CYAN << "|" << COLOR_RESET;
            }

            std::cout << "\n";

            if ((r + 1) % boxR == 0)
                printDivider();
        }

        std::cout << "\n";
    }




    /*
     * playGame()
     *
     * MAIN GAME LOOP:
     * Handles input → validation → update → redraw cycle
     */



    bool playGame() {
        while (!isSolved()) {
            displayBoard();

            std::cout << COLOR_WHITE << boardP
                      << "Enter move (Row Col Value) e.g., '1 3 5'.\n"
                      << COLOR_RESET;

            std::cout << COLOR_DIM << boardP
                      << "Enter '0 0 0' to quit.\n"
                      << COLOR_RESET;

            std::cout << COLOR_WHITE << "\n" << boardP
                      << "Your move: " << COLOR_RESET;

            struct termios cooked, raw;
            tcgetattr(STDIN_FILENO, &cooked);
            raw = cooked;

            cooked.c_lflag |= (ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &cooked);

            std::string input;
            std::getline(std::cin, input);

            tcsetattr(STDIN_FILENO, TCSANOW, &raw);

            std::stringstream ss(input);
            int r, c, val;

            if (ss >> r >> c >> val) {

                if (r == 0 && c == 0 && val == 0) {
                    g_minigameForfeited = true;
                    showEndScreen(false);
                    return false;
                }

                r--; c--;

                if (r < 0 || r >= size || c < 0 || c >= size) {
                    lastMessage = "Invalid row or column!";
                    continue;
                }

                if (val < 0 || val > size) {
                    lastMessage = "Invalid value!";
                    continue;
                }

                if (fixed[r][c]) {
                    lastMessage = "You can't change a fixed number!";
                    continue;
                }

                if (val == 0) {
                    grid[r][c] = 0;
                } else {
                    int temp = grid[r][c];
                    grid[r][c] = 0;

                    if (isSafe(r, c, val)) {
                        grid[r][c] = val;
                    } else {
                        grid[r][c] = temp;
                        lastMessage = "Invalid move!";
                    }
                }
            } else {
                lastMessage = "Invalid input format.";
            }
        }

        showEndScreen(true);
        return true;
    }
};

// External entry point for engine
bool runSudoku(Difficulty diff) {
    SudokuGame game(diff);
    return game.playGame();
}
