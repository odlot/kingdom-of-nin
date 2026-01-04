#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "component.h"
#include "items/item.h"

class InventoryComponent : public Component {
public:
  static constexpr std::size_t kMaxSlots = 16;

  bool addItem(const ItemInstance& item) {
    if (items.size() >= kMaxSlots) {
      return false;
    }
    items.push_back(item);
    return true;
  }

  bool removeItemAt(std::size_t index) {
    if (index >= items.size()) {
      return false;
    }
    items.erase(items.begin() + static_cast<long>(index));
    return true;
  }

  std::optional<ItemInstance> takeItemAt(std::size_t index) {
    if (index >= items.size()) {
      return std::nullopt;
    }
    ItemInstance item = items[index];
    items.erase(items.begin() + static_cast<long>(index));
    return item;
  }

  std::vector<ItemInstance> items;
};
