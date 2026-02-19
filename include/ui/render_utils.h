#pragma once

#include "SDL3/SDL.h"
#include "ecs/position.h"

void drawCircle(SDL_Renderer* renderer, const Position& center, float radius,
                const Position& cameraPosition, SDL_Color color);

void drawFacingArc(SDL_Renderer* renderer, const Position& center, float radius, float facingX,
                   float facingY, float halfAngle, const Position& cameraPosition,
                   SDL_Color color);
