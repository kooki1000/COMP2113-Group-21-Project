// =============================================================================
// minesweeper.h
// TermiCraft — Minesweeper Minigame Module Header
//
// Declares the Minesweeper minigame for TermiCraft. This module handles the
// grid-based minesweeper minigame.
//
// The minigame is triggered automatically in the main game loop when the
// player tries to mine an ore block (after successful stone-tier Wordle
// progression). It enforces classic Minesweeper rules with numbered hints,
// flagging, flood-fill reveal, and win/loss detection. High-score tracking
// with elapsed time adds replay value and ties into the game's equipment
// progression system.
//
// Integration with the main TermiCraft game is handled through the
// pendingUpgrade flag in GameState. On successful completion, the player
// successfully mines the ore; failure results in no ore; escape results
// in losing HP.
//
// Author:       Nan
// Dependencies: termios.h, unistd.h
// =============================================================================
#ifndef MINESWEEPER_H
#define MINESWEEPER_H

#include <vector>
#include <chrono>
#include <termios.h>
#include <unistd.h>

class Minesweeper {
public:
    Minesweeper(int size, int mines);
    void playGame();
    bool didWin() const { return win; }

private:
    int size;
    int mines;
    std::vector<std::vector<bool>> mineGrid;
    std::vector<std::vector<int>>  solutionGrid;
    std::vector<std::vector<char>> revealedGrid;
    bool gameOver;
    bool win;
    std::chrono::steady_clock::time_point startTime;

    void initializeGrids(int size);
    void placeMines();
    void fillSolutionGrid();
    bool isValidMove(int x, int y);
    int  countAdjacentMines(int x, int y);
    void revealSingleCell(int x, int y);
    void floodReveal(int x, int y);
    void flagCell(int x, int y);
    bool checkWin();
    void revealMines();
    void displayBoard();
    void clearScreen();
    void getPlayerInput();
    void makeMove(char action, int x, int y);
    char toUpper(char c);
    double getElapsedTime();
    void showHighScore();
    void writeHighScore(double time);
    void gameOverMessage();
};

bool runMinesweeper(int gridSize);

#endif
