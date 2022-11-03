#include "skiplist.h"

static int RandIntValue(int from, int to) {
  static std::mt19937_64 engine(time(nullptr));
  return std::uniform_int_distribution<int>(from, to)(engine);
}

static int ZeroOrOne() { return RandIntValue(0, 1); }

/**
 * Generate random level for skiplist nodes
 * @return
 */
static size_t GenRandomLevel() {
  size_t level = 1;
  while (level < kMaxLevel && ZeroOrOne()) {
    level++;
  }
  return level;
}

SKNode::SKNode(int64_t score, const DynamicString& data, uint8_t level)
    : score(score), data(data), nexts(new SKNode*[level]), level(level) {
  for (int l = 0; l < level; ++l) {
    nexts[l] = nullptr;
  }
}

SKNode::SKNode(int64_t score, DynamicString&& data, uint8_t level)
    : score(score), data(std::move(data)), nexts(new SKNode*[level]), level(level) {
  for (int l = 0; l < level; ++l) {
    nexts[l] = nullptr;
  }
}

SKNode::~SKNode() {
  if (nexts) {
    delete[] nexts;
    nexts = nullptr;
  }
}

// TODO finish skiplist 
Skiplist::Skiplist() : head_(new SKNode(0, DynamicString(""), kMaxLevel)) {}

Skiplist::Skiplist(std::initializer_list<SKNode> list)
    : head_(new SKNode(0, DynamicString(""), kMaxLevel)) {}

bool Skiplist::Insert(int64_t score, const DynamicString& target) {}

bool Skiplist::Exists(int64_t score, const DynamicString& target) {}

bool Skiplist::Remove(int64_t score, const DynamicString& target) {}

SKNode* Skiplist::Find(int64_t, const DynamicString& target) {}
