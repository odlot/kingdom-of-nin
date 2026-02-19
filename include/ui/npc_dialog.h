#pragma once

#include "SDL3/SDL.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

void renderNpcDialog(SDL_Renderer* renderer, TTF_Font* font, const std::string& title,
                     const std::string& text, int windowWidth, int windowHeight,
                     float& scroll);
