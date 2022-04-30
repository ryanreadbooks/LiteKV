#include <iostream>
#include <exception>
#include "str.h"

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

int64_t DynamicString::TryConvertToInt64() const {
  /* int64_max = 9223372036854775807　(19 chars)
   * int64_min = -9223372036854775808 (20 chars)
   * */
  if (buf_ == nullptr) {
    throw std::invalid_argument("can not convert null string to int");
  }
  if (len_ > 20) {
    throw std::overflow_error("string length too large to convert to int");
  }
  char *tmp;
  int64_t ans = strtoll(buf_, &tmp, 10);
  /* check if number overflow */
  if ((ans == INT64_MAX && errno == ERANGE) || (ans == INT64_MIN && errno == ERANGE)) {
    throw std::overflow_error("number overflow");
  }
  if (std::to_string(ans).size() != len_) {
    throw std::invalid_argument("can not convert full string to int");
  }
  return ans;
}

bool CanConvertToInt64(const std::string& str, int64_t& ans) {
  if (str.empty() || str.size() > 20) {
    return false;
  }
  try {
    int64_t tmp = std::stoll(str);
    if (std::to_string(tmp)== str) {
      ans = tmp;
      return true;
    }
  } catch (const std::exception& ex) {
    return false;
  }
  return false;
}

bool CanConvertToInt32(const std::string& str, int& ans) {
  if (str.empty() || str.size() > 20) {
    return false;
  }
  try {
    int tmp = std::stoi(str);
    if (std::to_string(tmp)== str) {
      ans = tmp;
      return true;
    }
  } catch (const std::exception& ex) {
    return false;
  }
  return false;
}
