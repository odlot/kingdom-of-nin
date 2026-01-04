#include "ecs/system/graphic_system.h"
#include "SDL3/SDL.h"
#include "ecs/component/graphic_component.h"
#include "ecs/component/transform_component.h"
#include "ecs/registry.h"
#include <SDL3/SDL_rect.h>

GraphicSystem::GraphicSystem(Registry& registry, std::bitset<MAX_COMPONENTS> signature)
    : System(registry, signature) {}

void GraphicSystem::render(SDL_Renderer* renderer, const Position& cameraPosition) {
  for (auto it = this->entityIdsBegin(); it != this->entityIdsEnd(); ++it) {
    int entityId = *it;
    // Render each entity based on its components
    // This is a placeholder for actual rendering logic
    const GraphicComponent& graphicComponent = registry.getComponent<GraphicComponent>(entityId);
    const TransformComponent& transformComponent =
        registry.getComponent<TransformComponent>(entityId);
    SDL_FRect adjustedRect = {transformComponent.position.x - cameraPosition.x,
                              transformComponent.position.y - cameraPosition.y, 32, 32};
    // Use graphicComponent data to render the entity
    SDL_SetRenderDrawColor(renderer, graphicComponent.color.r, graphicComponent.color.g,
                           graphicComponent.color.b, graphicComponent.color.a);
    SDL_RenderFillRect(renderer, &adjustedRect);
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderRect(renderer, &adjustedRect);
  }
}
