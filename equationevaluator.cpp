#include <stack>
#include <queue>
#include <map>
#include <sstream>
#include <cctype>
#include <cmath>

class evaluator{
    public:
    double evaluate(const std::string& expression);
    private:
    std::map<char, int> precedence = {
        {'+', 1}, {'-', 1}, {'*', 2}, {'/', 2}
    };
    bool isOperator(char c);
    std::vector<std::string> shunting_yard(const std::string& expression);
    double evaluate_rpn(const std::vector<std::string>& rpn);
};

bool evaluator::isOperator(char c) {
    return precedence.find(c) != precedence.end();
}

//converts mathematical expressions into RPN for easier processing
std::vector<std::string> evaluator::shunting_yard(const std::string& expression) {
    std::vector<std::string> output;
    std::stack<char> operators;
    std::string multidigit;
    for(int i = 0; i<expression.length(); i++){
        char c = expression[i];
        if(std::isdigit(c)){
            multidigit += c;
        }
        //handles expressions with paranthesis 
        else if(c=='('){
            if(!multidigit.empty()){
                output.push_back(multidigit);
                multidigit = "";
            }
            operators.push(c);
        }
        else if(c==')'){
            if (!multidigit.empty()) {
                output.push_back(multidigit);
                multidigit = "";
            }
            while(!operators.empty() && operators.top() != '('){
                output.push_back(std::string(1, operators.top()));
                operators.pop();
            }
            if(!operators.empty() && operators.top() == '('){
                operators.pop();
            }
        }
        else if (isOperator(c)){
            if (!multidigit.empty()) {
                output.push_back(multidigit);
                multidigit = "";
            }
            while (!operators.empty() && operators.top() != '(' && precedence[operators.top()] >= precedence[c]) {
                output.push_back(std::string(1, operators.top()));
                operators.pop();
            }
            operators.push(c);
        }
    }
    if (!multidigit.empty()) {
        output.push_back(multidigit);
    }
    while (!operators.empty()) {
            output.push_back(std::string(1, operators.top()));
            operators.pop();
    }

    return output;
}

double evaluator::evaluate_rpn(const std::vector<std::string>& rpn) {
    std::stack<double> values;
    for (const std::string& token : rpn) {
        if (std::isdigit(token[0])) {
            values.push(std::stod(token));
        }
        //removes and calculates pairs of values in order of operation 
        else if (isOperator(token[0]) && token.length() == 1) {
            double b = values.top(); 
            values.pop();
            double a = values.top(); 
            values.pop();
            switch (token[0]) {
                case '+': values.push(a + b); break;
                case '-': values.push(a - b); break;
                case '*': values.push(a * b); break;
                case '/': values.push(a / b); break;
            }
        }
    }
return values.top();
}

double evaluator::evaluate(const std::string& expression) {
    std::vector<std::string> rpn = shunting_yard(expression);
    return evaluate_rpn(rpn);
}