#include <iostream>
#include <cctype>
#include <vector>

const int easygrid = 5;
const int mediumgrid = 10;
const int hardgrid = 15;
const int easymine = 5;
const int mediummine = 10;
const int hardmine = 20;

class Minesweeper {
public:
    Minesweeper(int size, int mines);
    void displayBoard();
    void clearScreen();

private:
    void initializeGrids(int size);
    int size;
    int mines;
    std::vector<std::vector<bool>> mineGrid;           
    std::vector<std::vector<int>> solutionGrid;      
    std::vector<std::vector<char>> revealedGrid; 
    bool gameOver;
    bool win;
};

void clearScreen(){
    for(int i=0; i<100; i++){
        cout << endl;
    }
}

char toUpper(char c){
    return toupper(c);
}

bool isValidMove(int x, int y, int gridSize) {
    if (x < 0 || x >= gridSize || y < 0 || y >= gridSize) {
        return false;
    }
    return true;
}

int countAdjacentMines(int x, int y, const std::vector<std::vector<int>>& board) { 
    int count = 0;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            if (isValidMove(x + i, y + j, board.size()) && board[x + i][y + j] == -1) {
                count++;
            }
        }
    }
    return count;
}

void displayBoard(const std::vector<std::vector<int>>& grid, const std::vector<std::vector<bool>>& revealed) {
    cout << "   ";
    for (int i = 0; i < grid.size(); ++i) {
        cout << i << " ";
    }
    cout << endl;

    for (int i = 0; i < grid.size(); ++i) {
        cout << i << " ";
        for (int j = 0; j < grid[i].size(); ++j) {
            if (revealed[i][j]) {
                if (grid[i][j] == -1) {
                    cout << "* ";
                } else {
                    cout << countAdjacentMines(i, j, grid) << " ";
                }
            } else {
                cout << ". ";
            }
        }
        cout << endl;
    }
}

void initializeGrids(int size){
    mineGrid.assign(size, std::vector<bool>(size, false));  
    solutionGrid.assign(size, std::vector<int>(size, 0)); 
    revealedGrid.assign(size, std::vector<char>(size, '#')); 
}

void placeMines(){
    int placedMines = 0;
    while (placedMines < mines) {
        int x = rand() % size;
        int y = rand() % size;
        if (!mineGrid[x][y]) {
            mineGrid[x][y] = true;
            placedMines++;
        }
    }
}

void fillSolutionGriod(){
    for(int x=0; x<size; x++){
        for(int y=0; y<size; y++){
            if(mineGrid[x][y]){
                solutionGrid[x][y] = -1;
            } else {
                solutionGrid[x][y] = countAdjacentMines(x, y, solutionGrid);
            }
        }
    }
}

void revealSingleCell(int x, int y){
    if (!isValidMove(x, y, size)) {
        return;
    }
    if(mineGrid[x][y]){
        gameOver = true;
        return;
    }
    revealedGrid[x][y] = solutionGrid[x][y];
    return;
}

void floodReveal(int x, int y){
    if (!isValidMove(x, y, size) || revealedGrid[x][y] != '#') {
        return;
    }
    revealedGrid[x][y] = solutionGrid[x][y];
    if (solutionGrid[x][y] == 0) {
        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                floodReveal(x + i, y + j);
            }
        }
    }
}

void flagCell(int x, int y) {
    if (!isValidMove(x, y, size)) {
        return;
    }
    if(revealedGrid[x][y] == 'F'){
        revealedGrid[x][y] = '#';
    }
    else if(revealedGrid[x][y] == '#'){
        revealedGrid[x][y] = 'F';
    }

}

bool checkWin(){
    for(int x=0; x<size; x++){
        for(int y=0; y<size; y++){
            if(mineGrid[x][y] && revealedGrid[x][y] != 'F'){
                return false;
            }
            if(!mineGrid[x][y] && revealedGrid[x][y] == 'F'){
                return false;
            }
        }
    }
    return true;
}

void revealMines(){
    for(int x=0; x<size; x++){
        for(int y=0; y<size; y++){
            if(mineGrid[x][y]){
                revealedGrid[x][y] = 'M';
            }
        }
    }
}

void gameOverMessage(){
    if (win == true) {
        std::cout << cout << R"(
   ██╗   ██╗ ██████╗ ██╗   ██╗    ██╗    ██╗██╗███╗   ██╗██╗
   ╚██╗ ██╔╝██╔═══██╗██║   ██║    ██║    ██║██║████╗  ██║██║
    ╚████╔╝ ██║   ██║██║   ██║    ██║ █╗ ██║██║██╔██╗ ██║██║
     ╚██╔╝  ██║   ██║██║   ██║    ██║███╗██║██║██║╚██╗██║██║
      ██║   ╚██████╔╝╚██████╔╝    ╚███╔███╔╝██║██║ ╚████║██║
      ╚═╝    ╚═════╝  ╚═════╝      ╚══╝╚══╝ ╚═╝╚═╝  ╚═══╝╚═╝
        )"<< std::endl;
    } else {
        std::cout << R"(
   ██████╗  █████╗ ███╗   ███╗███████╗     ██████╗ ██╗   ██╗███████╗██████╗ 
  ██╔════╝ ██╔══██╗████╗ ████║██╔════╝    ██╔═══██╗██║   ██║██╔════╝██╔══██╗
  ██║  ███╗███████║██╔████╔██║█████╗      ██║   ██║██║   ██║█████╗  ██████╔╝
  ██║   ██║██╔══██║██║╚██╔╝██║██╔══╝      ██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██╗
  ╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗    ╚██████╔╝ ╚████╔╝ ███████╗██║  ██║
   ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝     ╚═════╝   ╚═══╝  ╚══════╝╚═╝  ╚═╝
        )"<< std::endl;
    }
}

