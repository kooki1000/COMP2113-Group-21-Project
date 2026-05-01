/*
 * menu.cpp
 *
 * All the UI screens and menus.
 */

#include "menu.h"
#include "colors.h"
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <termios.h>
#include <unistd.h>

bool g_minigameForfeited = false;

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
  // Art content is 83 visual chars wide
  static const char* artLines[] = {
    "████████╗███████╗██████╗ ███╗   ███╗██╗ ██████╗██████╗  █████╗ ███████╗████████╗",
    "╚══██╔══╝██╔════╝██╔══██╗████╗ ████║██║██╔════╝██╔══██╗██╔══██╗██╔════╝╚══██╔══╝",
    "   ██║   █████╗  ██████╔╝██╔████╔██║██║██║     ██████╔╝███████║█████╗     ██║   ",
    "   ██║   ██╔══╝  ██╔══██╗██║╚██╔╝██║██║██║     ██╔══██╗██╔══██║██╔══╝     ██║   ",
    "   ██║   ███████╗██║  ██║██║ ╚═╝ ██║██║╚██████╗██║  ██║██║  ██║██║        ██║   ",
    "   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝╚═╝ ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝        ╚═╝  ",
  };
  std::string P = hpad(83);
  std::cout << COLOR_BOLD_CYAN << "\n";
  for (const char* line : artLines)
    std::cout << P << line << "\n";
  std::cout << COLOR_RESET;

  const char* tagline = "⛏   M I N E .  C R A F T .  S U R V I V E .  ⚔";
  const char* stars   = "·  ˚  ✦  ·  ˚    ⋆    ·  ✦  ˚  ·  ✦  ·  ˚  ⋆  ·";
  std::cout << COLOR_YELLOW  << hpad(49) << tagline << "\n" << COLOR_RESET;
  std::cout << COLOR_DIM     << hpad(51) << stars   << "\n" << COLOR_RESET;
}

void showVictoryArt() {
  std::string V = hpad(63);  // art is ~63 chars wide
  std::cout << COLOR_BOLD_YELLOW << "\n";
  std::cout << V << "\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97 \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97 \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97 \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97 \xe2\x96\x88\xe2\x96\x88\xe2\x95\x97   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\n";
  std::cout << V << "\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x95\x9a\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97 \xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\n";
  std::cout << V << "\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91        \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d \xe2\x95\x9a\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\n";
  std::cout << V << "\xe2\x95\x9a\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97 \xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91        \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97  \xe2\x95\x9a\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d  \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d\n";
  std::cout << V << " \xe2\x95\x9a\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x95\x9a\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x95\x9a\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91  \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\n";
  std::cout << V << "  \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d  \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d   \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d    \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d  \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d   \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d   \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d\n";
  std::cout << COLOR_RESET << "\n";

  // "🐉  THE DRAGON HAS BEEN SLAIN!  🐉" = 2+2+26+2+2 = 34 display cols
  std::string Md = hpad(34);
  // LEGENDARY box: ╔19═╗ = 21 display cols
  std::string Mb = hpad(21);
  std::cout << COLOR_GREEN  << Md << "\xf0\x9f\x90\x89  THE DRAGON HAS BEEN SLAIN!  \xf0\x9f\x90\x89\n\n";
  std::cout << COLOR_BOLD_GREEN << Mb << "\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x97\n";
  std::cout << Mb << "\xe2\x95\x91   \xf0\x9f\x91\x91 LEGENDARY!   \xe2\x95\x91\n";
  std::cout << Mb << "\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\n";
  std::cout << COLOR_RESET;
}

void showDefeatArt() {
  std::string D = hpad(79);  // art is ~79 chars wide
  std::cout << COLOR_BOLD_RED << "\n";
  std::cout << D << " \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97 \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97   \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97     \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97 \xe2\x96\x88\xe2\x96\x88\xe2\x95\x97   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\n";
  std::cout << D << "\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d \xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97 \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d    \xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\n";
  std::cout << D << "\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97      \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d\n";
  std::cout << D << "\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x95\x9a\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d      \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d  \xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\n";
  std::cout << D << "\xe2\x95\x9a\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91  \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91 \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97    \xe2\x95\x9a\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d \xe2\x95\x9a\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91  \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\n";
  std::cout << D << " \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d  \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d     \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d     \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d   \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d  \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d  \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d\n";
  std::cout << COLOR_RESET << "\n";

  std::string M = hpad(34);
  std::cout << COLOR_DIM;
  std::cout << M << "\xf0\x9f\x92\x80  You have perished...  \xf0\x9f\x92\x80\n\n";
  std::cout << M << "   The mines claim another soul.\n";
  std::cout << COLOR_RESET;
}

// ----- BOX DRAWING -----

void showBox(const std::vector<std::string> &lines, int width) {
  // Centers the box horizontally: total visual width = width + 2 (borders)
  std::string P = hpad(width + 2);
  std::cout << COLOR_CYAN;
  std::cout << P << "╔";
  for (int i = 0; i < width; i++) std::cout << "═";
  std::cout << "╗\n";

  for (const std::string &line : lines) {
    std::cout << P << "║" << COLOR_WHITE;
    int padding = width - (int)line.length();
    int leftPad = padding / 2;
    int rightPad = padding - leftPad;
    std::cout << std::string(leftPad, ' ') << line
              << std::string(rightPad, ' ');
    std::cout << COLOR_CYAN << "║\n";
  }

  std::cout << P << "╚";
  for (int i = 0; i < width; i++) std::cout << "═";
  std::cout << "╝\n";
  std::cout << COLOR_RESET;
}

// ----- MAIN MENU -----

int showMainMenu(const HighScore &highScore) {
  while (true) {
    // title(8) + tagline+stars(2) + gap(1) + box(13) + score(2) + input(1) = 27
    clearAndCenterV(27);
    showTitleArt();

    std::cout << "\n";

    int termWmm, termHmm;
    getTermSize(termWmm, termHmm);
    int menuW = std::max(40, std::min(termWmm - 6, 60));

    std::vector<std::string> menuItems = {
        "", "[1]  🎮  NEW GAME",    "", "[2]  💾  LOAD GAME",
        "", "[3]  🏆  HIGH SCORES", "", "[4]  📖  HOW TO PLAY",
        "", "[5]  🚪  QUIT",        ""};
    showBox(menuItems, menuW);

    std::string CP = hpad(menuW + 2);
    std::cout << "\n";
    std::cout << COLOR_YELLOW << CP << "🏆 HIGH SCORE: " << COLOR_BOLD_YELLOW;
    std::cout << highScore.score;
    std::cout << COLOR_YELLOW << " (" << getDifficultyColor(highScore.difficulty);
    switch (highScore.difficulty) {
    case DIFF_EASY:   std::cout << "EASY";   break;
    case DIFF_NORMAL: std::cout << "NORMAL"; break;
    case DIFF_HARD:   std::cout << "HARD";   break;
    }
    std::cout << COLOR_YELLOW << ")" << COLOR_RESET << "\n";

    if (highScore.defeatedDragon) {
      std::cout << COLOR_GREEN << CP << "🐉 Dragon Slayer: "
                << highScore.playerName << COLOR_RESET << "\n";
    }

    std::cout << "\n" << COLOR_DIM << CP << "Enter your choice [1-5]: " << COLOR_RESET;

    char choice = getch();
    if (choice >= '1' && choice <= '5') {
      return choice - '0';
    }
  }
}

// ----- DIFFICULTY SELECTION -----

Difficulty selectDifficulty() {
  int selected = 1; // start on Normal

  struct DiffCard {
    const char* key; const char* name; const char* tag;
    const char* flavor; const char* hp; const char* mult;
    const char* col; const char* colBold;
  };
  static const DiffCard CARDS[3] = {
    {"1","EASY",  "RECRUIT","the world is generous. enemies nap. ore is plentiful.",
     "150 HP","x1.0",COLOR_GREEN, COLOR_BOLD_GREEN},
    {"2","NORMAL","SOLDIER","the intended experience. plan your moves. survive.",
     "100 HP","x1.5",COLOR_YELLOW,COLOR_BOLD_YELLOW},
    {"3","HARD",  "VETERAN","no mercy. ore is scarce. one mistake ends you.",
     " 75 HP","x2.0",COLOR_RED,   COLOR_BOLD_RED},
  };
  // Helper: repeat a UTF-8 string n times
  auto rep = [](const char* s, int n) {
    std::string r; for (int i = 0; i < n; i++) r += s; return r;
  };

  while (true) {
    int termWd, termHd;
    getTermSize(termWd, termHd);
    int BW = std::max(54, std::min(termWd - 6, 88));

    clearAndCenterV(18);
    std::string P = hpad(BW + 2);

    // ── Header ──────────────────────────────────────────
    std::cout << COLOR_BOLD_WHITE
              << P << "╔" << rep("═", BW) << "╗\n"
              << P << "║" << rep(" ", (BW-22)/2)
              << "  ⚔   SELECT DIFFICULTY   ⚔  "
              << rep(" ", BW-22-(BW-22)/2) << "║\n"
              << P << "╚" << rep("═", BW) << "╝\n\n"
              << COLOR_RESET;

    // ── Cards ───────────────────────────────────────────
    for (int i = 0; i < 3; i++) {
      const DiffCard& c = CARDS[i];
      bool sel = (selected == i);

      if (sel) {
        std::cout << P << c.colBold << "┌" << rep("─", BW) << "┐\n";

        // Title: arrow + key + name + tag ... stats
        char title[64], stats[20];
        std::snprintf(title, sizeof(title), " ▶  [%s]  %-6s  ·  %-7s", c.key, c.name, c.tag);
        std::snprintf(stats, sizeof(stats), "  %s  %s ", c.hp, c.mult);
        int gap = BW - (int)strlen(title) - (int)strlen(stats);
        if (gap < 0) gap = 0;
        std::cout << P << "│" << c.colBold << title
                  << rep(" ", gap) << COLOR_BOLD_WHITE << stats
                  << c.colBold << "│\n";

        // Flavor — truncate with ellipsis if too long for the current BW
        char flv[128];
        std::snprintf(flv, sizeof(flv), "   %s", c.flavor);
        int flen = (int)strlen(flv);
        if (flen > BW - 1) {
            flv[BW - 4] = '.'; flv[BW - 3] = '.'; flv[BW - 2] = ' '; flv[BW - 1] = '\0';
            flen = BW - 1;
        }
        int fpad = BW - flen - 1;
        if (fpad < 0) fpad = 0;
        std::cout << P << "│" << COLOR_DIM << flv
                  << rep(" ", fpad) << c.colBold << "│\n"
                  << P << "└" << rep("─", BW) << "┘\n" << COLOR_RESET;
      } else {
        char line[160];
        std::snprintf(line, sizeof(line), "     [%s]  %-6s  ·  %-7s         %s  %s",
                      c.key, c.name, c.tag, c.hp, c.mult);
        int llen = (int)strlen(line);
        int lpad = BW - llen;
        if (lpad < 0) { line[BW] = '\0'; lpad = 0; }
        std::cout << P << COLOR_DIM << line << std::string(lpad, ' ') << "\n" << COLOR_RESET;
      }
      if (i < 2) std::cout << "\n";
    }

    // ── Hint ────────────────────────────────────────────
    std::cout << "\n" << COLOR_DIM << P
              << "  W/S  navigate   ·   1-3  select   ·   ENTER  confirm   ·   Q  back\n"
              << COLOR_RESET;

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
  // header(3) + gap(1) + table header(3) + 10 rows + footer(1) + prompt(2) = 20
  clearAndCenterV(20);
  std::string H = hpad(45);  // header box width
  std::string T = hpad(58);  // table visual width is 58 chars

  std::cout << COLOR_BOLD_YELLOW;
  std::cout << H << "╔═══════════════════════════════════════════╗\n";
  std::cout << H << "║            🏆 HIGH SCORES 🏆              ║\n";
  std::cout << H << "╚═══════════════════════════════════════════╝\n\n";
  std::cout << COLOR_RESET;

  std::cout << COLOR_CYAN;
  std::cout << T << "┌──────┬────────────────┬──────────┬────────────┬────────┐\n";
  std::cout << T << "│ RANK │     NAME       │  SCORE   │ DIFFICULTY │ DRAGON │\n";
  std::cout << T << "├──────┼────────────────┼──────────┼────────────┼────────┤\n";
  std::cout << COLOR_RESET;

  for (int i = 0; i < 10; i++) {
    std::cout << T << "│";

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
  std::cout << T << "└──────┴────────────────┴──────────┴────────────┴────────┘\n";
  std::cout << COLOR_RESET;

  std::cout << "\n" << COLOR_DIM << T << "Press any key to return to menu..." << COLOR_RESET;
  getch();
}

// ----- HOW TO PLAY -----

void showHowToPlay() {
  int termW, termH;
  getTermSize(termW, termH);
  // Box header is 74 chars wide; expand up to 118 on big terminals
  const int MIN_BW = 74, MAX_BW = 118;
  int BW = std::max(MIN_BW, std::min(termW - 6, MAX_BW));
  clearAndCenterV(30);
  std::string P = hpad(BW + 2);

  // Build dynamic border
  auto hbar = [&]() {
    std::string s;
    for (int i = 0; i < BW; i++) s += "\xe2\x95\x90";
    return s;
  };
  // Center a title string within BW (display width, no ANSI codes)
  auto center = [&](const char* s, int dispW) {
    int pad = (BW - dispW) / 2;
    return std::string(std::max(0, pad), ' ') + s
         + std::string(std::max(0, BW - dispW - pad), ' ');
  };

  std::cout << COLOR_BOLD_WHITE << "\n";
  std::cout << P << "\xe2\x95\x94" << hbar() << "\xe2\x95\x97\n";
  // "📖 HOW TO PLAY 📖" — emoji are 2-wide each, text = 2+1+12+1+2 = 18 display cols
  std::cout << P << "\xe2\x95\x91" << center("\xf0\x9f\x93\x96 HOW TO PLAY \xf0\x9f\x93\x96", 18) << "\xe2\x95\x91\n";
  std::cout << P << "\xe2\x95\x9a" << hbar() << "\xe2\x95\x9d\n\n";
  std::cout << COLOR_RESET;

  std::cout << COLOR_BOLD_CYAN << P << "OBJECTIVE:\n" << COLOR_RESET;
  std::cout << P << "Mine resources, craft better equipment, and defeat the Dragon!\n\n";

  std::cout << COLOR_BOLD_CYAN << P << "CONTROLS:\n" << COLOR_RESET;
  std::cout << P << "┌─────────────┬────────────────────────────────────────┐\n";
  std::cout << P << "│ " << COLOR_YELLOW << "W A S D" << COLOR_RESET
            << "     │ Move Up/Left/Down/Right                │\n";
  std::cout << P << "│ " << COLOR_YELLOW << "SPACE" << COLOR_RESET
            << "       │ Mine block / Attack                    │\n";
  std::cout << P << "│ " << COLOR_YELLOW << "C" << COLOR_RESET
            << "           │ Open Crafting Menu                     │\n";
  std::cout << P << "│ " << COLOR_YELLOW << "I" << COLOR_RESET
            << "           │ View Inventory                         │\n";
  std::cout << P << "│ " << COLOR_YELLOW << "P" << COLOR_RESET
            << "           │ Pause / Save Game                      │\n";
  std::cout << P << "│ " << COLOR_YELLOW << "Q" << COLOR_RESET
            << "           │ Quit to Menu                           │\n";
  std::cout << P << "└─────────────┴────────────────────────────────────────┘\n\n";

  std::cout << COLOR_BOLD_CYAN << P << "RESOURCES:\n" << COLOR_RESET;
  std::cout << P << COLOR_WOOD    << "T" << COLOR_RESET << " Wood     → Craft wooden tools (mine stone)\n";
  std::cout << P << COLOR_STONE   << "#" << COLOR_RESET << " Stone    → Craft stone tools (mine iron)\n";
  std::cout << P << COLOR_IRON    << "I" << COLOR_RESET << " Iron     → Craft iron tools (mine gold)\n";
  std::cout << P << COLOR_GOLD_ORE<< "G" << COLOR_RESET << " Gold     → Craft gold tools (mine diamond)\n";
  std::cout << P << COLOR_DIAMOND << "D" << COLOR_RESET << " Diamond  → Best equipment!\n\n";

  std::cout << COLOR_BOLD_CYAN << P << "PROGRESSION:\n" << COLOR_RESET;
  std::cout << P << "1. Chop trees for wood\n";
  std::cout << P << "2. Craft wooden pickaxe to mine stone\n";
  std::cout << P << "3. Each tier upgrade requires winning a minigame!\n";
  std::cout << P << "4. Mine the Dragon Cave block to enter the lair\n";
  std::cout << P << "5. Defeat the Dragon in Space Invaders-style combat!\n\n";

  std::cout << COLOR_BOLD_CYAN << P << "MINIGAMES:\n" << COLOR_RESET;
  std::cout << P << "• " << COLOR_GREEN  << "Wordle"       << COLOR_RESET << " - Guess the word to succeed the mining challenge\n";
  std::cout << P << "• " << COLOR_YELLOW << "Minesweeper"  << COLOR_RESET << " - Clear the grid to succeed the mining challenge\n";
  std::cout << P << "• " << COLOR_CYAN   << "Dragon Fight" << COLOR_RESET << " - Final boss battle! Win a minigame to enter.\n\n";

  std::cout << COLOR_DIM << P << "Press any key to return to menu..." << COLOR_RESET;
  getch();
}

// ----- GAME OVER -----

void showGameOver(const GameState &state, bool isVictory) {
  // 7 art + 1 taunt + 1 blank + 7 stats + 1 prompt = 17 rows
  clearAndCenterV(17);

  if (isVictory) {
    showVictoryArt();
    std::cout << COLOR_BOLD_GREEN << hpad(47)
              << "GG EASY CLAP. THE DRAGON NEVER STOOD A CHANCE.\n"
              << COLOR_RESET;
  } else {
    showDefeatArt();
    static const char* taunts[] = {
        "SKILL ISSUE.",
        "YOU LOWKEY SUCK AT THIS.",
        "L + RATIO + DRAGON DIFF.",
        "TOUCH GRASS AND TRY AGAIN.",
        "THE DRAGON WASN'T EVEN TRYING.",
        "BRO GOT COOKED BY A PIXEL DRAGON.",
        "AVERAGE MINESWEEPER PLAYER.",
        "NOT EVEN CLOSE, BABY.",
        "CERTIFIED SKILL GAP MOMENT.",
        "THIS AIN'T IT, CHIEF.",
    };
    int idx = rand() % 10;
    std::cout << COLOR_RED << hpad((int)strlen(taunts[idx])) << taunts[idx] << "\n" << COLOR_RESET;
  }

  std::cout << "\n";

  // Stats box — dynamic width
  int termWgo, termHgo;
  getTermSize(termWgo, termHgo);
  int BW = std::max(37, std::min(termWgo - 6, 80));
  std::string B = hpad(BW + 2);

  auto topBarGO = [&]() { std::cout << COLOR_CYAN << B << "\xe2\x95\x94"; for (int i=0;i<BW;i++) std::cout << "\xe2\x95\x90"; std::cout << "\xe2\x95\x97\n"; };
  auto midBarGO = [&]() { std::cout << COLOR_CYAN << B << "\xe2\x95\xa0"; for (int i=0;i<BW;i++) std::cout << "\xe2\x95\x90"; std::cout << "\xe2\x95\xa3\n"; };
  auto botBarGO = [&]() { std::cout << COLOR_CYAN << B << "\xe2\x95\x9a"; for (int i=0;i<BW;i++) std::cout << "\xe2\x95\x90"; std::cout << "\xe2\x95\x9d\n"; };

  int titleLeft = (BW - 16) / 2;
  int titleRight = BW - 16 - titleLeft;

  topBarGO();
  std::cout << COLOR_CYAN << B << "\xe2\x95\x91"
            << std::string(titleLeft, ' ') << "FINAL STATISTICS" << std::string(titleRight, ' ')
            << "\xe2\x95\x91\n";
  midBarGO();
  std::cout << COLOR_RESET;

  auto row = [&](const char* label, const std::string& val) {
    int lw = BW * 6/10 - 2;
    int vw = BW - 2 - lw;
    std::cout << COLOR_CYAN << B << "\xe2\x95\x91" << COLOR_WHITE;
    std::cout << "  " << std::left << std::setw(lw) << label << std::right << std::setw(vw) << val;
    std::cout << COLOR_CYAN << "\xe2\x95\x91\n" << COLOR_RESET;
  };
  row("Ores Mined:",   std::to_string(state.oresMined));
  row("Best Pickaxe:", getMaterialName(state.player.equipment.pickaxe));
  row("Best Armor:",   getMaterialName(state.player.equipment.armor));

  midBarGO();
  std::cout << B << "\xe2\x95\x91" << COLOR_BOLD_YELLOW;
  std::cout << "  FINAL SCORE:      " << std::right << std::setw(BW - 20) << state.score;
  std::cout << COLOR_CYAN << "\xe2\x95\x91\n";
  botBarGO();
  std::cout << COLOR_RESET;

  // Flush any buffered input from ncurses fight before waiting
  tcflush(STDIN_FILENO, TCIFLUSH);
  std::cout << "\n" << COLOR_DIM << B << "Press any key to continue..." << COLOR_RESET;
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

  // Facing direction
  const char* dirArrow = "?";
  if      (state.player.facingX ==  1) dirArrow = "→";
  else if (state.player.facingX == -1) dirArrow = "←";
  else if (state.player.facingY == -1) dirArrow = "↑";
  else if (state.player.facingY ==  1) dirArrow = "↓";
  std::cout << "  " << COLOR_CYAN << "DIR:" << dirArrow << COLOR_RESET;

  // Depth and position
  std::cout << "  Depth: " << (state.player.pos.y - SURFACE_LEVEL);
  std::cout << "  Pos: (" << state.player.pos.x << ", " << state.player.pos.y << ")";
  std::cout << "\n";

  // Red zone alert banner — shows for first ~3 seconds after zone spawns
  const RandomEvent& ev = state.activeEvent;
  if (ev.active && ev.type == EVENT_RED_ZONE && ev.alertTicks > 0) {
    bool blink = (ev.alertTicks / 8) % 2 == 0;  // flash every 8 frames
    if (blink) {
      std::cout << COLOR_BOLD_RED
                << " \xe2\x96\x88\xe2\x96\x88 FIRE ZONE ACTIVE \xe2\x96\x88\xe2\x96\x88"
                << "  EVACUATE THE AREA OR TAKE BURN DAMAGE  "
                << "\xe2\x96\x88\xe2\x96\x88 FIRE ZONE ACTIVE \xe2\x96\x88\xe2\x96\x88"
                << COLOR_RESET << "\n";
    } else {
      std::cout << COLOR_BOLD_YELLOW
                << " !! DANGER: FIRE ZONE — move away from the burning area !!"
                << COLOR_RESET << "\n";
    }
  }
}

void showStatusMessage(const std::string &message, bool isError) {
  if (isError) {
    std::cout << COLOR_RED;
  } else {
    std::cout << COLOR_GREEN;
  }
  std::cout << " >> " << message << COLOR_RESET << "\n";
}
