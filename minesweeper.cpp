// =============================================================================
// minesweeper.cpp
// TermiCraft — Minesweeper minigame implementation 
//
// Minigame triggered when the player tries to mine an ore. 
//
// Grid generation: Solution grids are initialized with 0s, mine grids are
//                  initialized with a boolean value 'false', display grids are 
//                  initialized with '#'. Grid size and number of mines are based
//                  on difficulty levels. Random x, y coordinates are generated for
//                  mine positions and the solution grid and mine grid are appended 
//                  accordingly.
// Input:           Players input 'F' or 'R' followed by the x coordinate and y coordinate
//                  of the grid they wish to flag or reveal. Players can also enter 'Q'
//                  to quit. 
// Winning logic:   Players win by revealing all non-mine grids. checkWin() checks 
//                  whether this condition is satisfied.
// Game over logic: If the player reveals a grid that is a mine. 
// Revealing grid:  Revealing a non-mine grid will reveal an integer showing the 
//                  number of adjacent mines calculated using countAdjacentMines().
//                  If the revealed grid has no adjacent mines, a flood reveal is 
//                  triggered - meaning that all the adjacent grids will be revealed
//                  until a grid with adjacent mines is reached. 
// High score:      writeHighScore() records the time taken to complete the game if the
//                  time is faster than the current high score.
//
// Author:       Nan
// Dependencies: menu.h, colors.h, minesweeper.h, types.h
// =============================================================================
#include "minesweeper.h"
#include "menu.h"
#include "colors.h"

#include <iostream>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <fstream>
#include <chrono>
#include <limits>

// Constructor 

Minesweeper::Minesweeper(int size, int mines) : size(size), mines(mines) {
    initializeGrids(size);
    placeMines();
    fillSolutionGrid();
    gameOver = false;
    win      = false;
    startTime = std::chrono::steady_clock::now();
}

//Private helpers 
void Minesweeper::clearScreen() {
    std::cout << "\033[2J\033[H";
    std::cout.flush();
}

char Minesweeper::toUpper(char c) {
    return (char)toupper((unsigned char)c);
}

bool Minesweeper::isValidMove(int x, int y) {
    return x >= 0 && x < size && y >= 0 && y < size;
}

int Minesweeper::countAdjacentMines(int x, int y) {
    int count = 0;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0) continue;
            if (isValidMove(x + i, y + j) && mineGrid[x + i][y + j])
                count++;
        }
    }
    return count;
}

void Minesweeper::initializeGrids(int sz) {
    this->size = sz;
    mineGrid.assign(sz, std::vector<bool>(sz, false));
    solutionGrid.assign(sz, std::vector<int>(sz, 0));
    revealedGrid.assign(sz, std::vector<char>(sz, '#'));
}

void Minesweeper::placeMines() {
    int count = 0;
    while (count < mines) {
        int x = rand() % size;
        int y = rand() % size;
        if (!mineGrid[x][y]) {
            mineGrid[x][y] = true;
            count++;
        }
    }
}

void Minesweeper::fillSolutionGrid() {
    for (int x = 0; x < size; x++)
        for (int y = 0; y < size; y++)
            solutionGrid[x][y] = mineGrid[x][y] ? -1 : countAdjacentMines(x, y);
}

void Minesweeper::revealSingleCell(int x, int y) {
    if (!isValidMove(x, y) || revealedGrid[x][y] != '#') return;
    if (mineGrid[x][y]) { gameOver = true; return; }
    if (solutionGrid[x][y] == 0)
        floodReveal(x, y);
    else
        revealedGrid[x][y] = '0' + solutionGrid[x][y];
}

void Minesweeper::floodReveal(int x, int y) {
    if (!isValidMove(x, y) || revealedGrid[x][y] != '#') return;
    revealedGrid[x][y] = (solutionGrid[x][y] == 0) ? ' ' : ('0' + solutionGrid[x][y]);
    if (solutionGrid[x][y] != 0) return;
    for (int i = -1; i <= 1; ++i)
        for (int j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0) continue;
            floodReveal(x + i, y + j);
        }
}

void Minesweeper::flagCell(int x, int y) {
    if (!isValidMove(x, y)) return;
    if (revealedGrid[x][y] == '#')
        revealedGrid[x][y] = 'F';
    else if (revealedGrid[x][y] == 'F')
        revealedGrid[x][y] = '#';
    // Ignore flag attempts on already-revealed cells
}

bool Minesweeper::checkWin() {
    for (int x = 0; x < size; x++)
        for (int y = 0; y < size; y++)
            if (!mineGrid[x][y] && revealedGrid[x][y] == '#')
                return false;
    return true;
}

void Minesweeper::revealMines() {
    for (int x = 0; x < size; x++)
        for (int y = 0; y < size; y++)
            if (mineGrid[x][y] && revealedGrid[x][y] != 'F')
                revealedGrid[x][y] = 'M';
}

double Minesweeper::getElapsedTime() {
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = now - startTime;
    return elapsed.count();
}

// Display 

void Minesweeper::displayBoard() {
    std::string boardP = hpad(3 + size * 2);

    // Column header
    std::cout << "\n" << boardP << "   ";
    for (int i = 0; i < size; ++i)
        std::cout << i % 10 << " ";
    std::cout << "\n";

    for (int i = 0; i < size; ++i) {
        std::cout << boardP << i % 10 << "  ";
        for (int j = 0; j < size; ++j) {
            char c = revealedGrid[i][j];
            if      (c == 'F') std::cout << "\033[1;31mF\033[0m ";
            else if (c == 'M') std::cout << "\033[1;35m*\033[0m ";
            else if (c == '#') std::cout << "\033[38;5;240m.\033[0m ";
            else if (c == ' ') std::cout << "  ";
            else {
                // number: color by value
                const char* col = "\033[0m";
                switch (c) {
                    case '1': col = "\033[34m"; break;
                    case '2': col = "\033[32m"; break;
                    case '3': col = "\033[31m"; break;
                    case '4': col = "\033[34;1m"; break;
                    default:  col = "\033[31;1m"; break;
                }
                std::cout << col << c << "\033[0m ";
            }
        }
        std::cout << "\n";
    }

    // Timer
    std::cout << "\n" << boardP << "  Time: " << (int)getElapsedTime() << "s";
    std::cout << "   Mines: " << mines << "\n\n";
}

// High score saving

void Minesweeper::saveHighScore(double time) {
    const std::string filename = "minesweeper_best.txt";
    double best = 999999.0;
    std::ifstream fin(filename);
    if (fin) { fin >> best; fin.close(); }

    if (time < best) {
        std::ofstream fout(filename);
        if (fout) { fout << time; fout.close(); }
        std::cout << "\033[1;32m  *** New best time: " << (int)time << "s! ***\033[0m\n";
    }
}

void Minesweeper::showHighScore() {
    const std::string filename = "minesweeper_best.txt";
    std::ifstream fin(filename);
    if (fin) {
        double best;
        fin >> best;
        fin.close();
        std::cout << "  Best time: " << (int)best << "s\n";
    } else {
        std::cout << "  Best time: --\n";
    }
}

//Game over message

void Minesweeper::gameOverMessage() {
    ::clearAndCenterV(win ? 12 : 10);
    std::string Aw = hpad(63);   
    std::string Ag = hpad(80);   

    if (win) {
        std::cout << "\033[1;32m\n"
            << Aw << "   ██╗   ██╗ ██████╗ ██╗   ██╗    ██╗    ██╗██╗███╗   ██╗██╗\n"
            << Aw << "   ╚██╗ ██╔╝██╔═══██╗██║   ██║    ██║    ██║██║████╗  ██║██║\n"
            << Aw << "    ╚████╔╝ ██║   ██║██║   ██║    ██║ █╗ ██║██║██╔██╗ ██║██║\n"
            << Aw << "     ╚██╔╝  ██║   ██║██║   ██║    ██║███╗██║██║██║╚██╗██║██║\n"
            << Aw << "      ██║   ╚██████╔╝╚██████╔╝    ╚███╔███╔╝██║██║ ╚████║██║\n"
            << Aw << "      ╚═╝    ╚═════╝  ╚═════╝      ╚══╝╚══╝ ╚═╝╚═╝  ╚═══╝╚═╝\n"
            "\033[0m\n";
        double t = getElapsedTime();
        std::cout << Aw << "  Cleared in " << (int)t << " seconds!\n\n";
        saveHighScore(t);
        {
            // Center "Best time: Xs"
            const std::string filename = "minesweeper_best.txt";
            std::ifstream fin(filename);
            if (fin) {
                double best; fin >> best; fin.close();
                std::cout << Aw << "  Best time: " << (int)best << "s\n";
            } else {
                std::cout << Aw << "  Best time: --\n";
            }
        }
    } else {
        std::cout << "\033[1;31m\n"
            << Ag << "   ██████╗  █████╗ ███╗   ███╗███████╗     ██████╗ ██╗   ██╗███████╗██████╗\n"
            << Ag << "  ██╔════╝ ██╔══██╗████╗ ████║██╔════╝    ██╔═══██╗██║   ██║██╔════╝██╔══██╗\n"
            << Ag << "  ██║  ███╗███████║██╔████╔██║█████╗      ██║   ██║██║   ██║█████╗  ██████╔╝\n"
            << Ag << "  ██║   ██║██╔══██║██║╚██╔╝██║██╔══╝      ██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██╗\n"
            << Ag << "  ╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗    ╚██████╔╝ ╚████╔╝ ███████╗██║  ██║\n"
            << Ag << "   ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝     ╚═════╝   ╚═══╝  ╚══════╝╚═╝  ╚═╝\n"
            "\033[0m\n";
    }

    {
        const char* prompt = "Press any key to continue...";
        std::cout << "\n" << hpad((int)strlen(prompt)) << prompt;
    }
    std::cout.flush();
    char dummy;
    read(STDIN_FILENO, &dummy, 1);
}

//Input

void Minesweeper::getPlayerInput() {
    while (true) {
        int x, y;
        char action;
        std::cout << hpad(58) << "Action (R=reveal, F=flag, Q=flee [-2x penalty]) row col: ";
        if (!(std::cin >> action)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << hpad(58) << "Invalid input. Try again.\n";
            continue;
        }
        action = toUpper(action);
        if (action == 'Q') {
            g_minigameForfeited = true;
            gameOver = true;
            win = false;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return;
        }
        if (action != 'R' && action != 'F') {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << hpad(58) << "Use R (reveal), F (flag), or Q (flee).\n";
            continue;
        }
        if (!(std::cin >> x >> y)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << hpad(58) << "Enter valid row and column numbers.\n";
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (!isValidMove(x, y)) {
            std::cout << hpad(58) << "Out of bounds. Row/col must be within the grid.\n";
            continue;
        }
        makeMove(action, x, y);
        return;
    }
}

void Minesweeper::makeMove(char action, int x, int y) {
    if (!isValidMove(x, y)) return;

    if (action == 'R') {
        if (revealedGrid[x][y] == '#') {
            if (solutionGrid[x][y] == 0)
                floodReveal(x, y);
            else
                revealSingleCell(x, y);
        }
    } else if (action == 'F') {
        flagCell(x, y);
    }
}

//Main game loop 

void Minesweeper::playGame() {
    while (!gameOver) {
        int boxW = std::max(3 + size * 2 + 4, 36);
        ::clearAndCenterV(size + 9);
        std::string P = hpad(boxW + 2);

        auto msTopBar = [&]() {
            std::cout << P << "\xe2\x95\x94";
            for (int i = 0; i < boxW; i++) std::cout << "\xe2\x95\x90";
            std::cout << "\xe2\x95\x97\n";
        };
        auto msBotBar = [&]() {
            std::cout << P << "\xe2\x95\x9a";
            for (int i = 0; i < boxW; i++) std::cout << "\xe2\x95\x90";
            std::cout << "\xe2\x95\x9d\n";
        };
        auto msLine = [&](const char* s, int dispW) {
            int padL = (boxW - dispW) / 2;
            int padR = boxW - dispW - padL;
            std::cout << P << "\xe2\x95\x91" << std::string(padL, ' ') << s << std::string(padR, ' ') << "\xe2\x95\x91\n";
        };

        std::cout << "\033[1;33m";
        msTopBar();
        msLine("💣 MINESWEEPER 💣", 17);
        msLine("R row col = reveal", 18);
        msLine("F row col = flag/unflag", 23);
        msBotBar();
        std::cout << "\033[0m\n";

        displayBoard();

        if (checkWin()) {
            win = true;
            gameOver = true;
            break;
        }
        getPlayerInput();
    }

    clearScreen();
    revealMines();
    displayBoard();
    gameOverMessage();
}


bool runMinesweeper(int gridSize) {
    int mineCount = (gridSize * gridSize) / 5;
    if (mineCount < 3) mineCount = 3;

    // Restore echo/canonical so std::cin works
    struct termios cooked;
    tcgetattr(STDIN_FILENO, &cooked);
    cooked.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &cooked);

    srand((unsigned int)time(0));
    Minesweeper game(gridSize, mineCount);
    game.playGame();

    // Return to raw mode (caller — main.cpp — will call setupTerminal() after us,
    // but we restore raw here in case the minigame exits mid-session)
    cooked.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &cooked);

    return game.didWin();
}
