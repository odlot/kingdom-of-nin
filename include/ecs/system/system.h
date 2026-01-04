#pragma once

#include <bitset>
#include <vector>

class Registry;

constexpr int MAX_COMPONENTS = 32;

class System {
public:
  System(Registry& registry, std::bitset<MAX_COMPONENTS> signature);
  virtual ~System() = default;
  const std::bitset<MAX_COMPONENTS>& getSignature() const { return this->signature; }

  void registerEntity(int entityId) { this->entityIds.push_back(entityId); }

protected:
  Registry& registry;

  std::vector<int>::const_iterator entityIdsBegin() const;
  std::vector<int>::const_iterator entityIdsEnd() const;

private:
  std::bitset<MAX_COMPONENTS> signature;
  std::vector<int> entityIds;
};