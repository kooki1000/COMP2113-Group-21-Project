/*
 * menu.cpp
 *
 * All the UI screens and menus.
 */

#include "menu.h"
#include "colors.h"
#include <clocale>
#include <cstdio>
#include <cwchar>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

namespace {
std::string stripAnsi(const std::string &text) {
  std::string out;
  out.reserve(text.size());

  for (std::size_t i = 0; i < text.size();) {
    unsigned char ch = static_cast<unsigned char>(text[i]);
    if (ch == 0x1B && i + 1 < text.size() && text[i + 1] == '[') {
      i += 2;
      while (i < text.size()) {
        unsigned char code = static_cast<unsigned char>(text[i]);
        if (code >= 0x40 && code <= 0x7E) {
          i++;
          break;
        }
        i++;
      }
      continue;
    }
    out.push_back(text[i]);
    i++;
  }

  return out;
}

int displayWidth(const std::string &text) {
  static bool localeSet = (std::setlocale(LC_CTYPE, ""), true);
  (void)localeSet;

  std::string clean = stripAnsi(text);
  int width = 0;
  std::mbstate_t state{};
  const char *ptr = clean.c_str();
  std::size_t remaining = clean.size();

  while (remaining > 0) {
    wchar_t wc = 0;
    std::size_t consumed = std::mbrtowc(&wc, ptr, remaining, &state);

    if (consumed == static_cast<std::size_t>(-1) ||
        consumed == static_cast<std::size_t>(-2)) {
      state = std::mbstate_t{};
      width += 1;
      ptr++;
      remaining--;
      continue;
    }

    if (consumed == 0) {
      break;
    }

    int charWidth = wcwidth(wc);
    width += (charWidth >= 0) ? charWidth : 1;
    ptr += consumed;
    remaining -= consumed;
  }

  return width;
}
} // namespace

// ----- INPUT HELPERS -----

// Read a single keypress without waiting for Enter
char getch() {
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
  std::cout << COLOR_BOLD_CYAN;
  std::cout << R"(
    ████████╗███████╗██████╗ ███╗   ███╗██╗ ██████╗██████╗  █████╗ ███████╗████████╗
    ╚══██╔══╝██╔════╝██╔══██╗████╗ ████║██║██╔════╝██╔══██╗██╔══██╗██╔════╝╚══██╔══╝
       ██║   █████╗  ██████╔╝██╔████╔██║██║██║     ██████╔╝███████║█████╗     ██║
       ██║   ██╔══╝  ██╔══██╗██║╚██╔╝██║██║██║     ██╔══██╗██╔══██║██╔══╝     ██║
       ██║   ███████╗██║  ██║██║ ╚═╝ ██║██║╚██████╗██║  ██║██║  ██║██║        ██║
       ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝╚═╝ ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝        ╚═╝
)" << COLOR_RESET;

  std::cout << COLOR_YELLOW;
  std::cout << R"(
                         ⛏️   M I N E .  C R A F T .  S U R V I V E .  ⚔️
)" << COLOR_RESET;

  std::cout << COLOR_DIM;
  std::cout << "                    ·  ˚  ✦  ·  ˚    ⋆    ·  ✦  ˚  ·  ✦  ·  ˚  "
               "⋆  ·\n";
  std::cout << COLOR_RESET;
}

void showVictoryArt() {
  std::cout << COLOR_BOLD_YELLOW;
  std::cout << R"(
    ██╗   ██╗██╗ ██████╗████████╗ ██████╗ ██████╗ ██╗   ██╗██╗
    ██║   ██║██║██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗╚██╗ ██╔╝██║
    ██║   ██║██║██║        ██║   ██║   ██║██████╔╝ ╚████╔╝ ██║
    ╚██╗ ██╔╝██║██║        ██║   ██║   ██║██╔══██╗  ╚██╔╝  ╚═╝
     ╚████╔╝ ██║╚██████╗   ██║   ╚██████╔╝██║  ██║   ██║   ██╗
      ╚═══╝  ╚═╝ ╚═════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚═╝
)" << COLOR_RESET;

  std::cout << COLOR_GREEN;
  std::cout << R"(
                           🐉  THE DRAGON HAS BEEN SLAIN!  🐉

                               ╔═══════════════════╗
                               ║   👑 LEGENDARY!   ║
                               ╚═══════════════════╝
)" << COLOR_RESET;
}

void showDefeatArt() {
  std::cout << COLOR_BOLD_RED;
  std::cout << R"(
     ██████╗  █████╗ ███╗   ███╗███████╗     ██████╗ ██╗   ██╗███████╗██████╗
    ██╔════╝ ██╔══██╗████╗ ████║██╔════╝    ██╔═══██╗██║   ██║██╔════╝██╔══██╗
    ██║  ███╗███████║██╔████╔██║█████╗      ██║   ██║██║   ██║█████╗  ██████╔╝
    ██║   ██║██╔══██║██║╚██╔╝██║██╔══╝      ██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██╗
    ╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗    ╚██████╔╝ ╚████╔╝ ███████╗██║  ██║
     ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝     ╚═════╝   ╚═══╝  ╚══════╝╚═╝  ╚═╝
)" << COLOR_RESET;

  std::cout << COLOR_DIM;
  std::cout << R"(
                              💀  You have perished...  💀

                           The mines claim another soul.
)" << COLOR_RESET;
}

// ----- BOX DRAWING -----

void showBox(const std::vector<std::string> &lines, int width) {
  std::cout << COLOR_CYAN;
  std::cout << "    ╔";
  for (int i = 0; i < width; i++)
    std::cout << "═";
  std::cout << "╗\n";

  for (const std::string &line : lines) {
    std::cout << "    ║" << COLOR_WHITE;
    int padding = width - displayWidth(line);
    if (padding < 0)
      padding = 0;
    int leftPad = padding / 2;
    int rightPad = padding - leftPad;
    for (int i = 0; i < leftPad; i++)
      std::cout << " ";
    std::cout << line;
    for (int i = 0; i < rightPad; i++)
      std::cout << " ";
    std::cout << COLOR_CYAN << "║\n";
  }

  std::cout << "    ╚";
  for (int i = 0; i < width; i++)
    std::cout << "═";
  std::cout << "╝\n";
  std::cout << COLOR_RESET;
}

// ----- MAIN MENU -----

int showMainMenu(const HighScore &highScore) {
  while (true) {
    clearScreen();
    showTitleArt();

    std::cout << "\n";

    std::vector<std::string> menuItems = {
        "", "[1]  🎮  NEW GAME",    "", "[2]  💾  LOAD GAME",
        "", "[3]  🏆  HIGH SCORES", "", "[4]  📖  HOW TO PLAY",
        "", "[5]  🚪  QUIT",        ""};
    showBox(menuItems, 40);

    // Show current high score
    std::cout << "\n";
    std::cout << COLOR_YELLOW
              << "                    🏆 HIGH SCORE: " << COLOR_BOLD_YELLOW;
    bool hasRealHighScore =
        !(highScore.playerName == "---" && highScore.score == 0 &&
          highScore.timestamp == 0 && !highScore.defeatedDragon);

    if (!hasRealHighScore) {
      std::cout << "---" << COLOR_RESET << "\n";
    } else {
      std::cout << highScore.score;
      std::cout << COLOR_YELLOW << " ("
                << getDifficultyColor(highScore.difficulty);
      switch (highScore.difficulty) {
      case DIFF_EASY:
        std::cout << "EASY";
        break;
      case DIFF_NORMAL:
        std::cout << "NORMAL";
        break;
      case DIFF_HARD:
        std::cout << "HARD";
        break;
      }
      std::cout << COLOR_YELLOW << ")" << COLOR_RESET << "\n";
    }

    if (highScore.defeatedDragon) {
      std::cout << COLOR_GREEN << "                    🐉 Dragon Slayer: "
                << highScore.playerName << COLOR_RESET << "\n";
    }

    std::cout << "\n"
              << COLOR_DIM
              << "                         Enter your choice [1-5]: "
              << COLOR_RESET;

    char choice = getch();
    if (choice >= '1' && choice <= '5') {
      return choice - '0';
    }
  }
}

// ----- DIFFICULTY SELECTION -----

Difficulty selectDifficulty() {
  int selected = 1; // start on Normal

  while (true) {
    clearScreen();

    std::cout << COLOR_BOLD_WHITE << "\n\n";
    std::cout << "                    ╔═══════════════════════════════════╗\n";
    std::cout << "                    ║       SELECT DIFFICULTY           ║\n";
    std::cout
        << "                    ╚═══════════════════════════════════╝\n\n";
    std::cout << COLOR_RESET;

    // Easy
    if (selected == 0) {
      std::cout << COLOR_GREEN << "              ▶  ";
    } else {
      std::cout << COLOR_DIM << "                 ";
    }
    std::cout << "[1] EASY" << COLOR_RESET << "\n";
    // std::cout << COLOR_DIM
    //           << "                     • 150 HP  • More ores  • Fewer
    //           enemies\n";
    std::cout << COLOR_DIM << "                     • 150 HP  • More ores\n";
    std::cout << "                     • 4-letter Wordle  • 6x6 Minesweeper\n";
    std::cout << "                     • 6x6 Sudoku (Easy)\n";
    std::cout << "                     • Score: 1.0x multiplier\n\n"
              << COLOR_RESET;

    // Normal
    if (selected == 1) {
      std::cout << COLOR_YELLOW << "              ▶  ";
    } else {
      std::cout << COLOR_DIM << "                 ";
    }
    std::cout << "[2] NORMAL" << COLOR_RESET << "\n";
    // std::cout << COLOR_DIM
    //           << "                     • 100 HP  • Standard ores  • Standard
    //           "
    //              "enemies\n";
    std::cout << COLOR_DIM
              << "                     • 100 HP  • Standard ores\n";
    std::cout << "                     • 5-letter Wordle  • 8x8 Minesweeper\n";
    std::cout << "                     • 6x6 Sudoku (Hard)\n";
    std::cout << "                     • Score: 1.5x multiplier\n\n"
              << COLOR_RESET;

    // Hard
    if (selected == 2) {
      std::cout << COLOR_RED << "              ▶  ";
    } else {
      std::cout << COLOR_DIM << "                 ";
    }
    std::cout << "[3] HARD" << COLOR_RESET << "\n";
    // std::cout << COLOR_DIM
    //           << "                     • 75 HP  • Scarce ores  • Many
    //           enemies\n";
    std::cout << COLOR_DIM << "                     • 75 HP  • Scarce ores\n";
    std::cout
        << "                     • 6-letter Wordle  • 10x10 Minesweeper\n";
    std::cout << "                     • 9x9 Sudoku\n";
    std::cout << "                     • Score: 2.0x multiplier\n\n"
              << COLOR_RESET;

    std::cout << "\n";
    std::cout
        << COLOR_DIM
        << "              Use [W/S] or [1-3] to select, [ENTER] to confirm\n";
    std::cout << "              [Q] to go back\n" << COLOR_RESET;

    char input = getch();

    switch (input) {
    case 'w':
    case 'W':
      selected = (selected - 1 + 3) % 3;
      break;
    case 's':
    case 'S':
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
    case '\n':
    case '\r':
    case ' ':
      return static_cast<Difficulty>(selected);
    case 'q':
    case 'Q':
      return DIFF_NORMAL;
    }
  }
}

// ----- HIGH SCORES -----

void showHighScores(const std::vector<HighScore> &scores) {
  clearScreen();

  std::cout << COLOR_BOLD_YELLOW << "\n\n";
  std::cout
      << "                    ╔═══════════════════════════════════════════╗\n";
  std::cout
      << "                    ║            🏆 HIGH SCORES 🏆              ║\n";
  std::cout << "                    "
               "╚═══════════════════════════════════════════╝\n\n";
  std::cout << COLOR_RESET;

  std::cout << COLOR_CYAN;
  std::cout << "         "
               "┌──────┬────────────────┬──────────┬────────────┬────────┐\n";
  std::cout << "         │ RANK │     NAME       │  SCORE   │ DIFFICULTY │ "
               "DRAGON │\n";
  std::cout << "         "
               "├──────┼────────────────┼──────────┼────────────┼────────┤\n";
  std::cout << COLOR_RESET;

  for (int i = 0; i < 10; i++) {
    std::cout << "         │";

    // Rank with medals for top 3
    if (i < 3) {
      const char *medals[] = {"🥇", "🥈", "🥉"};
      std::cout << "  " << medals[i] << "  │";
    } else {
      std::cout << "  " << std::setw(2) << (i + 1) << "  │";
    }

    if (i < (int)scores.size()) {
      const HighScore &hs = scores[i];

      std::cout << " " << std::setw(14) << std::left
                << hs.playerName.substr(0, 14) << " │";
      std::cout << COLOR_YELLOW << " " << std::setw(8) << std::right << hs.score
                << COLOR_RESET << " │";

      std::cout << getDifficultyColor(hs.difficulty);
      std::string diffStr;
      switch (hs.difficulty) {
      case DIFF_EASY:
        diffStr = "Easy";
        break;
      case DIFF_NORMAL:
        diffStr = "Normal";
        break;
      case DIFF_HARD:
        diffStr = "Hard";
        break;
      }
      std::cout << " " << std::setw(10) << diffStr << COLOR_RESET << " │";

      if (hs.defeatedDragon) {
        std::cout << "   🐉   │";
      } else {
        std::cout << "   -    │";
      }
    } else {
      std::cout << "      ---      │    ---   │    ---     │   -    │";
    }
    std::cout << "\n";
  }

  std::cout << COLOR_CYAN;
  std::cout << "         "
               "└──────┴────────────────┴──────────┴────────────┴────────┘\n";
  std::cout << COLOR_RESET;

  std::cout << "\n"
            << COLOR_DIM
            << "                    Press any key to return to menu..."
            << COLOR_RESET;
  getch();
}

// ----- HOW TO PLAY -----

void showHowToPlay() {
  clearScreen();

  std::cout << COLOR_BOLD_WHITE << "\n";
  std::cout << "    "
               "╔══════════════════════════════════════════════════════════════"
               "═════╗\n";
  std::cout << "    ║                      📖 HOW TO PLAY 📖                   "
               "         ║\n";
  std::cout << "    "
               "╚══════════════════════════════════════════════════════════════"
               "═════╝\n\n";
  std::cout << COLOR_RESET;

  std::cout << COLOR_BOLD_CYAN << "    OBJECTIVE:\n" << COLOR_RESET;
  std::cout << "    Mine resources, craft better equipment, and defeat the "
               "Dragon!\n\n";

  std::cout << COLOR_BOLD_CYAN << "    CONTROLS:\n" << COLOR_RESET;
  std::cout << "    ┌─────────────┬────────────────────────────────────────┐\n";
  std::cout << "    │ " << COLOR_YELLOW << "W A S D" << COLOR_RESET
            << "     │ Move Up/Left/Down/Right                │\n";
  std::cout << "    │ " << COLOR_YELLOW << "SPACE" << COLOR_RESET
            << "       │ Mine block / Attack                    │\n";
  std::cout << "    │ " << COLOR_YELLOW << "C" << COLOR_RESET
            << "           │ Open Crafting Menu                     │\n";
  std::cout << "    │ " << COLOR_YELLOW << "I" << COLOR_RESET
            << "           │ View Inventory                         │\n";
  std::cout << "    │ " << COLOR_YELLOW << "P" << COLOR_RESET
            << "           │ Pause / Save Game                      │\n";
  std::cout << "    │ " << COLOR_YELLOW << "Q" << COLOR_RESET
            << "           │ Quit to Menu                           │\n";
  std::cout
      << "    └─────────────┴────────────────────────────────────────┘\n\n";

  std::cout << COLOR_BOLD_CYAN << "    RESOURCES:\n" << COLOR_RESET;
  std::cout << "    " << COLOR_WOOD << "T" << COLOR_RESET
            << " Wood     → Craft wooden tools (mine stone)\n";
  std::cout << "    " << COLOR_STONE << "#" << COLOR_RESET
            << " Stone    → Craft stone tools (mine iron)\n";
  std::cout << "    " << COLOR_IRON << "I" << COLOR_RESET
            << " Iron     → Craft iron tools (mine gold)\n";
  std::cout << "    " << COLOR_GOLD_ORE << "G" << COLOR_RESET
            << " Gold     → Craft gold tools (mine diamond)\n";
  std::cout << "    " << COLOR_DIAMOND << "D" << COLOR_RESET
            << " Diamond  → Best equipment!\n\n";

  std::cout << COLOR_BOLD_CYAN << "    PROGRESSION:\n" << COLOR_RESET;
  std::cout << "    1. Chop trees for wood\n";
  std::cout << "    2. Craft wooden pickaxe to mine stone\n";
  std::cout << "    3. Each tier upgrade requires winning a minigame!\n";
  std::cout << "    4. Find the Dragon Cave to face the final boss (Diamond "
               "Armor recommended)\n";
  std::cout << "    5. Defeat the Dragon in Space Invaders-style combat!\n\n";

  std::cout << COLOR_BOLD_CYAN << "    MINIGAMES:\n" << COLOR_RESET;
  std::cout << "    • " << COLOR_GREEN << "Wordle" << COLOR_RESET
            << " - Guess the word to upgrade tiers\n";
  std::cout << "    • " << COLOR_YELLOW << "Minesweeper" << COLOR_RESET
            << " - Clear the grid to upgrade tiers\n";
  std::cout << "    • " << COLOR_CYAN << "Sudoku" << COLOR_RESET
            << " - Fill the grid to upgrade tiers\n\n";

  std::cout << COLOR_DIM << "    Press any key to return to menu..."
            << COLOR_RESET;
  getch();
}

// ----- GAME OVER -----

void showGameOver(const GameState &state, bool isVictory) {
  clearScreen();

  if (isVictory) {
    showVictoryArt();
  } else {
    showDefeatArt();
  }

  std::cout << "\n";

  // Stats box
  std::cout << COLOR_CYAN;
  std::cout << "                    ╔═══════════════════════════════════╗\n";
  std::cout << "                    ║         FINAL STATISTICS          ║\n";
  std::cout << "                    ╠═══════════════════════════════════╣\n";
  std::cout << COLOR_RESET;

  std::cout << COLOR_CYAN << "                    ║" << COLOR_WHITE;
  std::cout << "  Ores Mined:     " << std::setw(15) << state.oresMined;
  std::cout << COLOR_CYAN << " ║\n";

  std::cout << COLOR_CYAN << "                    ║" << COLOR_WHITE;
  std::cout << "  Best Pickaxe:   " << std::setw(15)
            << getMaterialName(state.player.equipment.pickaxe);
  std::cout << COLOR_CYAN << " ║\n";

  std::cout << COLOR_CYAN << "                    ║" << COLOR_WHITE;
  std::cout << "  Best Armor:     " << std::setw(15)
            << getMaterialName(state.player.equipment.armor);
  std::cout << COLOR_CYAN << " ║\n";

  std::cout << COLOR_CYAN
            << "                    ╠═══════════════════════════════════╣\n";

  std::cout << COLOR_CYAN << "                    ║" << COLOR_BOLD_YELLOW;
  std::cout << "  FINAL SCORE:    " << std::setw(15) << state.score;
  std::cout << COLOR_CYAN << " ║\n";

  std::cout << COLOR_CYAN;
  std::cout << "                    ╚═══════════════════════════════════╝\n";
  std::cout << COLOR_RESET;

  std::cout << "\n"
            << COLOR_DIM << "                    Press any key to continue..."
            << COLOR_RESET;
  getch();
}

// ----- UTILITY FUNCTIONS -----

bool showConfirmation(const std::string &message) {
  std::cout << "\n"
            << COLOR_WARNING << "    " << message << " (Y/N): " << COLOR_RESET;
  char input = getch();
  return (input == 'y' || input == 'Y');
}

void waitForKeypress() {
  std::cout << COLOR_DIM << "\n    Press any key to continue..." << COLOR_RESET;
  getch();
}

std::string getPlayerName(const std::string &prompt) {
  std::cout << "\n" << COLOR_WHITE << "    " << prompt << ": " << COLOR_RESET;

  // Flush any leftover keypresses from the terminal buffer
  // This prevents phantom newlines from auto-submitting the name
  tcflush(STDIN_FILENO, TCIFLUSH);
  std::cin.clear();

  std::string name;
  std::getline(std::cin, name);

  if (name.empty())
    name = "Player";
  if (name.length() > 14)
    name = name.substr(0, 14);
  return name;
}

// ----- TRANSITION SCREENS -----
// TODO: FOR MINIGAME MAKERS -----

// ----- IN-GAME HUD -----

void renderHUD(const GameState &state) {
  // Health bar
  std::cout << COLOR_BOLD_WHITE << " HP: " << COLOR_RESET;

  // Guard against divide by zero if maxHealth somehow gets corrupted
  int maxHp = state.player.maxHealth > 0 ? state.player.maxHealth : 1;
  int healthPercent = (state.player.health * 20) / maxHp;

  // Clamp so we don't draw weird stuff if health goes negative
  if (healthPercent < 0)
    healthPercent = 0;
  if (healthPercent > 20)
    healthPercent = 20;

  std::cout << "[";
  for (int i = 0; i < 20; i++) {
    if (i < healthPercent) {
      if (healthPercent > 10)
        std::cout << COLOR_GREEN;
      else if (healthPercent > 5)
        std::cout << COLOR_YELLOW;
      else
        std::cout << COLOR_RED;
      std::cout << "█";
    } else {
      std::cout << COLOR_DIM << "░";
    }
  }
  std::cout << COLOR_RESET << "] " << state.player.health << "/"
            << state.player.maxHealth;

  // Score
  std::cout << "  " << COLOR_YELLOW << "⭐ " << state.score << COLOR_RESET;

  // Current equipment
  std::cout << "  " << getMaterialColor(state.player.equipment.pickaxe) << "⛏ "
            << getMaterialName(state.player.equipment.pickaxe);
  std::cout << "  " << getMaterialColor(state.player.equipment.armor) << "🛡 "
            << getMaterialName(state.player.equipment.armor);
  std::cout << COLOR_RESET << "\n";

  // Quick inventory counts
  std::cout << " " << COLOR_WOOD << "T:" << state.player.inventory.wood;
  std::cout << " " << COLOR_STONE << "#:" << state.player.inventory.stone;
  std::cout << " " << COLOR_IRON << "I:" << state.player.inventory.iron;
  std::cout << " " << COLOR_GOLD_ORE << "G:" << state.player.inventory.gold;
  std::cout << " " << COLOR_DIAMOND << "D:" << state.player.inventory.diamond;
  std::cout << COLOR_RESET;

  // Depth and position
  int depth = state.player.pos.y - SURFACE_LEVEL;
  if (depth < 0)
    depth = 0;
  std::cout << "  Depth: " << depth;
  std::cout << "  Pos: (" << state.player.pos.x << ", " << state.player.pos.y
            << ")";
  std::cout << "\n";
}

void showStatusMessage(const std::string &message, bool isError) {
  if (isError) {
    std::cout << COLOR_RED;
  } else {
    std::cout << COLOR_GREEN;
  }
  std::cout << " >> " << message << COLOR_RESET << "\n";
}
