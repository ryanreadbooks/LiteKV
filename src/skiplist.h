#ifndef __SKIPLIST_H__
#define __SKIPLIST_H__

#include <cassert>
#include <cctype>
#include <cstdint>

#include "str.h"
#include "valueobject.h"

#define SKIPLIST_MAX_LEVEL 32

/**
 *
 * level definition:
 *  |   next[3] -> level 4
 *  |   next[2] -> level 3
 *  |   next[1] -> level 2
 * _|_  next[0] -> level 1
 *
 */

/**
 * @brief template SkiplistNode implementation
 *
 * @tparam ElemT
 */
template <typename ElemT>
struct SkiplistNode {
  ElemT elem;
  /* pointer to the previous SkiplistNode */
  SkiplistNode* prev = nullptr;
  /* array of pointers to next SkiplistNodes */
  SkiplistNode** next = nullptr;
  /* the level of this node, level starting from 1 */
  uint8_t level;

  explicit SkiplistNode(const ElemT& elem, uint8_t level = 1)
      : elem(elem), next(new SkiplistNode*[level]), level(level) {}

  explicit SkiplistNode(ElemT&& elem, uint8_t level = 1)
      : elem(std::move(elem)), next(new SkiplistNode*[level]), level(level) {}

  /**
   * @brief free allocated space
   *
   */
  ~SkiplistNode() {
    if (next) {
      delete[] next;
      next = nullptr;
    }
  }

  /**
   * @brief Set node at level n
   *
   * @param n the level number
   * @param node the node to be set at level n
   */
  void SetNextNodeAtLevel(uint8_t n, SkiplistNode* node) {
    assert(n < SKIPLIST_MAX_LEVEL);
    next[n] = node;
  }

  /**
   * @brief Get the node at level n
   *
   * @param n the level number
   * @return SkiplistNode* the node at level n
   */
  SkiplistNode* GetNextNodeAtLevel(uint8_t n) {
    assert(n < SKIPLIST_MAX_LEVEL);
    return next[n];
  }
};

/**
 * @brief Skiplist implementation
 *
 */
template <typename ElemT, typename Compare>
class Skiplist {
public:
  typedef ElemT elem_type;

public:
  explicit Skiplist();

  ~Skiplist();

  bool Insert(const elem_type& elem);

  bool Insert(elem_type&& elem);

  template <typename... Args>
  bool Emplace(Args&&... args);

  bool Exists() const;

  bool Remove();

  size_t Size() const { return count_; }

private:
  int GenRandomLevel();

private:
  SkiplistNode<ElemT>* head_ = nullptr;
  SkiplistNode<ElemT>* tail_ = nullptr;
  Compare comp_;
  uint8_t cur_max_level_;
  size_t count_;
};

#define Template template<typename ElemT, typename Compare>

Template bool Skiplist<ElemT, Compare>::Insert(const elem_type& elem) { return false; }

Template bool Skiplist<ElemT, Compare>::Insert(elem_type&& elem) { return false; }

Template bool Skiplist<ElemT, Compare>::Exists() const { return false; }

Template bool Skiplist<ElemT, Compare>::Remove() { return false; }

#undef Template

#endif  // __SKIPLIST_H__