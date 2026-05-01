#include <iostream>
#include <cctype>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <chrono>
#include "types.h"
#include "minesweeper.h"
#include "menu.h"
#include "colors.h"

void writeHighScore(double time) {//creates high score file and writes to it
    std::string filename = "minesweeper_highscore.txt";
    double bestTime = 999999.0;   
    std::ifstream fin(filename);
    if (fin) {
        fin >> bestTime;
        fin.close();
    }
    if (time < bestTime) {
        std::ofstream fout(filename);
        if (fout) {
            fout << time;
            fout.close();
            std::cout << "\033[1;32m  *** New best time: " << (int)time << "s! ***\033[0m\n";
        }
    }
}
void Minesweeper::showHighScore() {
    const std::string filename = "minesweeper_highscore.txt";
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
void Minesweeper::clearScreen() {
    std::cout << "\033[2J\033[H";
    std::cout.flush();
}

char Minesweeper::toUpper(char c){
    return toupper(c);
}

bool Minesweeper::isValidMove(int x, int y) {
    if (x < 0 || x >= size || y < 0 || y >= size) {
        return false;
    }
    return true;
}

int Minesweeper::countAdjacentMines(int x, int y) {
    int count = 0;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0) continue;
            if (isValidMove(x + i, y + j)) {
                if (mineGrid[x + i][y + j]) {
                    count++;
                }
            }
        }
    }
    return count;
}

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
    std::cout << "\n" << boardP << "  Time: " << (int)getElapsedTime() << "s";
    std::cout << "   Mines: " << mines << "\n\n";
}

void Minesweeper::initializeGrids(int size){
    this->size = size;
    mineGrid.assign(size, std::vector<bool>(size, false));  
    solutionGrid.assign(size, std::vector<int>(size, 0)); 
    revealedGrid.assign(size, std::vector<char>(size, '#')); 
}

void Minesweeper::placeMines(){
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

void Minesweeper::fillSolutionGrid(){
    for(int x=0; x<size; x++){
        for(int y=0; y<size; y++){
            if(mineGrid[x][y]){
                solutionGrid[x][y] = -1;
            } else {
                solutionGrid[x][y] = countAdjacentMines(x, y);
            }
        }
    }
}

void Minesweeper::revealSingleCell(int x, int y){
    if (!isValidMove(x, y) || revealedGrid[x][y] != '#') {
        return;
    }
    if(mineGrid[x][y]){
        gameOver = true;
        return;
    }
    if (solutionGrid[x][y] == 0) {
        floodReveal(x, y);
    } else {
        revealedGrid[x][y] = '0' + solutionGrid[x][y];
    }
}

void Minesweeper::floodReveal(int x, int y){
    if (!isValidMove(x, y) || revealedGrid[x][y] != '#') {
        return;
    }

    if (solutionGrid[x][y] == 0) {
        revealedGrid[x][y] = ' ';
    } else {
        revealedGrid[x][y] = '0' + solutionGrid[x][y];
    }

    if (solutionGrid[x][y] != 0) {
        return;
    }

    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0) continue;
            floodReveal(x + i, y + j);
        }
    }
}

void Minesweeper::flagCell(int x, int y) {
    if (!isValidMove(x, y)) {
        return;
    }
    if(revealedGrid[x][y] == 'F'){
        revealedGrid[x][y] = '#';
    }
    else if(revealedGrid[x][y] == '#'){
        revealedGrid[x][y] = 'F';
    }
}

bool Minesweeper::checkWin(){
    for(int x=0; x<size; x++){
        for(int y=0; y<size; y++){
            if (!mineGrid[x][y] && revealedGrid[x][y] == '#') {
                return false;
            }
        }
    }
    return true;
}

void Minesweeper::revealMines(){
    for(int x=0; x<size; x++){
        for(int y=0; y<size; y++){
            if(mineGrid[x][y] && revealedGrid[x][y] != 'F'){
                revealedGrid[x][y] = 'M';
            }
        }
    }
}

void Minesweeper::gameOverMessage() {
    if (win) {
        std::cout << "\033[1;32m" << R"(
   ██╗   ██╗ ██████╗ ██╗   ██╗    ██╗    ██╗██╗███╗   ██╗██╗
   ╚██╗ ██╔╝██╔═══██╗██║   ██║    ██║    ██║██║████╗  ██║██║
    ╚████╔╝ ██║   ██║██║   ██║    ██║ █╗ ██║██║██╔██╗ ██║██║
     ╚██╔╝  ██║   ██║██║   ██║    ██║███╗██║██║██║╚██╗██║██║
      ██║   ╚██████╔╝╚██████╔╝    ╚███╔███╔╝██║██║ ╚████║██║
      ╚═╝    ╚═════╝  ╚═════╝      ╚══╝╚══╝ ╚═╝╚═╝  ╚═══╝╚═╝
)" << "\033[0m\n";
        double t = getElapsedTime();
        std::cout << "  Cleared in " << (int)t << " seconds!\n\n";
        saveHighScore(t);
        showHighScore();
    } else {
        std::cout << "\033[1;31m" << R"(
   ██████╗  █████╗ ███╗   ███╗███████╗     ██████╗ ██╗   ██╗███████╗██████╗
  ██╔════╝ ██╔══██╗████╗ ████║██╔════╝    ██╔═══██╗██║   ██║██╔════╝██╔══██╗
  ██║  ███╗███████║██╔████╔██║█████╗      ██║   ██║██║   ██║█████╗  ██████╔╝
  ██║   ██║██╔══██║██║╚██╔╝██║██╔══╝      ██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██╗
  ╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗    ╚██████╔╝ ╚████╔╝ ███████╗██║  ██║
   ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝     ╚═════╝   ╚═══╝  ╚══════╝╚═╝  ╚═╝
)" << "\033[0m\n";
    }

    std::cout << "\n  Press any key to continue...";
    std::cout.flush();
    char dummy;
    read(STDIN_FILENO, &dummy, 1);
}

void Minesweeper::getPlayerInput() {
    int x, y;
    char action;
    std::cout << "  Action (R=reveal, F=flag, Q=flee [-2x penalty]) row col: ";
    if (!(std::cin >> action)) { gameOver = true; return; }
    action = toUpper(action);
    if (action == 'Q') {
        g_minigameForfeited = true;
        gameOver = true;
        win = false;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }
    std::cin >> x >> y;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    makeMove(action, x, y);
}

void Minesweeper::makeMove(char action, int x, int y){
    if (!isValidMove(x, y)) return;

    if (action == 'R') {
        if(mineGrid[x][y]){
            gameOver = true;
            gameState=MINIGAME_LOSE;
            return;
        }
        if (revealedGrid[x][y] == '#') {
            if (solutionGrid[x][y] == 0){
                floodReveal(x, y);
            }
            else{
                revealSingleCell(x, y);
            }
        }
    } else if (action == 'F') {
        flagCell(x, y);
    }
}

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
        msLine("\xf0\x9f\x92\xa3 MINESWEEPER \xf0\x9f\x92\xa3", 16); 
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

double Minesweeper::getElapsedTime() {
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = now - startTime;
    return elapsed.count();
}

bool runMinesweeper(int gridSize) {
    int mineCount = (gridSize * gridSize) / 5;
    if (mineCount < 3) mineCount = 3;
    struct termios cooked;
    tcgetattr(STDIN_FILENO, &cooked);
    cooked.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
    srand((unsigned int)time(0));
    Minesweeper game(gridSize, mineCount);
    game.playGame();
    cooked.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
    return game.didWin();
}

Minesweeper::Minesweeper(int size, int mines) : size(size), mines(mines) {
    initializeGrids(size);
    placeMines();
    fillSolutionGrid();
    gameOver = false;
    win      = false;
    startTime = std::chrono::steady_clock::now();
}


