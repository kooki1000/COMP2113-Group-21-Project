// =============================================================================
// twentyfour.h
// TermiCraft — 24 Game Minigame Module Header
//
// Declares the 24 Game minigame triggered during mining, specifically during
//  material progression in TermiCraft. 
//
// The player receives four cards and must use each value exactly once with
// +, -, *, / and parentheses to reach exactly 24. Supports multiple
// difficulty modes via attempts and timeLimit parameters.
//
// Key Features:
//   - playGame(): Main entry point — loads puzzles, displays cards, runs
//     timed input loop, validates expressions, and returns MinigameResult.
//   - loadPuzzleNumbers() + parseNumbers(): Load and parse puzzle sets from
//     twentyfourpuzzles.csv.
//   - printCards(): ASCII poker-card style display.
//   - evaluateInput(), validateInput(), checkNumbersUsed(): Safe expression
//     checking using the EquationEvaluator.
//
// Integration: Called via runTwentyFour() from the main game loop. Success
// grants the tier upgrade; failure applies resource penalty.
//
// Author: Nan
// Dependencies: equationevaluator.h, types.h
// Standard headers only.
// =============================================================================
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
