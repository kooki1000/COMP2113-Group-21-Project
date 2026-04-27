#include <iostream>
#include <cctype>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <chrono>

enum class MinigameResult {
  MINIGAME_WIN = 0,
  MINIGAME_LOSE = 1,
  MINIGAME_ESCAPE = 2
};


class Minesweeper {
public:
    Minesweeper(int size, int mines);
    void initializeGrids(int size);
    void displayBoard();
    void clearScreen();
    bool playGame();
    void makeMove(char action, int x, int y);
    MinigameResult gameState; //lose, win, or escape

private:
    int size;
    int mines;
    std::vector<std::vector<bool>> mineGrid;           
    std::vector<std::vector<int>> solutionGrid;      
    std::vector<std::vector<char>> revealedGrid; 
    std::chrono::steady_clock::time_point startTime;
    bool gameOver;
    bool win;
    void placeMines();
    void fillSolutionGrid();
    bool isValidMove(int x, int y);
    int countAdjacentMines(int x, int y);
    void revealSingleCell(int x, int y);
    void floodReveal(int x, int y);
    void flagCell(int x, int y);
    bool checkWin();
    void revealMines();
    void gameOverMessage();
    void getPlayerInput();
    char toUpper(char c);
    void printInstructions();
    double getElapsedTime() const;
};

void writeHighScore(double time) {//creates high score file and writes to it
    std::string filename = "highscore.txt";
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
            std::cout << "New high score! Time: " << time << " seconds" << std::endl;
        }
    }
}
void Minesweeper::clearScreen() {
    std::system("clear");
}

void Minesweeper::printInstructions() {
    std::cout << "+----------------------------------------+\n";
    std::cout << "| Minesweeper Rules                      |\n";
    std::cout << "| R <row> <col>  : reveal cell          |\n";
    std::cout << "| F <row> <col>  : flag as mine         |\n";
    std::cout << "| Row/col in 0–" << (size - 1) << "          |\n";
    std::cout << "+----------------------------------------+\n\n";
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
    std::cout << "  ";              
    for (int i = 0; i < size; ++i) {
        std::cout << i << " ";
    }
    std::cout << std::endl;

    for (int i = 0; i < size; ++i) {
        std::cout << i << " ";      
        for (int j = 0; j < size; ++j) {
            if (revealedGrid[i][j] == 'F') {
                std::cout << "F ";
            } else if (revealedGrid[i][j] == 'M') {
                std::cout << "* ";
            } else if (revealedGrid[i][j] != '#') {
                std::cout << revealedGrid[i][j] << " ";
            } else {
                std::cout << ". ";
            }
        }
        std::cout << std::endl;
    }
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

void Minesweeper::gameOverMessage(){
    if (win == true) {
        std::cout << R"(
   ██╗   ██╗ ██████╗ ██╗   ██╗    ██╗    ██╗██╗███╗   ██╗██╗
   ╚██╗ ██╔╝██╔═══██╗██║   ██║    ██║    ██║██║████╗  ██║██║
    ╚████╔╝ ██║   ██║██║   ██║    ██║ █╗ ██║██║██╔██╗ ██║██║
     ╚██╔╝  ██║   ██║██║   ██║    ██║███╗██║██║██║╚██╗██║██║
      ██║   ╚██████╔╝╚██████╔╝    ╚███╔███╔╝██║██║ ╚████║██║
      ╚═╝    ╚═════╝  ╚═════╝      ╚══╝╚══╝ ╚═╝╚═╝  ╚═══╝╚═╝
        )"<< std::endl;
        double elapsedTime = getElapsedTime();
        std::cout << "Your time: " << elapsedTime << " seconds" << std::endl;
        writeHighScore(elapsedTime);
    } else {
        std::cout << R"(
   ██████╗  █████╗ ███╗   ███╗███████╗     ██████╗ ██╗   ██║███████╗██████╗ 
  ██╔════╝ ██╔══██╗████╗ ████║██╔════╝    ██╔═══██╗██║   ██║██╔════╝██╔══██╗
  ██║  ███╗███████║██╔████╔██║█████╗      ██║   ██║██║   ██║█████╗  ██████╔╝
  ██║   ██║██╔══██║██║╚██╔╝██║██╔══╝      ██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██╗
  ╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗    ╚██████╔╝ ╚████╔╝ ███████╗██║  ██║
   ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝     ╚═════╝   ╚═══╝  ╚══════╝╚═╝  ╚═╝
        )"<< std::endl;
    }
}

void Minesweeper::getPlayerInput(){
    int x, y;
    char action;

    std::cout << "Move (R/F row col) or enter 'Q 0 0' to quit: ";
    std::cin >> action >> x >> y;
    action = toUpper(action);
    if(action == 'Q'){
        gameState=MINIGAME_ESCAPE;
        gameOver = true;
    };
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

MinigameResult Minesweeper::playGame(){
    clearScreen();
    printInstructions();
    displayBoard();
    while (!gameOver) {
        clearScreen();
        printInstructions();
        displayBoard();
        if (checkWin()) {
            win = true;
            gameState = MINIGAME_WIN;
            gameOver = true;
        } else {
            getPlayerInput();
        }
    }
    clearScreen();
    revealMines();
    displayBoard();
    gameOverMessage();
    return gameState;
}

double Minesweeper::getElapsedTime() const {
    auto endTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;
    return elapsed.count()/1000.0;
}
bool runMinesweeper(int gridSize){
    Minesweeper game(gridSize, (gridSize*gridSize)/6);
    return game.playGame();
}

Minesweeper::Minesweeper(int s, int m) : size(s), mines(m) {
    srand(time(0));
    initializeGrids(size);
    placeMines();
    fillSolutionGrid();
    gameOver = false;
    win = false;
    gameState = MinigameResult::MINIGAME_LOSE;
    startTime = std::chrono::steady_clock::now();
}


