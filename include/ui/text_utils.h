#pragma once

#include <string>
#include <vector>

#include <SDL3_ttf/SDL_ttf.h>

std::vector<std::string> wrapText(TTF_Font* font, const std::string& text, int maxWidth);
