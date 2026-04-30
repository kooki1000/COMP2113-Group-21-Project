#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include "equationevaluator.h"
#include "types.h"

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
    
    return puzzles;
}

struct card {
    std::string face;
    int value;
    int suit;
};

class TwentyFour {
public:
    TwentyFour();
    MinigameResult playGame(int attempts, int timeLimit);
    int gamestate;
    
private:
    void printCards(const std::vector<card>& cards);
    std::vector<card> picked;
    std::vector<std::vector<int>> allPuzzles;
    bool evaluateInput(std::string expression);
    void pickCards();
    bool validateInput(std::string expression);
    bool checkNumbersUsed(const std::string& expression, std::vector<card> numbers);
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

bool TwentyFour::validateInput(std::string expression) {
    if (expression.empty()) {
        std::cout << "Error: Please enter an expression" << std::endl;
        return false;
    }
    std::string validChars = "0123456789+-*/() .";
    for (char c : expression) {
        if (validChars.find(c) == std::string::npos) {
            std::cout << "Error: Invalid character in expression" << std::endl;
            std::cout << "Only allowed: numbers, +, -, *, /, (, ), and spaces" << std::endl;
            return false;
        }
    }
    evaluator eval;
    try {
        double result = eval.evaluate(expression);
        return true;
    }
    catch (const std::exception& e){
        return false;
    }
}

bool TwentyFour::checkNumbersUsed(const std::string& expression, std::vector<card> cards) {
    evaluator eval;
    std::vector<int> cardValues;
    for (const auto& card : cards) {
        cardValues.push_back(card.value);
    }
    return eval.checkNumbersUsed(expression, cardValues);
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
MinigameResult TwentyFour::playGame(int attempts, int timeLimit) {
    srand(static_cast<unsigned>(time(nullptr)));
    std::string filename = "twentyfourpuzzles.csv";
    allPuzzles = loadPuzzleNumbers(filename);
    pickCards();
    
    std::cout << "\n";
    std::cout << " ╔══════════════════════════════════════════════════════╗\n";
    std::cout << " ║                                                      ║\n";
    std::cout << " ║              🃏  THE 24 GAME  🃏                     ║\n";
    std::cout << " ║                                                      ║\n";
    std::cout << " ║    Use +, -, *, / and parentheses to make 24        ║\n";
    std::cout << " ║    Use each card value exactly once                  ║\n";
    std::cout << " ║                                                      ║\n";
    std::cout << " ║    Example: (6-3)×4×2 = 24                          ║\n";
    std::cout << " ║                                                      ║\n";
    std::cout << " ╚══════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << " Your cards are:\n\n";
    printCards(picked);
    
    std::cout << "\nCard values: ";
    for (size_t i = 0; i < picked.size(); i++) {
        std::cout << picked[i].value;
        if (i < picked.size() - 1) std::cout << ", ";
    }
    auto startTime = std::chrono::steady_clock::now();
    std::cout << "\n ⏰ You have " <<timeLimit << " seconds to submit your answer!\n";
    std::cout << "\n\n";
    while(true){
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - startTime).count();
        
        if (elapsed >= timeLimit) {
            std::cout << "\n Time's up!\n";
            gamestate=-1;
            break;
        }
        if(attempts <= 0){
            gamestate=0;
            break;
        }
        std::string input;
        std::cout << "You have "<<attempts<<" attempts left. ";
        std::cout << "Enter expression (use each card once, make 24): ";
        std::getline(std::cin, input);
        if(input == "q" || input == "Q"){
            return MINIGAME_ESCAPE;
        }
        if(!validateInput(input)){
            std::cout << "Please enter a valid expression: "<<std::endl;
            attempts--;
        }
        else if(!checkNumbersUsed(input, picked)){
            std::cout << "Please use each card value exactly once: "<<std::endl;
            attempts--;
        }
        else if(evaluateInput(input)){
            gamestate=1;
            break;
        }
        else{
            std::cout << "Incorrect. Try again.\n";
            attempts--;
        }
    }

    if (gamestate == 1) {
        std::cout << "\n";
        std::cout << " ╔══════════════════════════════════════════════════╗\n";
        std::cout << " ║                                                   ║\n";
        std::cout << " ║                                                   ║\n";
        std::cout << " ║     🎉🎊✨  CONGRATULATIONS! YOU WIN!  ✨🎊🎉      ║\n";
        std::cout << " ║                                                   ║\n";
        std::cout << " ║                                                   ║\n";
        std::cout << " ╚══════════════════════════════════════════════════╝\n";
        std::cout << "\n";
        return MINIGAME_WIN;
    } else if (gamestate==0) {
        std::cout << "\n";
        std::cout << "  ╔══════════════════════════════════════════════════╗\n";
        std::cout << "  ║                                                  ║\n";
        std::cout << "  ║     💀  YOU LOSE!  💀                            ║\n";
        std::cout << "  ║                                                  ║\n";
        std::cout << "  ║     You ran out of attempts.                     ║\n";
        std::cout << "  ║                                                  ║\n";
        std::cout << "  ╚══════════════════════════════════════════════════╝\n";
        std::cout << "\n";
        return MINIGAME_LOSE;
    }
    else if (gamestate==-1){
        std::cout << "\n";
        std::cout << "  ╔══════════════════════════════════════════════════╗\n";
        std::cout << "  ║                                                  ║\n";
        std::cout << "  ║     💀  YOU LOSE!  💀                            ║\n";
        std::cout << "  ║                                                  ║\n";
        std::cout << "  ║     You failed to solve the puzzle in time.     ║\n";
        std::cout << "  ║                                                  ║\n";
        std::cout << "  ╚══════════════════════════════════════════════════╝\n";
        std::cout << "\n";
        return MINIGAME_LOSE;
    }
    return MINIGAME_LOSE; 
}
//easy mode: 5 attempts, 180 seconds 
//medium mode: 3 attempts, 90 seconds
//hard mode: 1 attempt, 30 seconds
MinigameResult runTwentyFour(int attempts, int timelimit) {
    TwentyFour game;
    return game.playGame(attempts, timelimit);
}

TwentyFour::TwentyFour() {
    gamestate = 0;
}
