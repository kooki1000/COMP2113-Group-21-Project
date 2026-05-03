// =============================================================================
// equationevaluator.cpp
// TermiCraft — Expression Evaluator Module Implementation
//
// Full implementation of a safe mathematical expression evaluator for the
// 24 Game minigame and other TermiCraft progression systems.
//
// Uses the Shunting-Yard algorithm to convert infix expressions (with + - * /
// and parentheses) into Reverse Polish Notation (RPN), then evaluates the RPN
// using a stack. This approach avoids direct eval() risks and allows easy
// validation of number usage.
//
// Features:
//   - shunting_yard(): Converts infix to RPN while respecting operator
//     precedence and parentheses.
//   - evaluate_rpn(): Evaluates the RPN expression using a value stack.
//   - evaluate(): High-level function that combines the above for a given
//     string expression.
//   - checkNumbersUsed(): Verifies the player used each required card value
//     exactly once (critical for 24 Game rules).
//
// Integration: Used by the TwentyFour class to validate and compute player
// input during the minigame. Returns double result or throws on invalid
// expressions. Designed to be lightweight with no external dependencies.
//
// Author: Nan
// Dependencies: evaluator.h
// Standard headers only (<stack>, <queue>, <map>, <sstream>, <cctype>, <cmath>)
// =============================================================================

#include "equationevaluator.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <stack>
#include <stdexcept>

//Checks if a char is an operator
bool evaluator::isOperator(char c) {
    return precedence.find(c) != precedence.end();
}

//Converts the user input expression into reverse polish notation (rpn) for easy calculation
std::vector<std::string> evaluator::shunting_yard(const std::string& expression) {
    std::vector<std::string> output;
    std::stack<char> operators;
    std::string number;

    for (size_t i = 0; i < expression.length(); i++) {
        char c = expression[i];

        if (std::isspace(static_cast<unsigned char>(c))) {
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            number += c;
            continue;
        }

        if (!number.empty()) {
            output.push_back(number);
            number.clear();
        }

        if (c == '(') {
            operators.push(c);
        } else if (c == ')') {
            while (!operators.empty() && operators.top() != '(') {
                output.push_back(std::string(1, operators.top()));
                operators.pop();
            }

            if (operators.empty()) {
                throw std::runtime_error("Mismatched parentheses");
            }

            operators.pop();
        } else if (isOperator(c)) {
            while (!operators.empty() && operators.top() != '(' &&
                   precedence[operators.top()] >= precedence[c]) {
                output.push_back(std::string(1, operators.top()));
                operators.pop();
            }

            operators.push(c);
        } else {
            throw std::runtime_error("Invalid character");
        }
    }

    if (!number.empty()) {
        output.push_back(number);
    }

    while (!operators.empty()) {
        if (operators.top() == '(') {
            throw std::runtime_error("Mismatched parentheses");
        }

        output.push_back(std::string(1, operators.top()));
        operators.pop();
    }

    return output;
}

//Evaluates the rpn expression using a stack 
double evaluator::evaluate_rpn(const std::vector<std::string>& rpn) {
    std::stack<double> values;

    for (const std::string& token : rpn) {
        if (!token.empty() && std::isdigit(static_cast<unsigned char>(token[0]))) {
            values.push(std::stod(token));
        } else if (token.length() == 1 && isOperator(token[0])) {
            if (values.size() < 2) {
                throw std::runtime_error("Invalid expression");
            }

            double b = values.top();
            values.pop();

            double a = values.top();
            values.pop();

            switch (token[0]) {
            case '+':
                values.push(a + b);
                break;
            case '-':
                values.push(a - b);
                break;
            case '*':
                values.push(a * b);
                break;
            case '/':
                if (std::fabs(b) < 1e-9) {
                    throw std::runtime_error("Division by zero");
                }
                values.push(a / b);
                break;
            }
        }
    }

    if (values.size() != 1) {
        throw std::runtime_error("Invalid expression");
    }

    return values.top();
}

//Combines the evaluation algorithm and the conversion algorithm
double evaluator::evaluate(const std::string& expression) {
    std::vector<std::string> rpn = shunting_yard(expression);
    return evaluate_rpn(rpn);
}

//Checks if all the numbers in the puzzle are used
bool evaluator::checkNumbersUsed(const std::string& expression, std::vector<int> numbers) {
    std::vector<std::string> rpn = shunting_yard(expression);
    std::vector<int> usedNumbers;

    for (const std::string& token : rpn) {
        if (!token.empty() && std::isdigit(static_cast<unsigned char>(token[0]))) {
            usedNumbers.push_back(std::stoi(token));
        }
    }

    std::sort(usedNumbers.begin(), usedNumbers.end());
    std::sort(numbers.begin(), numbers.end());

    return usedNumbers == numbers;
}
