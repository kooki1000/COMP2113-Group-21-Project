#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include "equationevaluator.h"

//parses puzzle numbers from puzzle bank
std::vector<int> parseNumbers(const std::string& numbersStr) {
    std::vector<int> result;
    
    // extract only digits and commas
    std::string clean;
    for (char c : numbersStr) {
        if (std::isdigit(c) || c == ',') {
            clean += c;
        }
    }
    
    //split by comma
    std::stringstream ss(clean);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (!token.empty()) {
            result.push_back(std::stoi(token));
        }
    }
    
    return result;
}

//load puzzles from csv file
std::vector<std::vector<int>> loadPuzzleNumbers(const std::string& filename) {
    std::vector<std::vector<int>> puzzles;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open " << filename << std::endl;
        return puzzles;
    }

    
    std::string line;
    bool firstLine = true;
    int lineCount = 0;
    int puzzleCount = 0;
    
    while (std::getline(file, line)) {
        lineCount++;
        
        if (firstLine) {
            firstLine = false;
            continue;
        }
        
        if (line.empty()) continue;
        
        //find the closing bracket
        size_t bracketPos = line.find(']');
        if (bracketPos == std::string::npos) continue;
        
        // extract numbers part (including the bracket)
        std::string numbersPart = line.substr(0, bracketPos + 1);
        
        // parse
        std::vector<int> numbers = parseNumbers(numbersPart);
        
        if (numbers.size() == 4) {
            puzzles.push_back(numbers);
            puzzleCount++;
        }
    }
    
    std::cout << "Loaded " << puzzles.size() << " puzzles from " << lineCount << " lines" << std::endl;
    return puzzles;
}

struct card {
    std::string face;
    int value;
    int suit;
};

class TwentyFour {
public:
    void playGame();
    
private:
    void printCards(const std::vector<card>& cards);
    std::vector<card> picked;
    std::vector<std::vector<int>> allPuzzles;
    bool evaluateInput(std::string expression);
    void pickCards();
};
//displays numbers in a way that resembles poker cards
void TwentyFour::printCards(const std::vector<card>& cards) {
    for (const auto& card : cards) std::cout << "┌─────────┐ ";
    std::cout << std::endl;
    
    for (const auto& card : cards) std::cout << "│" << card.face << "        │ ";
    std::cout << std::endl;
    
    for (const auto& card : cards) std::cout << "│         │ ";
    std::cout << std::endl;
    
    for (const auto& card : cards) {
        std::string suit_symbol;
        if (card.suit == 0) suit_symbol = "♥";
        else if (card.suit == 1) suit_symbol = "♦";
        else if (card.suit == 2) suit_symbol = "♣";
        else suit_symbol = "♠";
        std::cout << "│    " << suit_symbol << "    │ ";
    }
    std::cout << std::endl;
    
    for (const auto& card : cards) std::cout << "│         │ ";
    std::cout << std::endl;
    
    for (const auto& card : cards) std::cout << "│        " << card.face << "│ ";
    std::cout << std::endl;
    
    for (const auto& card : cards) std::cout << "└─────────┘ ";
    std::cout << std::endl;
}

//selects a random puzzle from all puzzles
void TwentyFour::pickCards() {
    if (allPuzzles.empty()) return;
    
    int puzzleNumber = rand() % allPuzzles.size();
    std::vector<int>& selectedNumbers = allPuzzles[puzzleNumber];
    picked.clear();
    
    for (int value : selectedNumbers) {
        card c;
        c.value = value;
        
        if (value == 1) c.face = "A";
        else if (value == 11) c.face = "J";
        else if (value == 12) c.face = "Q";
        else if (value == 13) c.face = "K";
        else c.face = std::to_string(value);
        
        c.suit = rand() % 4;
        picked.push_back(c);
    }
}

//Checks if input expression is equal to 24
bool TwentyFour::evaluateInput(std::string expression) {
    evaluator eval;
    try {
        double result = eval.evaluate(expression);
        return std::fabs(result - 24.0) < 1e-9;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return false;
    }
}

//Main game logic
void TwentyFour::playGame() {
    srand(static_cast<unsigned>(time(nullptr)));

    std::string filename = "twentyfourpuzzles.csv";
    bool loaded = false;
    allPuzzles = loadPuzzleNumbers(filename);
    pickCards();
    
    std::cout << "\n========== 24 GAME ==========\n";
    std::cout << "Your cards are:\n\n";
    printCards(picked);
    
    std::cout << "\nCard values: ";
    for (size_t i = 0; i < picked.size(); i++) {
        std::cout << picked[i].value;
        if (i < picked.size() - 1) std::cout << ", ";
    }
    std::cout << "\n\n";
    
    std::string input;
    std::cout << "Enter expression (use each card once, make 24): ";
    std::getline(std::cin, input);
    
    if (evaluateInput(input)) {
        std::cout << "\nCongratulations! You win!\n";
    } else {
        std::cout << "\nSorry, not correct. You LOSE!!!!!\n";
    }
}
