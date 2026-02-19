#include "ui/text_utils.h"

std::vector<std::string> wrapText(TTF_Font* font, const std::string& text, int maxWidth) {
  std::vector<std::string> lines;
  std::string current;
  std::string word;
  auto flushWord = [&]() {
    if (word.empty()) {
      return;
    }
    std::string candidate = current.empty() ? word : current + " " + word;
    int width = 0;
    int height = 0;
    TTF_GetStringSize(font, candidate.c_str(), candidate.size(), &width, &height);
    if (width <= maxWidth) {
      current = std::move(candidate);
      word.clear();
      return;
    }
    if (!current.empty()) {
      lines.push_back(current);
      current.clear();
    }
    TTF_GetStringSize(font, word.c_str(), word.size(), &width, &height);
    if (width <= maxWidth) {
      current = word;
      word.clear();
      return;
    }
    std::string chunk;
    for (char ch : word) {
      std::string next = chunk + ch;
      TTF_GetStringSize(font, next.c_str(), next.size(), &width, &height);
      if (width > maxWidth && !chunk.empty()) {
        lines.push_back(chunk);
        chunk.clear();
      }
      chunk += ch;
    }
    if (!chunk.empty()) {
      current = chunk;
    }
    word.clear();
  };

  for (char ch : text) {
    if (ch == '\n') {
      flushWord();
      if (!current.empty()) {
        lines.push_back(current);
        current.clear();
      } else {
        lines.push_back("");
      }
      continue;
    }
    if (ch == ' ') {
      flushWord();
    } else {
      word += ch;
    }
  }
  flushWord();
  if (!current.empty()) {
    lines.push_back(current);
  }
  if (lines.empty()) {
    lines.push_back("");
  }
  return lines;
}
