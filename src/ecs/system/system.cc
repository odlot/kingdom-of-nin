#include "ecs/system/system.h"

System::System(Registry& registry, std::bitset<MAX_COMPONENTS> signature)
    : registry(registry), signature(signature) {}

std::vector<int>::const_iterator System::entityIdsBegin() const {
  return std::cbegin(this->entityIds);
}

std::vector<int>::const_iterator System::entityIdsEnd() const {
  return std::cend(this->entityIds);
}