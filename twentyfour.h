#ifndef TWENTYFOUR_H
#define TWENTYFOUR_H

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include "equationevaluator.h"

std::vector<std::vector<std::string>> readCSV(const std::string& filename);
struct card{
    std::string face;
};

class TwentyFour{
    public:
        void playGame();
    private:
        void printCards(card c);
        bool evaluateInput(std::string expression);
        void pickCards();
};

#endif