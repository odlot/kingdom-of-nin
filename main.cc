#include "game.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

int main() {
  auto console = spdlog::stdout_color_mt("console");
  spdlog::get("console")->info("Kingdom of Nin");
  std::unique_ptr<Game> game = std::make_unique<Game>();

  float dt = 1.0 / 60.0;
  while (game->isRunning()) {
    game->update(dt);
    game->render();
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }
  return 0;
}
