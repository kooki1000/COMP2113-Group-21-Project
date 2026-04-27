#ifndef UTIL_H
#define UTIL_H

#include <string>

// Strip ANSI escape codes from text
std::string stripAnsi(const std::string& text);

// Calculate actual terminal display width (accounts for UTF-8/wide chars)
int displayWidth(const std::string& text);

#endif