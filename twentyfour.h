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

bool runTwentyFour(int attempts, int timeLimit);

#endif
