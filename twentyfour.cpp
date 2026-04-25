#include <iostream>
#include <vector>
#include <string>

struct card{
    string face;
    int value;
    int suit;
};

class TwentyFour{
    public:
        void play();
    private:
        void printCards(card c);
        vector<card> deck;
        vector<card> picked;
};


void TwentyFour::printCards(card c){
    string suit_symbol;
    if (c.suit == 0) suit_symbol = "♥";
    else if (c.suit == 1) suit_symbol = "♦";
    else if (c.suit == 2) suit_symbol = "♣";
    else suit_symbol = "♠";
    cout << "┌─────────┐\n";
    cout << "│" << c.value << "       │\n";
    cout << "│         │\n";
    cout << "│    " << suit_symbol << "    │\n";
    cout << "│         │\n";
    cout << "│       " << c.face << "│\n";
    cout << "└─────────┘\n";
}

void TwentyFour::generateCards(){
    vector<card> deck;
    string faces[] = {"A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"};
    for (int i = 0; i < 13; i++) {
        deck.push_back({faces[i], i + 1, 0});
        deck.push_back({faces[i], i + 1, 1});
        deck.push_back({faces[i], i + 1, 2});
        deck.push_back({faces[i], i + 1, 3});
    }
    return deck;
}

void TwentyFour::pickCards(){
    for(int i = 0; i<4; i++){
        int x = rand() % (52-i);
        picked.push_back(deck[x]);
        deck.erase(deck.begin() + x);
    }
}

vector<vector<string>> allcombinations(vector<card> picked) {
    vector<vector<string>> result;
    // Generate all combinations of the picked cards
    for (int i = 0; i < picked.size(); i++) {
        for (int j = i + 1; j < picked.size(); j++) {
            result.push_back({picked[i].face, picked[j].face});
        }
    }
    return result;
}

bool has_solution(vector<card> cards){
    if (cards.size()==1){
        
    }
}
