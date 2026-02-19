#include "ui/character_stats.h"
#include <cstdlib>
#include <iostream>

namespace {
int failures = 0;

void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    failures += 1;
  }
}
} // namespace

int main() {
  CharacterStats statsUi;
  StatsComponent stats;
  stats.unspentPoints = 1;

  const int windowWidth = 640;
  const SDL_FRect panel = CharacterStats::panelRectForTesting(windowWidth);
  const SDL_FRect plusRect = CharacterStats::plusRectForTesting(panel, 6);

  const int clickX = static_cast<int>(plusRect.x + (plusRect.w / 2.0f));
  const int clickY = static_cast<int>(plusRect.y + (plusRect.h / 2.0f));

  statsUi.handleInput(clickX, clickY, true, windowWidth, stats, true);
  statsUi.handleInput(clickX, clickY, false, windowWidth, stats, true);
  statsUi.handleInput(clickX, clickY, true, windowWidth, stats, true);

  expect(stats.strength == 6, "clicking STR plus increases strength");
  expect(stats.unspentPoints == 0, "unspent points decremented");

  if (failures > 0) {
    std::cerr << failures << " test(s) failed.\n";
    return EXIT_FAILURE;
  }
  std::cout << "All character stats input tests passed.\n";
  return EXIT_SUCCESS;
}
