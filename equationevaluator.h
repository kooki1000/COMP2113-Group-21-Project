// =============================================================================
// equationevaluator.h
// TermiCraft — Expression Evaluator Module Header
//
// Declares the safe mathematical expression evaluator used by the 24 Game
// minigame.
//
// The evaluator converts infix expressions (with +, -, *, / and parentheses)
// into Reverse Polish Notation (RPN) using the Shunting-Yard algorithm, then
// evaluates the RPN using a stack. This design avoids dangerous eval() calls
// while supporting easy number-usage validation for game rules.
//
// Key Features:
//   - evaluate(): High-level function that returns the numeric result of
//     a valid expression.
//   - checkNumbersUsed(): Verifies the player used each required card value
//     exactly once (core 24 Game rule enforcement).
//   - Private helpers: isOperator(), shunting_yard(), evaluate_rpn().
//
// Integration: Used by the TwentyFour class during minigame input validation.
// Returns double result or throws on invalid expressions. Fully self-contained
// with no external dependencies.
//
// Author: Nan
// Dependencies: Standard headers only (<iostream>, <string>, <stack>, <cctype>, <cmath>)
// =============================================================================

#ifndef EQUATIONEVALUATOR_H
#define EQUATIONEVALUATOR_H
#include <map>
#include <string>
#include <vector>
class evaluator{
    public:
    double evaluate(const std::string& expression);
    bool checkNumbersUsed(const std::string expression, std::vector<int> numbers);
    private:
    std::map<char, int> precedence = {
            {'+', 1}, {'-', 1}, {'*', 2}, {'/', 2}
    };
    bool isOperator(char c);
    std::vector<std::string> shunting_yard(const std::string& expression);
    double evaluate_rpn(const std::vector<std::string>& rpn);
};
#endif
