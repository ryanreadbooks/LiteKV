#ifndef __CORE_H__
#define __CORE_H__

#include <iostream>
#include <unordered_map>
#include <queue>
#include <vector>
#include <fstream>
#include <mutex>

#include "str.h"
#include "dict.h"
#include "valueobject.h"

static constexpr int EVICTION_POLICY_RANDOM = 0;
static constexpr int EVICTION_POLICY_LRU = 1;

static constexpr int kBucketSize = 512;
static constexpr int kNumEvictCandidates = 16;

using HashMap = std::unordered_map<Key, ValueObjectPtr, KeyHasher, KeyEqual>;
using LockGuard = std::unique_lock<std::mutex>;

struct Bucket {
  /* mutex for every bucket */
  mutable std::mutex mtx;

  /* every bucket has a hashmap */
  HashMap content;
};

constexpr static int kOkCode = 200;
constexpr static int kFailCode = 400;
constexpr static int kKeyNotFoundCode = 401;
constexpr static int kWrongTypeCode = 402;
constexpr static int kOutOfRangeCode = 403;
constexpr static int kOverflowCode = 404;

class KVContainer {
public:
  explicit KVContainer() = default;

  KVContainer(const KVContainer &) = delete;

  KVContainer &operator=(const KVContainer &) = delete;

  ~KVContainer() {}

public:
  /* container overview */
  std::vector<DynamicString> Overview() const;

  std::vector<std::string> KeyEviction(int policy, size_t n);

  int QueryObjectType(const Key &key);

  int QueryObjectType(const std::string &key) {
    return QueryObjectType(Key(key));
  }

  bool KeyExists(const Key &key);

  int KeyExists(const std::vector<std::string> &keys);

  bool KeyExists(const std::string &key) {
    return KeyExists(Key(key));
  }

  size_t NumItems() const;

  std::vector<std::string> RecoverCommandFromValue(const std::string& key, int &errcode);

  /**
   * @brief set integer
   * 
   */
  bool SetInt(const Key &key, int64_t intval);

  bool SetInt(const std::string &key, int64_t intval) {
    return SetInt(Key(key), intval);
  }

  int64_t IncrIntBy(const Key& key, int64_t increment, int& errcode);

  int64_t IncrIntBy(const std::string& key, int64_t increment, int& errcode) {
    return IncrIntBy(Key(key), increment, errcode);
  }

  int64_t IncrInt(const Key& key, int& errcode) {
    return IncrIntBy(key, 1, errcode);
  }

  int64_t IncrInt(const std::string& key, int& errcode) {
    return IncrInt(Key(key), errcode);
  }

  int64_t DecrIntBy(const Key& key, int64_t decrement, int& errcode);

  int64_t DecrIntBy(const std::string& key, int64_t increment, int& errcode) {
    return DecrIntBy(Key(key), increment, errcode);
  }

  int64_t DecrInt(const Key& key, int& errcode) {
    return DecrIntBy(key, 1, errcode);
  }

  int64_t DecrInt(const std::string& key, int& errcode) {
    return DecrInt(Key(key), errcode);
  }

  /**
   * @brief set string
   * 
   */
  bool SetString(const Key &key, const std::string &value);

  bool SetString(const std::string &key, const std::string &value) {
    return SetString(Key(key), value);
  }

  size_t StrLen(const Key &key, int &errcode);

  size_t StrLen(const std::string &key, int &errcode) {
    return StrLen(Key(key), errcode);
  }

  /**
   * @brief get integer or string, others will fail
   * 
   */
  ValueObjectPtr Get(const Key &key, int &errcode);

  ValueObjectPtr Get(const std::string &key, int &errcode);

  bool Delete(const Key &key);

  bool Delete(const std::string &key) {
    return Delete(Key(key));
  }

  int Delete(const std::vector<std::string> &keys);

  size_t Append(const Key &key, const std::string &val, int &errcode);

  size_t Append(const std::string &key, const std::string &val, int &errcode) {
    return Append(Key(key), val, errcode);
  }

  /******************** List operation ********************/

  /**
   * @brief push item into list to the left
   * 
   * @return true 
   * @return false 
   */
  bool LeftPush(const Key &key, const std::string &val, int &errcode);

  size_t LeftPush(const std::string &key, const std::vector<std::string> &values, int &errcode);

  bool LeftPush(const std::string &key, const std::string &val, int &errcode) {
    return LeftPush(Key(key), val, errcode);
  }

  /**
   * @brief push item into list to the right 
   * 
   * @return true 
   * @return false 
   */
  bool RightPush(const Key &key, const std::string &val, int &errcode);

  size_t RightPush(const std::string &key, const std::vector<std::string> &values, int &errcode);

  bool RightPush(const std::string &key, const std::string &val, int &errcode) {
    return RightPush(Key(key), val, errcode);
  }

  /**
   * @brief pop item from left, valid for OBJECT_LIST type
   * 
   */
  DynamicString LeftPop(const Key &key, int &errcode);

  DynamicString LeftPop(const std::string &key, int &errcode) {
    return LeftPop(Key(key), errcode);
  }

  /**
   * @brief pop item from right, valid for OBJECT_LIST type
   * 
   */
  DynamicString RightPop(const Key &key, int &errcode);

  DynamicString RightPop(const std::string &key, int &errcode) {
    return RightPop(Key(key), errcode);
  }

  /**
   * @brief get all items from a list
   * 
   */
  std::vector<DynamicString> ListRange(const Key &key, int begin, int end, int &errcode);

  std::vector<DynamicString> ListRange(const std::string &key, int begin, int end, int &errcode) {
    return ListRange(Key(key), begin, end, errcode);
  }

  std::vector<std::string> ListRangeAsStdString(const Key &key, int begin, int end, int &errcode);

  std::vector<std::string> ListRangeAsStdString(const std::string &key, int begin, int end, int &errcode) {
    return ListRangeAsStdString(Key(key), begin, end, errcode);
  }

  /**
   * @brief get the number of items in a list
   * 
   * @return size_t 
   */
  size_t ListLen(const Key &key, int &errcode);

  size_t ListLen(const std::string &key, int &errcode) {
    return ListLen(Key(key), errcode);
  }

  /**
   * @brief retrieve item at index from a list (index starting from 0)
   * 
   */
  DynamicString ListItemAtIndex(const Key &key, int index, int &errcode);

  DynamicString ListItemAtIndex(const std::string &key, int index, int &errcode) {
    return ListItemAtIndex(Key(key), index, errcode);
  }

  bool ListSetItemAtIndex(const Key &key, int index, const std::string &val, int &errcode);

  bool ListSetItemAtIndex(const std::string &key, int index, const std::string &val, int &errcode) {
    return ListSetItemAtIndex(Key(key), index, val, errcode);
  }

  /******************** Hashtable operation ********************/

  bool HashSetKV(const Key &key, const DictKey &field, const DictVal &value, int &errcode);

  bool HashSetKV(const std::string &key, const std::string &field, const std::string &value, int &errcode) {
    return HashSetKV(Key(key), DictKey(field), DictVal(value), errcode);
  }

  int HashSetKV(const Key &key, const std::vector<std::string> &fields,
                const std::vector<std::string> &values, int &errcode);

  DictVal HashGetValue(const Key &key, const DictKey &field, int &errcode);

  DictVal HashGetValue(const std::string &key, const std::string &field, int &errcode) {
    return HashGetValue(Key(key), DictKey(field), errcode);
  }

  std::vector<DictVal> HashGetValue(const Key& key, const std::vector<std::string>& fields, int &errcode);

  int HashDelField(const Key &key, const DictKey &field, int &errcode);

  int HashDelField(const std::string &key, const std::string &field, int &errcode) {
    return HashDelField(Key(key), DictKey(field), errcode);
  }

  int HashDelField(const Key &key, const std::vector<std::string> &fields, int &errcode);

  bool HashExistField(const Key &key, const DictKey &field, int &errcode);

  bool HashExistField(const std::string &key, const std::string &field, int &errcode) {
    return HashExistField(Key(key), DictKey(field), errcode);
  }

  std::vector<DynamicString> HashGetAllEntries(const Key &key, int &errcode);

  std::vector<DynamicString> HashGetAllEntries(const std::string &key, int &errcode) {
    return HashGetAllEntries(Key(key), errcode);
  }

  std::vector<DictKey> HashGetAllFields(const Key &key, int &errcode);

  std::vector<DictKey> HashGetAllFields(const std::string &key, int &errcode) {
    return HashGetAllFields(Key(key), errcode);
  }

  std::vector<DictVal> HashGetAllValues(const Key &key, int &errcode);

  std::vector<DictVal> HashGetAllValues(const std::string &key, int &errcode) {
    return HashGetAllValues(Key(key), errcode);
  }

  size_t HashLen(const Key &key, int &errcode);

  size_t HashLen(const std::string &key, int &errcode) {
    return HashLen(Key(key), errcode);
  }

private:
  inline Bucket &GetBucket(const Key &key) {
    size_t bucket_idx = key.Hash() % kBucketSize;
    return bucket_[bucket_idx];
  }

  bool ListPushAux(const Key &key, const std::string &val, bool leftpush, int &errcode);

  size_t ListPushAux(const Key &key, const std::vector<std::string> &values, bool leftpush, int &errcode);

  DynamicString ListPopAux(const Key &key, bool leftpop, int &errcode);

  std::vector<std::string> KeyEvictionRandom(size_t num);

  std::vector<std::string> KeyEvictionLru(size_t num);

  void KeyEvictionLruHelper(std::vector<std::string>& deleted_keys);

private:
  mutable std::mutex mtx_;
  std::array<Bucket, kBucketSize> bucket_;
  std::vector<Key> keys_pool_;
};

#endif  // __CORE_H__
