#include "str.h"
#include <iostream>

void DynamicString::Append(const char *str, uint32_t addlen) {
  if (buf_ == nullptr) {
    /* allocate buffer */
    alloc_ = sizeof(char) * (addlen + 1);
    buf_ = (char *)malloc(alloc_);
    if (buf_ == nullptr) {  /* malloc fail */
      buf_ = nullptr;
      return;
    }
  }
  uint32_t free_bytes = alloc_ - len_ - 1; /* remaining space */
  if (free_bytes >= addlen) {
    memcpy(buf_ + len_, str, addlen);
  } else {
    /* free space not enough, allocate more */
    alloc_ = uint32_t((len_ + addlen) * kBufGrowFactor + 1);
    char *tmp = (char *)realloc(buf_, alloc_);
    if (tmp == nullptr) {
      return;
    }
    /* if succeed, old buf_ space will be freed by realloc */
    buf_ = tmp;
    /* append new content to new buffer */
    memcpy(buf_ + len_, str, addlen);
  }
  len_ += addlen;
  buf_[len_] = '\0';
}

void DynamicString::Append(const std::string &str) {
  Append(str.data(), str.size());
}

void DynamicString::Append(const DynamicString &ds) {
  Append(ds.Data(), ds.Length());
}

void DynamicString::Clear() {
  /* set buffer to zero, but do not free space */
  if (len_ == 0 || alloc_ == 0 || buf_ ==  nullptr) {
    return;
  }
  len_ = 0;
  memset(buf_, 0, alloc_);
  buf_[len_] = '\0';
}

void DynamicString::Reset(const char *str, uint32_t len) {
  if (buf_ == nullptr) {
    uint32_t allocated = len + 1;
    buf_ = (char *)malloc(sizeof(char) * allocated);
    if (buf_ == nullptr) {
      buf_ = nullptr;
      return;
    }
    alloc_ = allocated;
  }
  Clear();
  Append(str, len);
}

void DynamicString::Reset(const std::string &str) {
  Reset(str.data(), str.size());
}

void DynamicString::Shrink() {
  if (buf_) {
    char *tmp = (char *)realloc(buf_, len_ + 1);
    if (tmp == NULL) {
      return;
    }
    alloc_ = len_ + 1;
    buf_ = tmp;
  }
}

//TODO convert to int64 max number is 20 - 1 = 19 (negative sign should be considered)
int64_t DynamicString::TryConvertToInt64() const {
  return 0;
}