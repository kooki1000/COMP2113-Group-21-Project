#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

std::vector<std::vector<std::string>> readCSV(const std::string& filename) {
    std::vector<std::vector<std::string>> data;
    std::ifstream file(filename);
    std::string line;
    
    while (std::getline(file, line)) {
        std::vector<std::string> row;
        std::stringstream ss(line);
        std::string cell;
        
        while (std::getline(ss, cell, ',')) {
            row.push_back(cell);
        }
        data.push_back(row);
    }
    
    return data;
}

struct card{
    std::string face;
    int value;
    int suit;
};

class TwentyFour{
    public:
        void play();
    private:
        void printCards(card c);
        std::vector<card> deck;
        std::vector<card> picked;
        std::vector<std::vector<std::string>> allpuzzles;
        void pickCards();
};


void TwentyFour::printCards(card c){
    std::string suit_symbol;
    if (c.suit == 0) suit_symbol = "♥";
    else if (c.suit == 1) suit_symbol = "♦";
    else if (c.suit == 2) suit_symbol = "♣";
    else suit_symbol = "♠";
    std::cout << "┌─────────┐\n";
    std::cout << "│" << c.value << "       │\n";
    std::cout << "│         │\n";
    std::cout << "│    " << suit_symbol << "    │\n";
    std::cout << "│         │\n";
    std::cout << "│       " << c.face << "│\n";
    std::cout << "└─────────┘\n";
}


void TwentyFour::pickCards(){
    int puzzleNumber = rand()%1363;
    std::string puzzle = allpuzzles[puzzleNumber][0].substr(1, allpuzzles[puzzleNumber][0].size() - 2);
    std::stringstream ss(puzzle);
    std::string value;
    while (std::getline(ss, value, ',')) {
        card c;
        if(stoi(value)==11){
            c.face = "J";
        }
        else if(stoi(value)==12){
            c.face = "Q";
        }
        else if(stoi(value)==13){
            c.face = "K";
        }
        else if(stoi(value)==1){
            c.face = "A";
        }
        else{
            c.face = value;
        }
        c.value = stoi(value);
        c.suit = rand() % 4;
        picked.push_back(c);
    }
}

bool evaluateInput(std::string expression){
    
}
