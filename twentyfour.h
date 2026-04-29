#ifndef TWENTYFOUR_H
#define TWENTYFOUR_H

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include "equationevaluator.h"
#include "types.h"

std::vector<std::vector<int>> loadPuzzleNumbers(const std::string& filename);
std::vector<int> parseNumbers(const std::string& numbersStr);
struct card{
    std::string face;
    int value;
    int suit;
};

class TwentyFour {
public:
    TwentyFour();
    MinigameResult playGame(int attempts, int timeLimit);
    
private:
    void printCards(const std::vector<card>& cards);
    bool evaluateInput(std::string expression);
    void pickCards();
    bool validateInput(std::string expression);
    bool checkNumbersUsed(const std::string& expression, std::vector<card> numbers);
};

#endif
