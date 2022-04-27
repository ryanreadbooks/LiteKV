#ifndef __DLIST_H__
#define __DLIST_H__

#include <vector>
#include <string>
#include "str.h"

class DList;

/* dlist is restricted to store dynamic string only */
typedef DynamicString ElemType;

const static uint16_t kBlockSize = 16;
const static uint16_t kRedundantFactor = 2;

#pragma pack(4)
struct Node {
  /* pointer to the previous node */
  Node *prev = nullptr;
  /* pointer to the next node */
  Node *next = nullptr;
  /* pointer to buffer array of size kBlockSize */
  ElemType *data = nullptr;
  /* space occupation */
  uint16_t occupied = 0;

  Node() {
    data = new ElemType[kBlockSize];
  }

  ~Node() {
    if (data != nullptr) {
      delete[] data;
      data = nullptr;
      prev = nullptr;
      next = nullptr;
      occupied = 0;
    }
  }
};

struct Iterator {
  Node *node = nullptr;
  ElemType *ptr = nullptr;
  
  bool operator==(const Iterator& b) const {
    return node == b.node && ptr == b.ptr;
  }
};

class DList {
public:
  DList();

  ~DList();

  void PushLeft(const char* val, uint32_t len);

  void PushRight(const char *val, uint32_t len);

  void PushLeft(const std::string &val);

  void PushRight(const std::string &val);

  ElemType PopLeft();

  ElemType PopRight();

  inline size_t Length() const { return len_; }

  inline bool Empty() const { return len_ == 0; }

  inline size_t NodeNum() const { return n_nodes_; }

  ElemType& operator[](size_t idx);

  std::vector<std::string> RangeAsStdStringVector();

  std::vector<std::string> RangeAsStdStringVector(int start, int finish);

  std::vector<ElemType> RangeAsDynaStringVector();

  std::vector<ElemType> RangeAsDynaStringVector(int start, int finish);

  inline Node *Front() const { return head_; }

  inline Node *Back() const { return tail_; }

  void FreeRedundantNodes();

private:
  inline Node *Head() const { return head_; }
  inline Node *Tail() const { return tail_; }
  inline ElemType *HeadElem() const { return begin_.ptr; }
  inline ElemType *TailElem() const { return end_.ptr; }

  void FreeNodes();

  inline Node *NewNode() { return new Node; }

  std::tuple<Node*, int, int> NodeAtIndex(size_t idx);

private:
  Iterator begin_;
  Iterator end_;
  Node *head_;
  Node *tail_;
  size_t len_ = 0;
  size_t n_nodes_ = 2;
};

#endif // __DLIST_H__