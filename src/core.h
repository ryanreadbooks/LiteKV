#ifndef __CORE_H__
#define __CORE_H__

#include <iostream>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <mutex>

#include "str.h"
#include "valueobject.h"

static const int kBucketSize = 512;

using HashMap = std::unordered_map<Key, ValueObjectPtr, KeyHasher, KeyEqual>;
using LockGuard = std::unique_lock<std::mutex>;

struct Bucket {
  /* mutex for every bucket */
  mutable std::mutex mtx;

  /* every bucket has a hashmap */
  HashMap content;
};

const static int kOkCode = 0x02;
const static int kFailCode = 0x03;
const static int kKeyNotFoundCode = 0x04;
const static int kWrongTypeCode = 0x08;

class KVContainer
{
public:
  explicit KVContainer() {}

  KVContainer(const KVContainer &) = delete;

  KVContainer &operator=(const KVContainer &) = delete;

  ~KVContainer() {}

public:
  int QueryObjectType(const Key &key);

  int QueryObjectType(const std::string &key) {
    return QueryObjectType(Key(key));
  }

  bool KeyExists(const Key &key);

  bool KeyExists(const std::string& key) {
    return KeyExists(Key(key));
  }

  size_t NumItems() const;

  /**
   * @brief set integer
   * 
   */
  bool SetInt(const Key& key, int64_t intval, uint64_t exp_time = 0);

  bool SetInt(const std::string &key, int64_t intval, uint64_t exp_time = 0);

  /**
   * @brief set string
   * 
   */
  bool SetString(const Key& key, const std::string& value, uint64_t exp_time = 0);

  bool SetString(const std::string &key, const std::string &value, uint64_t exp_time = 0);

  /**
   * @brief get integer or string, others will fail
   * 
   */
  ValueObjectPtr Get(const Key& key, int& errcode);

  ValueObjectPtr Get(const std::string &key, int& errcode);

  bool Delete(const Key &key);

  bool Delete(const std::string &key);

  /**
   * @brief push item into list to the left
   * 
   * @return true 
   * @return false 
   */
  bool LeftPush(const Key& key, const std::string& val, int& errcode);

  bool LeftPush(const std::string& key, const std::string& val, int& errcode) {
    return LeftPush(Key(key), val, errcode);
  }

  /**
   * @brief push item into list to the right 
   * 
   * @return true 
   * @return false 
   */
  bool RightPush(const Key& key, const std::string& val, int& errcode);

  bool RightPush(const std::string& key, const std::string& val, int& errcode) {
    return RightPush(Key(key), val, errcode);
  }

  /**
   * @brief pop item from left, valid for OBJECT_LIST type
   * 
   */
  DynamicString LeftPop(const Key& key, int& errcode);

  DynamicString LeftPop(const std::string& key, int& errcode) {
    return LeftPop(Key(key), errcode);
  }

  /**
   * @brief pop item from right, valid for OBJECT_LIST type
   * 
   */
  DynamicString RightPop(const Key& key, int& errcode);

  DynamicString RightPop(const std::string& key, int& errcode) {
    return RightPop(Key(key), errcode);
  }

  /**
   * @brief get all items from a list
   * 
   */
  std::vector<DynamicString> ListRange(const Key& key, int begin, int end, int& errcode);

  std::vector<DynamicString> ListRange(const std::string& key, int begin, int end, int& errcode) {
    return ListRange(Key(key), begin, end, errcode);
  }

  /**
   * @brief get the number of items in a list
   * 
   * @return size_t 
   */
  size_t ListLen(const Key& key, int& errcode);

  size_t ListLen(const std::string& key, int& errcode) {
    return ListLen(Key(key), errcode);
  }

  /**
   * @brief retrieve item at index from a list (index starting from 0)
   * 
   */
  DynamicString ListItemAtIndex(const Key& key, int index, int& errcode);

  DynamicString ListItemAtIndex(const std::string &key, int index, int& errcode) {
    return ListItemAtIndex(Key(key), index, errcode);
  }

  bool ListSetItemAtIndex(const Key &key, int index, const std::string &val, int &errcode);

  bool ListSetItemAtIndex(const std::string& key, int index, const std::string& val, int &errcode) {
    return ListSetItemAtIndex(Key(key), index, val, errcode);
  }

private:
  inline Bucket &GetBucket(const Key &key) {
    size_t bucket_idx = key.Hash() % kBucketSize;
    return bucket_[bucket_idx];
  }

  bool ListPushAux(const Key &key, const std::string &val, bool leftpush, int &errcode);

  DynamicString ListPopAux(const Key &key, bool leftpop, int &errcode);

private:
  mutable std::mutex mtx_;
  std::array<Bucket, kBucketSize> bucket_;
};

#endif  // __CORE_H__
