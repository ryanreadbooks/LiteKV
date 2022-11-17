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

SKNode::SKNode(int64_t score, const DynamicString& data, uint8_t lv)
    : score(score), data(data), nexts(new SKNode*[lv]), level(lv) {
  for (int l = 0; l < level; ++l) {
    nexts[l] = nullptr;
  }
}

SKNode::SKNode(int64_t score, DynamicString&& data, uint8_t lv)
    : score(score), data(std::move(data)), nexts(new SKNode*[lv]), level(lv) {
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

Skiplist::Skiplist() : head_(new SKNode(INT64_MIN, DynamicString(""), kMaxLevel)) {}

Skiplist::~Skiplist() {
  /* we need to free all nodes in the skiplist */
  std::vector<SKNode*> nodes;
  GatherAllNodes(nodes, true);
  for (auto& node : nodes) {
    delete node;
    node = nullptr;
  }
  head_ = nullptr;
  cur_max_level_ = 0;
  count_ = 0;
}

Skiplist::Skiplist(std::initializer_list<SKNode> list)
    : head_(new SKNode(0, DynamicString(""), kMaxLevel)) {}

Skiplist::Skiplist(std::initializer_list<std::pair<double, std::string>> list)
    : head_(new SKNode(0, DynamicString(""), kMaxLevel)) {}

// FIXME: optimize args copy overhead, target may be copied many times
bool Skiplist::Insert(int64_t score, const DynamicString& target) {
  if (head_ == nullptr) return false;
  if (head_->nexts[0] == nullptr) {
    /* we do not have nodes yet,
     * we should insert the first node here whose level is specified to be 1 */
    SKNode* new_node = new (std::nothrow) SKNode(score, target, 1);
    if (new_node == nullptr) return false;
    head_->nexts[0] = new_node;
    new_node->prev = head_;
    ++count_;
    cur_max_level_ = 1;
    return true;
  }
  /* determine a level for new inserted node */
  size_t level = GenRandomLevel();
  SKNode* path[kMaxLevel] = {nullptr}; /* store the path when finding the position for insertion */
  SKNode* cur = Find(score, target, level - 1, path);
  if (cur == nullptr || (cur->nexts[0] && comparator_(score, target, *cur->nexts[0]) == 0)) {
    /* the same node exists */
    return false;
  }
  SKNode* new_node = new (std::nothrow) SKNode(score, target, level);
  if (new_node == nullptr) return false;
  SKNode* after = cur->nexts[0];
  /* adjust nexts pointers */
  for (size_t l = 0; l < level; ++l) {
    new_node->nexts[l] = path[l]->nexts[l];
    path[l]->nexts[l] = new_node;
  }
  /* adjust pre pointer */
  if (after) {
    after->prev = new_node;
  }
  new_node->prev = cur;
  cur_max_level_ = std::max(cur_max_level_, (uint8_t)level);
  ++count_;
  return true;
}

bool Skiplist::Contains(int64_t score, const DynamicString& target) const {
  if (head_ == nullptr || head_->nexts[0] == nullptr) {
    return false;
  }
  SKNode* node = Find(score, target, cur_max_level_ - 1);
  if (node->nexts[0] == nullptr || comparator_(score, target, *node->nexts[0]) != 0) {
    return false;
  }
  return true;
}

bool Skiplist::Remove(int64_t score, const DynamicString& target) {
  if (head_ == nullptr || head_->nexts[0] == nullptr) {
    return false;
  }
  SKNode* path[kMaxLevel] = {nullptr};
  SKNode* cur = Find(score, target, cur_max_level_ - 1, path);
  /* cur is the previous node of the deleting node, 
  which means that cur->nexts[0] will be removed */
  if (cur == nullptr || cur->nexts[0] == nullptr) return false;
  if (comparator_(score, target, *cur->nexts[0]) != 0) return false;

  /* this is the node to be deleted */
  SKNode* del_node = cur->nexts[0];
  /* we should adjust the next pointers for cur node */
  for (int l = cur_max_level_ - 1; l >= 0; --l) {
    if (path[l] && path[l]->nexts[l] && comparator_(score, target, *path[l]->nexts[l]) == 0) {
      path[l]->nexts[l] = del_node->nexts[l];
    }
  }
  /* adjust next and previous pointer */
  if (del_node->nexts && del_node->nexts[0] != nullptr) {
    del_node->nexts[0]->prev = cur;
  }
  /* we may need to adjust the current level of the whole skiplist */
  if (del_node->level == cur_max_level_) {
    /* further check */
    while (head_->nexts[cur_max_level_ - 1] == nullptr && cur_max_level_ > 0) {
      --cur_max_level_;
    }
  }
  delete del_node;
  --count_;
  return true;
}

void Skiplist::Clear() {
  if (head_ == nullptr || count_ == 0) {
    return;
  }
  std::vector<SKNode*> nodes;
  GatherAllNodes(nodes, false);
  for (auto& node : nodes) {
    delete node;
    node = nullptr;
  }
  for (int l = 0; l < kMaxLevel; ++l) {
    head_->nexts[l] = nullptr;
  }
  cur_max_level_ = 0;
  count_ = 0;
}

SKNode& Skiplist::operator[](size_t idx) {
  if (idx >= count_ || idx < 0) {
    throw std::out_of_range("skiplist index out of range");
  }
  /* TODO we should find the corresponding element at idx */

  return *head_->nexts[0];
}

SKNode* Skiplist::Find(int64_t score, const DynamicString& target, int start,
                       SKNode* path[kMaxLevel]) const {
  SKNode* cur = head_;
  /* search from top to down */
  for (int l = start; l >= 0; --l) {
    /* search from left to right */
    while (cur->nexts[l] && comparator_(score, target, *cur->nexts[l]) > 0) {
      cur = cur->nexts[l];
    }
    path[l] = cur;
  }
  /**
   * 1. if target can be found, then cur is the previous node the target node;
   * 2. if target cannot be found, then cur is the node that will be followed by the target node.
   */
  return cur;
}

SKNode* Skiplist::Find(int64_t score, const DynamicString& target, int start) const {
  SKNode* cur = head_;
  /* search from top to down */
  for (int l = start; l >= 0; --l) {
    /* search from left to right */
    while (cur->nexts[l] && comparator_(score, target, *cur->nexts[l]) > 0) {
      cur = cur->nexts[l];
    }
  }
  /**
   * 1. if target can be found, then cur is the previous node the target node;
   * 2. if target cannot be found, then cur is the node that will be followed by the target node.
   */
  return cur;
}

void Skiplist::GatherAllNodes(std::vector<SKNode*>& nodes, bool include_head) const {
  SKNode* cur;
  if (include_head) {
    cur = head_;
    nodes.reserve(count_ + 1);
  } else {
    cur = head_->nexts[0];
    nodes.reserve(count_);
  }
  while (cur && cur->nexts) {
    nodes.push_back(cur);
    cur = cur->nexts[0];
  }
}

std::ostream& operator<<(std::ostream& os, const Skiplist& sklist) {
  SKNode* p = sklist.head_;
  size_t cnt = sklist.count_;
  size_t idx = 1;
  while (p) {
    p = p->nexts[0];
    if (p) {
      /* print node score, data and level */
      os << '[' << p->score << ", " << p->data << ", " << std::to_string(p->level) << ']';
      if (idx != cnt) {
        os << " -> ";
      }
      ++idx;
    }
  }
  return os;
}