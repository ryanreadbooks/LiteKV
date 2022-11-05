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

  SKNode(int64_t score, const DynamicString& data, uint8_t lv = 1);

  SKNode(int64_t score, DynamicString&& data, uint8_t lv = 1);

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
  /**
   * @brief
   *
   * @param score
   * @param data
   * @param b
   * @return int 0 -> target == b; 1 -> target > b; -1 -> target < b
   */
  int operator()(int64_t score, const DynamicString& data, const SKNode& b) const {
    if (score == b.score) {
      size_t len1 = data.Length(), len2 = b.data.Length();
      int res1 = strncmp(data.Data(), b.data.Data(), std::min(len1, len2));
      if (res1 == 0) {
        /* further check */
        if (len1 == len2) {
          return 0;
        }
        return len1 < len2 ? -1 : 1;
      }
      return res1 < 0 ? -1 : 1;
    }
    return score < b.score ? -1 : 1;
  }

  int operator()(const SKNode& a, const SKNode& b) const {
    return this->operator()(a.score, a.data, b);
  }
};

class Skiplist {
public:
  Skiplist();

  ~Skiplist();

  Skiplist(const Skiplist&) = delete;

  Skiplist& operator=(const Skiplist&) = delete;

  Skiplist(Skiplist&&) = delete;

  // TODO: is this necessary?
  Skiplist(std::initializer_list<SKNode> list);

  // TODO: finish this
  Skiplist(std::initializer_list<std::pair<double, std::string>> list);

  /**
   * @brief Insert a new element into skiplist
   *
   * @param score
   * @param target
   * @return true
   * @return false
   */
  bool Insert(int64_t score, const DynamicString& target);

  /**
   * @brief Check if a element exists
   *
   * @param score
   * @param target
   * @return true
   * @return false
   */
  bool Contains(int64_t score, const DynamicString& target) const;

  /**
   * @brief Remove a element from the skiplist
   *
   * @param score
   * @param target
   * @return true
   * @return false
   */
  bool Remove(int64_t score, const DynamicString& target);

  /**
   * @brief Clear all elements in skiplist
   *
   */
  void Clear();

  /**
   * @brief Get number of elements
   *
   * @return size_t
   */
  size_t Count() const { return count_; }

  /**
   * @brief Check if skiplist is empty
   *
   * @return true
   * @return false
   */
  bool Empty() const { return count_ == 0; }

  /**
   * @brief Get the max level of skiplist
   *
   * @return size_t
   */
  size_t CurMaxLevel() const { return cur_max_level_; }

  /**
   * @brief Get the first node of skiplist
   *
   * @return SKNode*
   */
  SKNode* Begin() const { return head_->nexts[0]; }

  /**
   * @brief Get the last node of skiplist
   *
   * @return SKNode*
   */
  SKNode* End() const {
    if (head_->nexts[0] == nullptr) return nullptr;
    SKNode* cur = head_;
    for (int l = kMaxLevel - 1; l >= 0; --l) {
      while (cur->nexts[l]) {
        cur = cur->nexts[l];
      }
    }
    return cur;
  }

  /**
   * @brief Get element at index using operator[]
   *
   * @param index the index of element in skiplist, range in [0, count_ - 1]
   * @return SKNode& the underneath SKNode instance stored in skiplist instance
   */
  SKNode& operator[](size_t index);

  std::vector<SKNode*> GetAllSKNodes() const {
    std::vector<SKNode*> nodes;
    GatherAllNodes(nodes, false);
    return nodes;
  }

  friend std::ostream& operator<<(std::ostream& os, const Skiplist& sklist);

private:
  SKNode* Find(int64_t score, const DynamicString& target, int start,
               SKNode* path[kMaxLevel]) const;

  SKNode* Find(int64_t score, const DynamicString& target, int start) const;

  void GatherAllNodes(std::vector<SKNode*>& nodes, bool include_head = true) const;

private:
  SKNode* head_ = nullptr;
  size_t count_ = 0;
  uint8_t cur_max_level_ = 0;
  SKNodeComparator comparator_;
};

#endif  // __SKIPLIST_H__