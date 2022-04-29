#ifndef __STR_H__
#define __STR_H__

#include <iostream>
#include <memory>
#include <string>
#include <functional>
#include <cstring>

/**
 * @brief Static sized string
 * 
 */
class StaticString {
public:
  explicit StaticString() : len_(0), buf_(nullptr) {}

  explicit StaticString(const char* str) : 
    len_(strlen(str)), buf_(str) {}

  // TODO need optimization
  explicit StaticString(const std::string& str) : 
    len_(str.size()), buf_(str.data()) {}

  StaticString(const StaticString& other) : len_(other.len_), buf_(other.buf_) {}

  StaticString& operator=(const StaticString& other) {
    if (this == &other) {
      return *this;
    }
    // 不允许改变,所以指向同一块内存就行
    len_ = other.len_;
    buf_ = other.buf_;
    return *this;
  }

  ~StaticString() {
    len_ = 0;
    buf_ = nullptr;
  }

  inline size_t Length() const { return len_; }

  inline const char *Data() const { return buf_; }

  const std::string ToStdString() const { return std::string(buf_); }

  bool operator==(const StaticString& other) const {
    return strcmp(buf_, other.buf_) == 0 && len_ == other.len_;
  }

  bool operator!=(const StaticString& other) const {
    return strcmp(buf_, other.buf_) != 0 && len_ != other.len_;
  }

  bool operator<(const StaticString& other) const {
    return strcmp(buf_, other.buf_) == -1;
  }

  bool operator>(const StaticString& other) const {
    return strcmp(buf_, other.buf_) == 1;
  }

  friend std::ostream& operator<<(std::ostream& ss, const StaticString& s) {
    ss << s.Data();
    return ss;
  }

  size_t Hash() const {
    return std::hash<std::string>()(ToStdString());
  }

private:
  size_t len_ = 0;
  const char* buf_ = nullptr;
};


struct KeyHasher {
  std::size_t operator()(const StaticString& key) const {
    return key.Hash();
  }
};

struct KeyEqual {
  bool operator()(const StaticString& a, const StaticString& b) const {
    return a == b;
  }
};

typedef std::shared_ptr<StaticString> StaticStringPtr;

const static float kBufGrowFactor = 1.5;


/**
 * @brief Dynamic sized string
 * 
 */
class DynamicString {
public:
  DynamicString() = default;

  DynamicString(const char *str, uint32_t len) {
    if (str != nullptr) {
      /* alloc and copy memory to buf */
      alloc_ = (uint32_t)(sizeof(char) * (len + 1)); /* reserve more space */
      buf_ = (char *)malloc(alloc_);
      len_ = len;
      memcpy(buf_, str, len);
      buf_[len] = '\0';
    }
  }

  explicit DynamicString(const std::string &str) {
    if (!str.empty()) {
      alloc_ = (uint32_t)(sizeof(char) * (str.size() + 1));
      buf_ = (char *)malloc(alloc_);
      len_ = str.size();
      memcpy(buf_, str.data(), len_);
      buf_[len_] = '\0';
    }
  }

  DynamicString(DynamicString&& ds) noexcept {
    if (ds.buf_ != nullptr) {
      buf_ = ds.buf_;
      len_ = ds.len_;
      alloc_ = ds.alloc_;
      ds.buf_ = nullptr;
      ds.len_ = 0;
      ds.alloc_ = 0;
    }
  }

  ~DynamicString() {
    if (buf_) {
      free(buf_);
      buf_ = nullptr;
      len_ = 0;
      alloc_ = 0;
    }
  }

  DynamicString(const DynamicString &x) {
    /* construct totally new object with new allocated space */
    if (x.buf_ != nullptr) {
      buf_ = (char *)malloc(x.alloc_);
      memcpy(buf_, x.buf_, x.len_);
      len_ = x.len_;
      alloc_ = x.alloc_;
      buf_[len_] = '\0';
    }
  }

  DynamicString &operator=(const DynamicString &x) {
    if (this == &x) {
      return *this;
    }
    char * buf = (char *)malloc(x.alloc_);
    if (buf != nullptr) {
      memcpy(buf, x.buf_, x.len_);
      alloc_ = x.alloc_;
      len_ = x.len_;
      buf[len_] = '\0';
      free(buf_);
      buf_ = buf;
    }
    return *this;
  };

  inline const char *Data() const { return buf_; }

  inline uint32_t Length() const { return len_; }

  inline uint32_t Allocated() const { return alloc_; }

  inline uint32_t Available() const { return Allocated() - Length() - 1; }

  inline bool Empty() const { return len_ == 0; }

  void Append(const char *str, uint32_t len);

  void Append(const std::string &str);

  void Append(const DynamicString &ds);

  void Clear();

  void Reset(const char *str, uint32_t len);

  void Reset(const std::string &str);

  void Shrink();

  inline std::string ToStdString() {
    return std::string(buf_, len_);
  }

  int64_t TryConvertToInt64() const;

  friend std::ostream& operator<<(std::ostream& os, const DynamicString& ds) {
    if (ds.buf_) {
      os << ds.buf_;
    } else {
      os << "";
    }
    return os;
  }

  char& operator[](int idx) {
    if (idx >= 0) {
      if (idx >= (int)len_) {
        return *(buf_ + len_);
      }
      return *(buf_ + idx);
    } else { /* minus index supported */
      if (abs(idx) > (int)len_) {
        return *(buf_ + len_);
      } else {
        return *(buf_ + len_ + idx);
      }
    }
  }

  bool operator==(const DynamicString& other) const {
    return len_ == other.len_ && memcmp(buf_, other.buf_, len_) == 0;
  }

  bool operator==(const char* other) const {
    auto l = strlen(other);
    return len_ == l && memcmp(buf_, other, l) == 0;
  }

  bool operator!=(const DynamicString& other) const {
    return len_ != other.len_ || memcmp(buf_, other.buf_, len_) != 0;
  }

  bool operator!=(const char* other) const {
    auto l = strlen(other);
    return len_ != l || memcmp(buf_, other, l) != 0;
  }

  bool operator<(const DynamicString& other) const {
    return memcmp(buf_, other.buf_, std::min(len_, other.len_)) == -1;
  }

  bool operator<(const char* other) const {
    return memcmp(buf_, other, std::min((size_t)len_, strlen(other))) == -1;
  }

  bool operator>(const DynamicString& other) const {
    return memcmp(buf_, other.buf_, std::min(len_, other.len_)) == 1;
  }

  bool operator>(const char* other) const {
    return memcmp(buf_, other, std::min((size_t)len_, strlen(other))) == 1;
  }

private:
  /* real string length (4 bytes) */
  uint32_t len_ = 0;

  /* allocated bytes (4 bytes) */
  uint32_t alloc_ = 0;

  /* pointer to char buffer in heap (8 bytes) */
  char *buf_ = nullptr;
};

#endif  // __STR_H__
