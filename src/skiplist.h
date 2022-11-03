#ifndef __SKIPLIST_H__
#define __SKIPLIST_H__

#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <random>

#include "str.h"
#include "valueobject.h"

static constexpr uint8_t kMaxLevel = 32;

struct SKNode {
  int64_t score = 0l;
  DynamicString data;

  SKNode* prev = nullptr;
  SKNode** nexts = nullptr;
  const uint8_t level = 1;

  SKNode() = delete;

  explicit SKNode(int64_t score, const DynamicString& data, uint8_t level = 1);

  explicit SKNode(int64_t score, DynamicString&& data, uint8_t level = 1);

  ~SKNode();

  bool operator==(const SKNode& b) const {
    return this->data.Length() == b.data.Length() &&
           strncmp(this->data.Data(), b.data.Data(), this->data.Length()) == 0;
  }
};

/**
 * Comparator for SKNode
 */
struct SKNodeComparator {
  bool operator()(const SKNode& a, const SKNode& b) const {
    if (a.score == b.score) {
      return strncmp(a.data.Data(), b.data.Data(), std::min(a.data.Length(), b.data.Length())) < 0;
    }
    return a.score < b.score;
  }

  bool operator()(int64_t score, const DynamicString& data, const SKNode& b) const {
    if (score == b.score) {
      return strncmp(data.Data(), b.data.Data(), std::min(data.Length(), b.data.Length())) < 0;
    }
    return score < b.score;
  }
};

class Skiplist {
public:
  Skiplist();

  Skiplist(const Skiplist&) = delete;

  Skiplist& operator=(const Skiplist&) = delete;

  Skiplist(Skiplist&&) = delete;

  Skiplist(std::initializer_list<SKNode> list);

  bool Insert(int64_t score, const DynamicString& target);

  bool Exists(int64_t score, const DynamicString& target);

  bool Remove(int64_t score, const DynamicString& target);

private:
  SKNode* Find(int64_t, const DynamicString& target);

private:
  SKNode* head_ = nullptr;
  size_t count_ = 0;
  uint8_t cur_max_level_ = 0;
  SKNodeComparator compare_;
};

#endif  // __SKIPLIST_H__