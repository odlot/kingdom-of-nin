#pragma once

#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <array>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "camera.h"
#include "ecs/position.h"
#include "ecs/registry.h"
#include "ecs/system/respawn_system.h"
#include "events/event_bus.h"
#include "items/item_database.h"
#include "skills/skill_database.h"
#include "skills/skill_tree.h"
#include "ui/buff_bar.h"
#include "ui/character_stats.h"
#include "ui/floating_text_system.h"
#include "ui/inventory.h"
#include "ui/minimap.h"
#include "ui/quest_log.h"
#include "ui/skill_bar.h"
#include "ui/skill_tree.h"
#include "quests/quest_database.h"
#include "quests/quest_system.h"
#include "world/map.h"

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

class Game {
public:
  Game();
  ~Game();

  bool isRunning();
  void update(float dt);
  void interpolate(float alpha);
  void render();

private:
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  TTF_Font* font = nullptr;
  std::unique_ptr<Camera> camera;
  std::unique_ptr<Inventory> inventoryUi;
  std::unique_ptr<CharacterStats> characterStats;
  std::unique_ptr<Minimap> minimap;
  std::unique_ptr<QuestLog> questLogUi;
  std::unique_ptr<ItemDatabase> itemDatabase;
  std::unique_ptr<SkillDatabase> skillDatabase;
  std::unique_ptr<SkillTreeDefinition> skillTreeDefinition;
  std::unique_ptr<QuestDatabase> questDatabase;
  std::unique_ptr<QuestSystem> questSystem;
  std::unique_ptr<EventBus> eventBus;
  std::unique_ptr<FloatingTextSystem> floatingTextSystem;
  std::unique_ptr<RespawnSystem> respawnSystem;
  std::unique_ptr<SkillBar> skillBar;
  std::unique_ptr<BuffBar> buffBar;
  std::unique_ptr<SkillTree> skillTree;
  bool running = true;
  std::unique_ptr<Registry> registry;
  std::unique_ptr<Map> map;
  int playerEntityId = -1;
  std::vector<int> mobEntityIds;
  std::vector<int> lootEntityIds;
  std::vector<int> projectileEntityIds;
  std::vector<int> npcEntityIds;
  std::vector<int> shopNpcIds;
  float attackCooldownRemaining = 0.0f;
  std::array<bool, 5> wasSkillPressed = {false, false, false, false, false};
  bool wasPickupPressed = false;
  bool wasInteractPressed = false;
  bool wasResurrectPressed = false;
  bool wasDebugPressed = false;
  bool wasQuestLogPressed = false;
  bool wasAcceptQuestPressed = false;
  bool wasTurnInQuestPressed = false;
  bool wasQuestPrevPressed = false;
  bool wasQuestNextPressed = false;
  bool showDebugMobRanges = false;
  int lastRegionIndex = -1;
  float playerHitFlashTimer = 0.0f;
  float playerKnockbackImmunityRemaining = 0.0f;
  float facingX = 0.0f;
  float facingY = 1.0f;
  float facingAngle = 1.5707964f;
  bool isPlayerGhost = false;
  bool hasCorpse = false;
  Position corpsePosition = Position(0.0f, 0.0f);
  int currentAutoTargetId = -1;
  int currentNpcId = -1;
  int activeNpcId = -1;
  int activeNpcQuestSelection = 0;
  bool shopOpen = false;
  bool questLogVisible = false;
  float questLogScroll = 0.0f;
  std::string shopNotice;
  float shopNoticeTimer = 0.0f;
  float shopScroll = 0.0f;
  float inventoryScroll = 0.0f;
  float npcDialogScroll = 0.0f;
  float mouseWheelDelta = 0.0f;
  bool wasMousePressed = false;
  std::mt19937 rng;
};
