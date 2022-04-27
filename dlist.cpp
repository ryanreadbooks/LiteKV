#include "dlist.h"
#include <iostream>

DList::DList() {
  /* we create 2 nodes at initialization */
  Node *node1 = new Node;
  Node *node2 = new Node;
  node1->next = node2;
  node2->prev = node1;
  begin_.node = node1;
  end_.node = node2;
  head_ = node1;
  tail_ = node2;
}

DList::~DList() { /* free all nodes in heap */
  FreeNodes();
}

void DList::PushLeft(const char *val, uint32_t len) {
  if (begin_.ptr == nullptr) { /* not inited */
    begin_.ptr = begin_.node->data + kBlockSize - 1;
    if (end_.ptr == nullptr) {
      end_.ptr = begin_.ptr;
      end_.node = begin_.node;
    }
  } else {
    if (begin_.ptr == begin_.node->data) { /* already at the left edge of buffer */
      if (begin_.node->prev == nullptr) {
        /* create new node */
        Node *newnode = NewNode();
        ++n_nodes_;
        head_ = newnode;
        newnode->next = begin_.node;
        begin_.node->prev = newnode;
        begin_.node = newnode;
        begin_.ptr = newnode->data + kBlockSize - 1;
      } else {
        /* go to previous not null node */
        begin_.ptr = begin_.node->prev->data + kBlockSize - 1;
        begin_.node = begin_.node->prev;
      }
    } else {
      begin_.ptr--;
    }
  }
  begin_.ptr->Reset(val, len);
  begin_.node->occupied++;
  ++len_;
}

void DList::PushRight(const char *val, uint32_t len) {
  if (end_.ptr == nullptr) { /* not inited */
    end_.ptr = end_.node->data;
    if (begin_.ptr == nullptr) {
      begin_.ptr = end_.ptr;
      begin_.node = end_.node;
    }
  } else {
    if (end_.ptr == end_.node->data + kBlockSize - 1) { /* already at the right edge of buffer */
      if (end_.node->next == nullptr) {
        Node *newnode = NewNode();
        ++n_nodes_;
        tail_ = newnode;
        newnode->prev = end_.node;
        end_.node->next = newnode;
        end_.node = newnode;
        end_.ptr = newnode->data;
      } else {
        /* go to next not null node */
        end_.ptr = end_.node->next->data;
        end_.node = end_.node->next;
      }
    } else {
      end_.ptr++;
    }
  }
  end_.ptr->Reset(val, len);
  end_.node->occupied++;
  ++len_;
}

void DList::PushLeft(const std::string &val) {
  PushLeft(val.data(), val.size());
}

void DList::PushRight(const std::string &val) {
  PushRight(val.data(), val.size());
}

ElemType DList::PopLeft() {
  /* delete from left and return value */
  if (Empty())
  {
    return std::move(ElemType());
  }
  ElemType* ret = begin_.ptr;
  if (begin_.ptr == end_.ptr) { /* only one element left */
    begin_.node->occupied--;
    begin_.ptr = nullptr;
    end_.ptr = nullptr;
  } else {
    /* move left pointer and perform lazy deletion */
    /* already at the right edge of buffer */
    if (begin_.ptr == begin_.node->data + kBlockSize - 1) { 
      /* next node is not null */
      if (begin_.node->next != nullptr) {
        begin_.node->occupied--;
        begin_.node = begin_.node->next;
        begin_.ptr = begin_.node->data;
      }
    } else {
      begin_.ptr++; /* still in the same node */
      begin_.node->occupied--;
    }
  }
  --len_;
  return std::move(*ret);
}

ElemType DList::PopRight() {
  /* delete from right and return value */
  if (Empty()) {
    return std::move(ElemType());
  }
  ElemType *ret = end_.ptr;
  if (begin_.ptr == end_.ptr) { /* only one element left */
    end_.node->occupied--;
    begin_.ptr = nullptr;
    end_.ptr = nullptr;
  } else {
    if (end_.ptr == end_.node->data) {
      if (end_.node->prev != nullptr) {
        end_.node->occupied--;
        end_.node = end_.node->prev;
        end_.ptr = end_.node->data + kBlockSize - 1;
      }
    } else {
      end_.ptr--;
      end_.node->occupied--;
    }
  }
  --len_;
  return std::move(*ret);
}

ElemType& DList::operator[](size_t idx) {
  /* index range from [0, len_ - 1], only positive index supported */
  if (idx >= len_) {
    throw std::out_of_range("index out of range");
  }
  if (begin_.node->occupied > idx) { 
    /* index value in the same node */
    return *(begin_.ptr + idx);
  } else {
    /* go to the corresponding node in O(n) complexity */
    auto package = NodeAtIndex(idx);
    Node *tmp = std::get<0>(package);
    int offset = std::get<2>(package);
    return *(tmp->data + offset);
  }
}

std::vector<std::string> DList::RangeAsStdStringVector() {
  // if (Empty()) {
  //   return {};
  // }
  // std::vector<std::string> values;
  // values.reserve(len_);
  // Node *tmp = begin_.node;
  // ElemType *elem = begin_.ptr;

  // while (tmp && tmp->occupied > 0 && elem) {
  //   for (int i = 0; i < tmp->occupied; i++) {
  //     values.emplace_back(elem->ToStdString());
  //     elem++;
  //   }
  //   tmp = tmp->next;
  //   if (tmp) {
  //     elem = tmp->data;
  //   }
  // }
  // return values;
  return RangeAsStdStringVector(0, (int)(len_ - 1));
}

// std::vector<std::string> DList::RangeAsStdStringVector(int start, int finish) {
//   if (Empty() || start > finish) {
//     return {};
//   }
//   finish = std::min(finish, (int)(len_ - 1));  /* make sure finish does not out of bound */
//   std::vector<std::string> values;

//   int distance = finish - start + 1;
//   /* go to the corresponding node of the indexed item */
//   auto package = NodeAtIndex(start);
//   Node *tmp = std::get<0>(package);
//   int n_cross = std::get<1>(package);
//   int offset = std::get<2>(package);

//   ElemType *elem = tmp->data + offset;

//   while (tmp && tmp->occupied > 0 && elem && distance > 0) {
//     for (int i = 0; i < tmp->occupied && distance > 0; i++) {
//       values.emplace_back(elem->ToStdString());
//       elem++;
//       distance--;
//     }
//     tmp = tmp->next;
//     if (tmp) {
//       elem = tmp->data;
//     }
//   }
//   return values;
// }

#define RANGE_FUNC_HELPER(funcname, rettype, sentence)                         \
  std::vector<rettype> DList::funcname(int start, int finish) {                \
    if (Empty() || start > finish) {                                           \
      return {};                                                               \
    }                                                                          \
    finish = std::min(finish, (int)(len_ - 1));                                \
    std::vector<rettype> values;                                               \
    int distance = finish - start + 1;                                         \
    auto package = NodeAtIndex(start);                                         \
    Node *tmp = std::get<0>(package);                                          \
    int n_cross = std::get<1>(package);                                        \
    int offset = std::get<2>(package);                                         \
    ElemType *elem = tmp->data + offset;                                       \
    while (tmp && tmp->occupied > 0 && elem && distance > 0) {                 \
      for (int i = 0; i < tmp->occupied && distance > 0; i++) {                \
        values.emplace_back(sentence);                                         \
        elem++;                                                                \
        distance--;                                                            \
      }                                                                        \
      tmp = tmp->next;                                                         \
      if (tmp) {                                                               \
        elem = tmp->data;                                                      \
      }                                                                        \
    }                                                                          \
    return values;                                                             \
  }

RANGE_FUNC_HELPER(RangeAsStdStringVector, std::string, elem->ToStdString());

RANGE_FUNC_HELPER(RangeAsDynaStringVector, ElemType, *elem);

#undef RANGE_FUNC_HELPER

std::vector<ElemType> DList::RangeAsDynaStringVector() {
  // if (Empty()) {
  //   return {};
  // }
  // std::vector<ElemType> values;
  // values.reserve(len_);
  // Node *tmp = begin_.node;
  // ElemType *elem = begin_.ptr;

  // while (tmp && tmp->occupied > 0 && elem) {
  //   for (int i = 0; i < tmp->occupied; i++) {
  //     values.emplace_back(*elem);
  //     elem++;
  //   }
  //   tmp = tmp->next;
  //   if (tmp) {
  //     elem = tmp->data;
  //   }
  // }
  // return values;
  return RangeAsDynaStringVector(0, (int)(len_ - 1));
}

// std::vector<ElemType> DList::RangeAsDynaStringVector(int start, int finish) {
//   if (Empty() || start > finish) {
//     return {};
//   }
//   finish = std::min(finish, (int)(len_ - 1));  /* make sure finish does not out of bound */
//   std::vector<ElemType> values;
//   int distance = finish - start + 1;
//   /* go to the corresponding node of the indexed item */
//   auto package = NodeAtIndex(start);
//   Node *tmp = std::get<0>(package);
//   int n_cross = std::get<1>(package);
//   int offset = std::get<2>(package);

//   ElemType *elem = tmp->data + offset;

//   while (tmp && tmp->occupied > 0 && elem && distance > 0) {
//     for (int i = 0; i < tmp->occupied && distance > 0; i++) {
//       values.emplace_back(*elem);
//       elem++;
//       distance--;
//     }
//     tmp = tmp->next;
//     if (tmp) {
//       elem = tmp->data;
//     }
//   }
//   return values;
// }

void DList::FreeNodes() {
  Node *tmp = head_;
  Node *deleting = tmp;
  while (tmp) {
    tmp = tmp->next;
    delete deleting;
    deleting = tmp;
  }
  head_ = nullptr;
  tail_ = nullptr;
}

void DList::FreeRedundantNodes() {
  /* if len_ is much more small than the allocated space, 
  *  we consider redundant nodes shoule be removed */
  int capacity = n_nodes_ * kBlockSize;
  if (capacity >= len_ * kRedundantFactor) {
    /* remove redundant nodes from the left */
    Node *tmp = head_;
    Node *deleting = tmp;
    while (tmp && tmp->occupied == 0 && tmp != begin_.node->prev)
    {
      tmp = tmp->next;
      delete deleting;
      deleting = tmp;
      deleting->prev = nullptr; /* point to null */
      --n_nodes_;
    }
    head_ = tmp;
    /* remove redundant nodes from the right */
    tmp = tail_;
    deleting = tmp;
    while (tmp && tmp->occupied == 0 && tmp != end_.node->next) {
      tmp = tmp->prev;
      delete deleting;
      deleting = tmp;
      deleting->next = nullptr; /* point to null */
      --n_nodes_;
    }
    tail_ = tmp;
  }
}

std::tuple<Node*, int, int> DList::NodeAtIndex(size_t idx) {
  if (begin_.node->occupied > idx) {
    /* index does not need to jump cross nodes */
    return std::make_tuple(begin_.node, 0, idx);
  }
  int diff = idx - begin_.node->occupied;
  int n_cross = diff / kBlockSize;
  int offset = diff % kBlockSize;
  Node *tmp = begin_.node;
  for (int i = 0; i <= n_cross; i++) {
    tmp = tmp->next;
  }
  return std::make_tuple(tmp, n_cross, offset);
}