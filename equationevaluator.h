#ifndef EQUATIONEVALUATOR_H
#define EQUATIONEVALUATOR_H

#include <iostream>
#include <string>
#include <stack>
#include <cctype>
#include <cmath>

class evaluator{
    public:
    double evaluate(const std::string& expression);
    bool checkNumbersUsed(const std::string expression, std::vector<int> numbers);
    private:
    bool isOperator(char c);
    std::vector<std::string> shunting_yard(const std::string& expression);
    double evaluate_rpn(const std::vector<std::string>& rpn);
};

#endif
