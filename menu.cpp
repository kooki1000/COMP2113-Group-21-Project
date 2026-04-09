/*
 * menu.cpp
 * 
 * All the UI screens and menus.
 */

#include "menu.h"
#include "colors.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdio>
#include <termios.h>
#include <unistd.h>

using namespace std;

// ----- INPUT HELPERS -----

// Read a single keypress without waiting for Enter
static char getch() {
    struct termios oldattr, newattr;
    char ch;
    tcgetattr(STDIN_FILENO, &oldattr);
    newattr = oldattr;
    newattr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
    return ch;
}

// ----- ASCII ART -----

void showTitleArt() {
    cout << COLOR_BOLD_CYAN;
    cout << R"(
    ████████╗███████╗██████╗ ███╗   ███╗██╗ ██████╗██████╗  █████╗ ███████╗████████╗
    ╚══██╔══╝██╔════╝██╔══██╗████╗ ████║██║██╔════╝██╔══██╗██╔══██╗██╔════╝╚══██╔══╝
       ██║   █████╗  ██████╔╝██╔████╔██║██║██║     ██████╔╝███████║█████╗     ██║   
       ██║   ██╔══╝  ██╔══██╗██║╚██╔╝██║██║██║     ██╔══██╗██╔══██║██╔══╝     ██║   
       ██║   ███████╗██║  ██║██║ ╚═╝ ██║██║╚██████╗██║  ██║██║  ██║██║        ██║   
       ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝╚═╝ ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝        ╚═╝   
)" << COLOR_RESET;
    
    cout << COLOR_YELLOW;
    cout << R"(
                         ⛏️   M I N E .  C R A F T .  S U R V I V E .  ⚔️
)" << COLOR_RESET;
    
    cout << COLOR_DIM;
    cout << "                    ·  ˚  ✦  ·  ˚    ⋆    ·  ✦  ˚  ·  ✦  ·  ˚  ⋆  ·\n";
    cout << COLOR_RESET;
}

void showVictoryArt() {
    cout << COLOR_BOLD_YELLOW;
    cout << R"(
    ██╗   ██╗██╗ ██████╗████████╗ ██████╗ ██████╗ ██╗   ██╗██╗
    ██║   ██║██║██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗╚██╗ ██╔╝██║
    ██║   ██║██║██║        ██║   ██║   ██║██████╔╝ ╚████╔╝ ██║
    ╚██╗ ██╔╝██║██║        ██║   ██║   ██║██╔══██╗  ╚██╔╝  ╚═╝
     ╚████╔╝ ██║╚██████╗   ██║   ╚██████╔╝██║  ██║   ██║   ██╗
      ╚═══╝  ╚═╝ ╚═════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚═╝
)" << COLOR_RESET;

    cout << COLOR_GREEN;
    cout << R"(
                           🐉  THE DRAGON HAS BEEN SLAIN!  🐉
                      
                               ╔═══════════════════╗
                               ║   👑 LEGENDARY!   ║
                               ╚═══════════════════╝
)" << COLOR_RESET;
}

void showDefeatArt() {
    cout << COLOR_BOLD_RED;
    cout << R"(
     ██████╗  █████╗ ███╗   ███╗███████╗     ██████╗ ██╗   ██╗███████╗██████╗ 
    ██╔════╝ ██╔══██╗████╗ ████║██╔════╝    ██╔═══██╗██║   ██║██╔════╝██╔══██╗
    ██║  ███╗███████║██╔████╔██║█████╗      ██║   ██║██║   ██║█████╗  ██████╔╝
    ██║   ██║██╔══██║██║╚██╔╝██║██╔══╝      ██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██╗
    ╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗    ╚██████╔╝ ╚████╔╝ ███████╗██║  ██║
     ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝     ╚═════╝   ╚═══╝  ╚══════╝╚═╝  ╚═╝
)" << COLOR_RESET;

    cout << COLOR_DIM;
    cout << R"(
                              💀  You have perished...  💀
                           
                           The mines claim another soul.
)" << COLOR_RESET;
}

// ----- BOX DRAWING -----

void showBox(const vector<string>& lines, int width) {
    cout << COLOR_CYAN;
    cout << "    ╔";
    for (int i = 0; i < width; i++) cout << "═";
    cout << "╗\n";
    
    for (const string& line : lines) {
        cout << "    ║" << COLOR_WHITE;
        int padding = width - line.length();
        int leftPad = padding / 2;
        int rightPad = padding - leftPad;
        for (int i = 0; i < leftPad; i++) cout << " ";
        cout << line;
        for (int i = 0; i < rightPad; i++) cout << " ";
        cout << COLOR_CYAN << "║\n";
    }
    
    cout << "    ╚";
    for (int i = 0; i < width; i++) cout << "═";
    cout << "╝\n";
    cout << COLOR_RESET;
}

// ----- MAIN MENU -----

int showMainMenu(const HighScore& highScore) {
    while (true) {
        clearScreen();
        showTitleArt();
        
        cout << "\n";
        
        vector<string> menuItems = {
            "",
            "[1]  🎮  NEW GAME",
            "",
            "[2]  💾  LOAD GAME",
            "",
            "[3]  🏆  HIGH SCORES",
            "",
            "[4]  📖  HOW TO PLAY",
            "",
            "[5]  🚪  QUIT",
            ""
        };
        showBox(menuItems, 40);
        
        // Show current high score
        cout << "\n";
        cout << COLOR_YELLOW << "                    🏆 HIGH SCORE: " << COLOR_BOLD_YELLOW;
        cout << highScore.score;
        cout << COLOR_YELLOW << " (" << getDifficultyColor(highScore.difficulty);
        switch(highScore.difficulty) {
            case DIFF_EASY: cout << "EASY"; break;
            case DIFF_NORMAL: cout << "NORMAL"; break;
            case DIFF_HARD: cout << "HARD"; break;
        }
        cout << COLOR_YELLOW << ")" << COLOR_RESET << "\n";
        
        if (highScore.defeatedDragon) {
            cout << COLOR_GREEN << "                    🐉 Dragon Slayer: " << highScore.playerName << COLOR_RESET << "\n";
        }
        
        cout << "\n" << COLOR_DIM << "                         Enter your choice [1-5]: " << COLOR_RESET;
        
        char choice = getch();
        if (choice >= '1' && choice <= '5') {
            return choice - '0';
        }
    }
}

// ----- DIFFICULTY SELECTION -----

Difficulty selectDifficulty() {
    int selected = 1;  // start on Normal
    
    while (true) {
        clearScreen();
        
        cout << COLOR_BOLD_WHITE << "\n\n";
        cout << "                    ╔═══════════════════════════════════╗\n";
        cout << "                    ║       SELECT DIFFICULTY           ║\n";
        cout << "                    ╚═══════════════════════════════════╝\n\n";
        cout << COLOR_RESET;
        
        // Easy
        if (selected == 0) {
            cout << COLOR_GREEN << "              ▶  ";
        } else {
            cout << COLOR_DIM << "                 ";
        }
        cout << "[1] EASY" << COLOR_RESET << "\n";
        cout << COLOR_DIM << "                     • 150 HP  • More ores  • Fewer enemies\n";
        cout << "                     • 4-letter Wordle  • 6x6 Minesweeper\n";
        cout << "                     • Score: 1.0x multiplier\n\n" << COLOR_RESET;
        
        // Normal
        if (selected == 1) {
            cout << COLOR_YELLOW << "              ▶  ";
        } else {
            cout << COLOR_DIM << "                 ";
        }
        cout << "[2] NORMAL" << COLOR_RESET << "\n";
        cout << COLOR_DIM << "                     • 100 HP  • Standard ores  • Standard enemies\n";
        cout << "                     • 5-letter Wordle  • 8x8 Minesweeper\n";
        cout << "                     • Score: 1.5x multiplier\n\n" << COLOR_RESET;
        
        // Hard
        if (selected == 2) {
            cout << COLOR_RED << "              ▶  ";
        } else {
            cout << COLOR_DIM << "                 ";
        }
        cout << "[3] HARD" << COLOR_RESET << "\n";
        cout << COLOR_DIM << "                     • 75 HP  • Scarce ores  • Many enemies\n";
        cout << "                     • 6-letter Wordle  • 10x10 Minesweeper\n";
        cout << "                     • Score: 2.0x multiplier\n\n" << COLOR_RESET;
        
        cout << "\n";
        cout << COLOR_DIM << "              Use [W/S] or [1-3] to select, [ENTER] to confirm\n";
        cout << "              [Q] to go back\n" << COLOR_RESET;
        
        char input = getch();
        
        switch(input) {
            case 'w': case 'W':
                selected = (selected - 1 + 3) % 3;
                break;
            case 's': case 'S':
                selected = (selected + 1) % 3;
                break;
            case '1':
                selected = 0;
                break;
            case '2':
                selected = 1;
                break;
            case '3':
                selected = 2;
                break;
            case '\n': case '\r': case ' ':
                return static_cast<Difficulty>(selected);
            case 'q': case 'Q':
                return DIFF_NORMAL;
        }
    }
}

// ----- HIGH SCORES -----

void showHighScores(const vector<HighScore>& scores) {
    clearScreen();
    
    cout << COLOR_BOLD_YELLOW << "\n\n";
    cout << "                    ╔═══════════════════════════════════════════╗\n";
    cout << "                    ║            🏆 HIGH SCORES 🏆              ║\n";
    cout << "                    ╚═══════════════════════════════════════════╝\n\n";
    cout << COLOR_RESET;
    
    cout << COLOR_CYAN;
    cout << "         ┌──────┬────────────────┬──────────┬────────────┬────────┐\n";
    cout << "         │ RANK │     NAME       │  SCORE   │ DIFFICULTY │ DRAGON │\n";
    cout << "         ├──────┼────────────────┼──────────┼────────────┼────────┤\n";
    cout << COLOR_RESET;
    
    for (int i = 0; i < 10; i++) {
        cout << "         │";
        
        // Rank with medals for top 3
        if (i < 3) {
            const char* medals[] = {"🥇", "🥈", "🥉"};
            cout << "  " << medals[i] << "  │";
        } else {
            cout << "  " << setw(2) << (i + 1) << "  │";
        }
        
        if (i < (int)scores.size()) {
            const HighScore& hs = scores[i];
            
            cout << " " << setw(14) << left << hs.playerName.substr(0, 14) << " │";
            cout << COLOR_YELLOW << " " << setw(8) << right << hs.score << COLOR_RESET << " │";
            
            cout << getDifficultyColor(hs.difficulty);
            string diffStr;
            switch(hs.difficulty) {
                case DIFF_EASY: diffStr = "Easy"; break;
                case DIFF_NORMAL: diffStr = "Normal"; break;
                case DIFF_HARD: diffStr = "Hard"; break;
            }
            cout << " " << setw(10) << diffStr << COLOR_RESET << " │";
            
            if (hs.defeatedDragon) {
                cout << "   🐉   │";
            } else {
                cout << "   -    │";
            }
        } else {
            cout << "      ---      │    ---   │    ---     │   -    │";
        }
        cout << "\n";
    }
    
    cout << COLOR_CYAN;
    cout << "         └──────┴────────────────┴──────────┴────────────┴────────┘\n";
    cout << COLOR_RESET;
    
    cout << "\n" << COLOR_DIM << "                    Press any key to return to menu..." << COLOR_RESET;
    getch();
}

// ----- HOW TO PLAY -----

void showHowToPlay() {
    clearScreen();
    
    cout << COLOR_BOLD_WHITE << "\n";
    cout << "    ╔═══════════════════════════════════════════════════════════════════╗\n";
    cout << "    ║                      📖 HOW TO PLAY 📖                            ║\n";
    cout << "    ╚═══════════════════════════════════════════════════════════════════╝\n\n";
    cout << COLOR_RESET;
    
    cout << COLOR_BOLD_CYAN << "    OBJECTIVE:\n" << COLOR_RESET;
    cout << "    Mine resources, craft better equipment, and defeat the Dragon!\n\n";
    
    cout << COLOR_BOLD_CYAN << "    CONTROLS:\n" << COLOR_RESET;
    cout << "    ┌─────────────┬────────────────────────────────────────┐\n";
    cout << "    │ " << COLOR_YELLOW << "W A S D" << COLOR_RESET << "     │ Move Up/Left/Down/Right                │\n";
    cout << "    │ " << COLOR_YELLOW << "SPACE" << COLOR_RESET << "       │ Mine block / Attack                    │\n";
    cout << "    │ " << COLOR_YELLOW << "C" << COLOR_RESET << "           │ Open Crafting Menu                     │\n";
    cout << "    │ " << COLOR_YELLOW << "I" << COLOR_RESET << "           │ View Inventory                         │\n";
    cout << "    │ " << COLOR_YELLOW << "P" << COLOR_RESET << "           │ Pause / Save Game                      │\n";
    cout << "    │ " << COLOR_YELLOW << "Q" << COLOR_RESET << "           │ Quit to Menu                           │\n";
    cout << "    └─────────────┴────────────────────────────────────────┘\n\n";
    
    cout << COLOR_BOLD_CYAN << "    RESOURCES:\n" << COLOR_RESET;
    cout << "    " << COLOR_WOOD << "T" << COLOR_RESET << " Wood     → Craft wooden tools (mine stone)\n";
    cout << "    " << COLOR_STONE << "#" << COLOR_RESET << " Stone    → Craft stone tools (mine iron)\n";
    cout << "    " << COLOR_IRON << "I" << COLOR_RESET << " Iron     → Craft iron tools (mine gold)\n";
    cout << "    " << COLOR_GOLD_ORE << "G" << COLOR_RESET << " Gold     → Craft gold tools (mine diamond)\n";
    cout << "    " << COLOR_DIAMOND << "D" << COLOR_RESET << " Diamond  → Best equipment!\n\n";
    
    cout << COLOR_BOLD_CYAN << "    PROGRESSION:\n" << COLOR_RESET;
    cout << "    1. Chop trees for wood\n";
    cout << "    2. Craft wooden pickaxe to mine stone\n";
    cout << "    3. Each tier upgrade requires winning a minigame!\n";
    cout << "    4. Get full Diamond armor to access the Dragon Cave\n";
    cout << "    5. Defeat the Dragon in Space Invaders-style combat!\n\n";
    
    cout << COLOR_BOLD_CYAN << "    MINIGAMES:\n" << COLOR_RESET;
    cout << "    • " << COLOR_GREEN << "Wordle" << COLOR_RESET << " - Guess the word to unlock Iron\n";
    cout << "    • " << COLOR_YELLOW << "Minesweeper" << COLOR_RESET << " - Clear the grid to unlock Gold\n";
    cout << "    • " << COLOR_CYAN << "Space Invaders" << COLOR_RESET << " - Final boss battle!\n\n";
    
    cout << COLOR_DIM << "    Press any key to return to menu..." << COLOR_RESET;
    getch();
}

// ----- GAME OVER -----

void showGameOver(const GameState& state, bool isVictory) {
    clearScreen();
    
    if (isVictory) {
        showVictoryArt();
    } else {
        showDefeatArt();
    }
    
    cout << "\n";
    
    // Stats box
    cout << COLOR_CYAN;
    cout << "                    ╔═══════════════════════════════════╗\n";
    cout << "                    ║         FINAL STATISTICS          ║\n";
    cout << "                    ╠═══════════════════════════════════╣\n";
    cout << COLOR_RESET;
    
    cout << COLOR_CYAN << "                    ║" << COLOR_WHITE;
    cout << "  Ores Mined:     " << setw(15) << state.oresMined;
    cout << COLOR_CYAN << " ║\n";
    
    cout << COLOR_CYAN << "                    ║" << COLOR_WHITE;
    cout << "  Enemies Killed: " << setw(15) << state.enemiesKilled;
    cout << COLOR_CYAN << " ║\n";
    
    cout << COLOR_CYAN << "                    ║" << COLOR_WHITE;
    cout << "  Best Pickaxe:   " << setw(15) << getMaterialName(state.player.equipment.pickaxe);
    cout << COLOR_CYAN << " ║\n";
    
    cout << COLOR_CYAN << "                    ║" << COLOR_WHITE;
    cout << "  Best Armor:     " << setw(15) << getMaterialName(state.player.equipment.armor);
    cout << COLOR_CYAN << " ║\n";
    
    cout << COLOR_CYAN << "                    ╠═══════════════════════════════════╣\n";
    
    cout << COLOR_CYAN << "                    ║" << COLOR_BOLD_YELLOW;
    cout << "  FINAL SCORE:    " << setw(15) << state.score;
    cout << COLOR_CYAN << " ║\n";
    
    cout << COLOR_CYAN;
    cout << "                    ╚═══════════════════════════════════╝\n";
    cout << COLOR_RESET;
    
    cout << "\n" << COLOR_DIM << "                    Press any key to continue..." << COLOR_RESET;
    getch();
}

// ----- UTILITY FUNCTIONS -----

bool showConfirmation(const string& message) {
    cout << "\n" << COLOR_WARNING << "    " << message << " (Y/N): " << COLOR_RESET;
    char input = getch();
    return (input == 'y' || input == 'Y');
}

void waitForKeypress() {
    cout << COLOR_DIM << "\n    Press any key to continue..." << COLOR_RESET;
    getch();
}

string getPlayerName(const string& prompt) {
    cout << "\n" << COLOR_WHITE << "    " << prompt << ": " << COLOR_RESET;
    
    // Flush any leftover keypresses from the terminal buffer
    // This prevents phantom newlines from auto-submitting the name
    tcflush(STDIN_FILENO, TCIFLUSH);
    cin.clear();
    
    string name;
    getline(cin, name);
    
    if (name.empty()) name = "Player";
    if (name.length() > 14) name = name.substr(0, 14);
    return name;
}
