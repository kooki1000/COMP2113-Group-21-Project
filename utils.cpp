#include "utils.h"

#include <clocale>

#include "types.h"

std::string stripAnsi(const std::string& text) {
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

int displayWidth(const std::string& text) {
    static bool localeSet = (std::setlocale(LC_CTYPE, ""), true);
    (void)localeSet;

    std::string clean = stripAnsi(text);
    int width = 0;
    std::mbstate_t state{};
    const char* ptr = clean.c_str();
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

        if (consumed == 0) break;

        int charWidth = wcwidth(wc);
        width += (charWidth >= 0) ? charWidth : 1;
        ptr += consumed;
        remaining -= consumed;
    }
    return width;
}
