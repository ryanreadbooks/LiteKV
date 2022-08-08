#include <sstream>
#include <random>
#include <algorithm>
#include <gperftools/malloc_extension.h>

#include "core.h"

/* maximum time in lru eviction loop, unit ms */
#define LRU_EVICTION_TIME_LIMIT_MS 25

static std::mt19937_64 sRandEngine(std::time(nullptr));

static int RandIntValue(int from, int to) {
 return std::uniform_int_distribution<int>(from, to)(sRandEngine);
}

static double RandDoubleValue(double from, double to) {
 return std::uniform_real_distribution<double>(from, to)(sRandEngine);
}

/* Get bucket according to key and lock the bucket */
#define GetBucketAndLock(key) \
  Bucket &bucket = GetBucket(key); \
  /*std::mutex &mtx = bucket.mtx; \
  LockGuard lck(mtx) \
  */

#define KeyNotFoundInBucket(key) \
  bucket.content.find(key) == bucket.content.end()

#define KeyFoundInBucket(key) \
  bucket.content.find(key) != bucket.content.end()

/* if key is not in bucket, then return retval */
#define IfKeyNotFoundThenReturn(key, retval)                                   \
  do {                                                                         \
    if (KeyNotFoundInBucket(key)) {                                            \
      errcode = kKeyNotFoundCode;                                              \
      return retval;                                                           \
    }                                                                          \
  } while (0)

/* if key is not type of target_type, then return retval */
#define IfKeyNotTypeThenReturn(key, target_type, retval)                       \
  do {                                                                         \
    if (bucket.content[key]->type != target_type) {                            \
      errcode = kWrongTypeCode;                                                \
      return retval;                                                           \
    }                                                                          \
  } while (0)

#define IfKeyNeitherTypeThenReturn(key, option1, option2, retval)              \
  do {                                                                         \
    if (bucket.content[key]->type != option1 &&                                \
        bucket.content[key]->type != option2) {                                \
      errcode = kWrongTypeCode;                                                \
      return retval;                                                           \
    }                                                                          \
  } while (0)

#define RetrievePtr(key, Type) ((Type *) (bucket.content[key]->ptr))

#define UpdateLastVisitTime(key) \
  bucket.content[key]->lv_time = GetCurrentMs()

std::vector<DynamicString> KVContainer::Overview() const {
  /* make statistic */
  LockGuard lck(mtx_);
  size_t n_int = 0, n_str = 0, n_list = 0, n_dict = 0;
  size_t n_list_elem = 0, n_dict_entry = 0;
  for (const auto &bucket : bucket_) {
    for (const auto &item : bucket.content) {
      if (item.second->type == OBJECT_INT) {
        ++n_int;
      } else if (item.second->type == OBJECT_STRING) {
        ++n_str;
      } else if (item.second->type == OBJECT_LIST) {
        ++n_list;
        /* find out this list item count */
        n_list_elem += ((DList *) (item.second->ptr))->Length();
      } else if (item.second->type == OBJECT_HASH) {
        ++n_dict;
        /* find out this dict item count */
        n_dict_entry += ((Dict *) (item.second->ptr))->Count();
      }
    }
  }
  std::stringstream ss;
  ss << "Number of int: " << n_int
     << "\tNumber of string: " << n_str
     << "\tNumber of list: " << n_list << ", total elements: " << n_list_elem
     << "\tNumber of hash: " << n_dict << ", total entries: " << n_dict_entry;
  std::vector<DynamicString> overview;
  overview.emplace_back("Number of int:");
  overview.emplace_back(std::to_string(n_int));
  overview.emplace_back("Number of string:");
  overview.emplace_back(std::to_string(n_str));
  overview.emplace_back("Number of list:");
  overview.emplace_back(std::to_string(n_list));
  overview.emplace_back("Number of elements in list:");
  overview.emplace_back(std::to_string(n_list_elem));
  overview.emplace_back("Number of hash:");
  overview.emplace_back(std::to_string(n_dict));
  overview.emplace_back("Number of elements in hash:");
  overview.emplace_back(std::to_string(n_dict_entry));
  return overview;
}

std::vector<std::string> KVContainer::KeyEviction(int policy, size_t n) {
  if (keys_pool_.empty()) {
    return {};
  }
  if (policy == EVICTION_POLICY_LRU) {
#ifdef TCMALLOC_FOUND
    MallocExtension::instance()->ReleaseFreeMemory();
#endif
    return KeyEvictionLru(n);
  } else if (policy == EVICTION_POLICY_RANDOM) {
#ifdef TCMALLOC_FOUND
    MallocExtension::instance()->ReleaseFreeMemory();
#endif
    return KeyEvictionRandom(n);
  }
  return {};
}

#define FetchLvTimeFromKeyPointer(key)  \
  GetBucket(key).content[key]->lv_time

std::vector<std::string> KVContainer::KeyEvictionRandom(size_t num) {
  /* Simple eviction policy: random eviction
   * random select num keys and discard them
  */
  num = std::min(keys_pool_.size(), num);
  std::vector<std::string> deleted_keys;
  for (size_t i = 0; i < num; ++i) {
    size_t idx = RandIntValue(0, (int) keys_pool_.size() - 1);
    const Key& key = keys_pool_[idx];
    if (Delete(key)) {
      deleted_keys.emplace_back(key.ToStdString());
    }
  }
  return deleted_keys;
}

std::vector<std::string> KVContainer::KeyEvictionLru(size_t num) {
  /* LRU eviction policy: discard num keys according to approximate LRU */
  size_t n_deleted = 0;
  std::vector<std::string> deleted_keys;
  auto start = GetCurrentMs();
  while (!keys_pool_.empty() && n_deleted < num) {
    KeyEvictionLruHelper(deleted_keys);
    /* avoid massive time consumption thus set max time limit */
    if (GetCurrentMs() - start > LRU_EVICTION_TIME_LIMIT_MS) {
      break;
    }
  }
  return deleted_keys;
}

void KVContainer::KeyEvictionLruHelper(std::vector<std::string>& deleted_keys) {
  /* Random select 10 keys as one group, and evict the smallest lv_time in group,
  * repeat the process till n_deleted reaches num
  */
  if (keys_pool_.empty()) return;
  std::vector<Key> candidates;
  for (int i = 0; i < 10; ++i) {
    size_t idx = RandIntValue(0, (int) keys_pool_.size() - 1);
    if (std::find(candidates.begin(), candidates.end(), keys_pool_[idx]) == candidates.end()) {
      candidates.emplace_back(keys_pool_[idx]);
    }
  }
  auto min_it = std::min_element(candidates.begin(),
                                 candidates.end(),
                                 [this](const Key &a, const Key &b) -> bool {
                                   return FetchLvTimeFromKeyPointer(a) < FetchLvTimeFromKeyPointer(b);
                                 });
  if (min_it != candidates.end() && Delete(*min_it)) {
    deleted_keys.emplace_back(min_it->ToStdString());
  }
}

#undef FetchLvTimeFromKeyPointer

int KVContainer::QueryObjectType(const Key &key) {
  GetBucketAndLock(key);
  if (KeyNotFoundInBucket(key)) {
    return -1; // not found
  }
  return bucket.content[key]->type;
}

bool KVContainer::KeyExists(const Key &key) {
  GetBucketAndLock(key);
  return bucket.content.find(key) != bucket.content.end();
}

int KVContainer::KeyExists(const std::vector<std::string> &keys) {
  int ans = 0;
  for (auto &key : keys) {
    if (KeyExists(Key(key))) {
      ++ans;
    }
  }
  return ans;
}

size_t KVContainer::NumItems() const {
  size_t cnt = 0;
  for (const Bucket &bucket : bucket_) {
    LockGuard bk_lck(bucket.mtx);
    cnt += bucket.content.size();
  }
  return cnt;
}

std::vector<std::string> KVContainer::RecoverCommandFromValue(const std::string &key, int &errcode) {
  /* given an existing key, recover a command to set the key and value */
  Key k(key);
  GetBucketAndLock(k);
  IfKeyNotFoundThenReturn(k, {});
  auto k_type = bucket.content[k]->type;
  errcode = kOkCode;
  if (k_type == OBJECT_INT) {
    return {"set", key, std::to_string(bucket.content[k]->ToInt64())};
  } else if (k_type == OBJECT_STRING) {
    return {"set", key, bucket.content[k]->ToStdString()};
  } else if (k_type == OBJECT_LIST) {
    /* rpush */
    std::vector<std::string> ranges = RetrievePtr(k, DList)->RangeAsStdStringVector();
    std::vector<std::string> ret = {"rpush", key};
    ret.insert(ret.end(), ranges.begin(), ranges.end());
    return ret;
  } else if (k_type == OBJECT_HASH) {
    /* hset */
    std::vector<HTEntry *> entries = RetrievePtr(k, Dict)->AllEntries();
    std::vector<std::string> entries_str;
    entries_str.reserve(entries.size() * 2);
    for (const auto &p_entry : entries) {
      const HTEntry &entry = *p_entry;
      entries_str.emplace_back(entry.key->ToStdString());
      entries_str.emplace_back(entry.value->ToStdString());
    }
    std::vector<std::string> ret = {"hset", key};
    ret.insert(ret.end(), entries_str.begin(), entries_str.end());
    return ret;
  }
  errcode = kFailCode;
  return {};
}

bool KVContainer::SetInt(const Key &key, int64_t intval) {
  // get bucket
  GetBucketAndLock(key);
  if (KeyNotFoundInBucket(key)) {
    // set new value
    auto iptr = ConstructIntObjPtr(intval);
    if (!iptr) {
      return false;
    }
    /* put new value into bucket */
    bucket.content[key] = iptr;
    keys_pool_.emplace_back(bucket.content.find(key)->first);
  } else {
    // check existing key is type int
    if (bucket.content[key]->type != OBJECT_INT) {
      // delete old value and replace it with new intval
      bucket.content[key]->FreePtr();
    }
    // replace old value for int, and reset exp_time if needed
    bucket.content[key]->type = OBJECT_INT;
    bucket.content[key]->ptr = reinterpret_cast<void *>(intval);
    UpdateLastVisitTime(key);
  }
  return true;
}

int64_t KVContainer::IncrIntBy(const Key &key, int64_t increment, int &errcode) {
  // get bucket
  // we ensure here increment >= 0, 0 <= increment <= INT64_MAX
  GetBucketAndLock(key);
  /* if no key found, we create new int and set it to zero then perform increment operation */
  if (KeyNotFoundInBucket(key)) {
    auto iptr = ConstructIntObjPtr(increment);
    if (!iptr) {
      errcode = kFailCode;
      return 0;
    }
    bucket.content[key] = iptr;
    keys_pool_.emplace_back(bucket.content.find(key)->first);
    errcode = kOkCode;
    return iptr->ToInt64();
  } else {
    /* add increment on existing value */
    IfKeyNotTypeThenReturn(key, OBJECT_INT, 0);
    UpdateLastVisitTime(key);
    /* check overflow */
    int64_t val = bucket.content[key]->ToInt64();
    if (val > INT64_MAX - increment) {
      errcode = kOverflowCode;
      return 0;
    }
    int64_t ans = val + increment;
    bucket.content[key]->ptr = reinterpret_cast<void *>(ans);
    errcode = kOkCode;
    return ans;
  }
}

int64_t KVContainer::DecrIntBy(const Key &key, int64_t decrement, int &errcode) {
  // we ensure here decrement is positive. decrement >= 0, 0 <= decrement <= INT64_MAX
  GetBucketAndLock(key);
  /* if no key found, we create new int and set it to zero then perform decrement operation */
  if (KeyNotFoundInBucket(key)) {
    auto iptr = ConstructIntObjPtr(-decrement);
    if (!iptr) {
      errcode = kFailCode;
      return 0;
    }
    bucket.content[key] = iptr;
    keys_pool_.emplace_back(bucket.content.find(key)->first);
    errcode = kOkCode;
    return iptr->ToInt64();
  } else {
    /* add decrement on existing value */
    IfKeyNotTypeThenReturn(key, OBJECT_INT, 0);
    UpdateLastVisitTime(key);
    int64_t val = bucket.content[key]->ToInt64();
    if (val < INT64_MIN + decrement) {
      errcode = kOverflowCode;
      return 0;
    }
    int64_t ans = val - decrement;
    bucket.content[key]->ptr = reinterpret_cast<void *>(ans);
    errcode = kOkCode;
    return ans;
  }
}


bool KVContainer::SetString(const Key &key, const std::string &value) {
  // get bucket
  GetBucketAndLock(key);
  if (KeyNotFoundInBucket(key)) {
    // set new value
    ValueObjectPtr sptr = ConstructStrObjPtr(value);
    if (!sptr) {
      return false;
    }
    bucket.content[key] = sptr;
    keys_pool_.emplace_back(bucket.content.find(key)->first);
  } else {
    // check existing key is type string
    auto type = bucket.content[key]->type;
    if (type == OBJECT_STRING) {
      /* override existing string object */
      RetrievePtr(key, DynamicString)->Reset(value);
    } else if (type == OBJECT_LIST || type == OBJECT_HASH) {
      bucket.content[key]->FreePtr();
    }
    if (type == OBJECT_INT || type == OBJECT_LIST || type == OBJECT_HASH) {
      /* construct a new dynamic string object */
      DynamicString *dsptr = new(std::nothrow) DynamicString(value);
      if (dsptr == nullptr) {
        return false;
      }
      bucket.content[key]->ptr = (void *) dsptr;
    }
    UpdateLastVisitTime(key);
    bucket.content[key]->type = OBJECT_STRING;
  }
  return true;
}

size_t KVContainer::StrLen(const Key &key, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, 0);
  if (bucket.content[key]->type == OBJECT_INT) {
    UpdateLastVisitTime(key);
    errcode = kOkCode;
    int64_t num = reinterpret_cast<int64_t>(bucket.content[key]->ptr);
    return std::to_string(num).size();
  } else if (bucket.content[key]->type == OBJECT_STRING) {
    UpdateLastVisitTime(key);
    errcode = kOkCode;
    return RetrievePtr(key, DynamicString)->Length();
  }
  errcode == kWrongTypeCode;
  return 0;
}

ValueObjectPtr KVContainer::Get(const Key &key, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, nullptr);
  // invalid type for get operation
  IfKeyNeitherTypeThenReturn(key, OBJECT_INT, OBJECT_STRING, nullptr);
  UpdateLastVisitTime(key);
  errcode = kOkCode;
  return bucket.content[key];
}

ValueObjectPtr KVContainer::Get(const std::string &key, int &errcode) {
  return Get(Key(key), errcode);
}

bool KVContainer::Delete(const Key &key) {
  GetBucketAndLock(key);
  auto it = bucket.content.find(key);
  if (it == bucket.content.end()) {
    return false;
  }
  const Key &p = it->first;
  /* FIXME slow: O(n) time */
  keys_pool_.erase(std::remove(keys_pool_.begin(), keys_pool_.end(), p),
                   keys_pool_.end());
  bucket.content.erase(it);
  return true;
}

int KVContainer::Delete(const std::vector<std::string> &keys) {
  size_t n = 0;
  for (const auto &key : keys) {
    if (Delete(key)) {
      ++n;
    }
  }
  return n;
}

size_t KVContainer::Append(const Key &key, const std::string &val, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, false);
  if (bucket.content[key]->type == OBJECT_STRING) {
    ((DynamicString *) bucket.content[key]->ptr)->Append(val);
  } else if (bucket.content[key]->type == OBJECT_INT) {
    /* append operation will make int turn to string */
    int64_t num = bucket.content[key]->ToInt64();
    DynamicString *dsptr = new(std::nothrow) DynamicString(std::to_string(num));
    if (dsptr == nullptr) return 0;
    dsptr->Append(val);
    /* change value object */
    bucket.content[key]->ptr = dsptr;
    bucket.content[key]->type = OBJECT_STRING;
  } else {
    errcode = kWrongTypeCode;
    return 0;
  }
  UpdateLastVisitTime(key);
  errcode = kOkCode;
  return RetrievePtr(key, DynamicString)->Length();
}

bool KVContainer::LeftPush(const Key &key, const std::string &val, int &errcode) {
  return ListPushAux(key, val, true, errcode);
}

size_t KVContainer::LeftPush(const std::string &key, const std::vector<std::string> &values, int &errcode) {
  return ListPushAux(Key(key), values, true, errcode);
}

bool KVContainer::RightPush(const Key &key, const std::string &val, int &errcode) {
  return ListPushAux(key, val, false, errcode);
}

size_t KVContainer::RightPush(const std::string &key, const std::vector<std::string> &values, int &errcode) {
  return ListPushAux(Key(key), values, false, errcode);
}

#define ListPushAuxCommon(key)                                \
  GetBucketAndLock(key);                                      \
  if (KeyNotFoundInBucket(key)) {                             \
    /* create new dlist object */                             \
    ValueObjectPtr obj = ConstructDListObjPtr();              \
    bucket.content[key] = obj;                                \
    keys_pool_.emplace_back(bucket.content.find(key)->first); \
  } else {                                                    \
    /* check key validity */                                  \
    IfKeyNotTypeThenReturn(key, OBJECT_LIST, false);          \
  }

#define ListPushAuxCommon2(key, val) \
  if (leftpush) { \
    RetrievePtr(key, DList)->PushLeft(val); \
  } else {  \
    RetrievePtr(key, DList)->PushRight(val);  \
  }

bool KVContainer::ListPushAux(const Key &key, const std::string &val, bool leftpush, int &errcode) {
  ListPushAuxCommon(key);
  ListPushAuxCommon2(key, val);
  UpdateLastVisitTime(key);
  errcode = kOkCode;
  return true;
}

size_t KVContainer::ListPushAux(const Key &key, const std::vector<std::string> &values, bool leftpush, int &errcode) {
  ListPushAuxCommon(key);
  for (const auto &val : values) {
    ListPushAuxCommon2(key, val)
  }
  UpdateLastVisitTime(key);
  errcode = kOkCode;
  return ((DList *) (bucket.content[key]->ptr))->Length();
}

#undef ListPushAuxCommon2
#undef ListPushAuxCommon

DynamicString KVContainer::LeftPop(const Key &key, int &errcode) {
  return ListPopAux(key, true, errcode);
}

DynamicString KVContainer::RightPop(const Key &key, int &errcode) {
  return ListPopAux(key, false, errcode);
}

DynamicString KVContainer::ListPopAux(const Key &key, bool leftpop, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, DynamicString());
  /* key exists */
  IfKeyNotTypeThenReturn(key, OBJECT_LIST, DynamicString());
  UpdateLastVisitTime(key);
  errcode = kOkCode;
  if (leftpop) {
    return RetrievePtr(key, DList)->PopLeft();
  }
  return RetrievePtr(key, DList)->PopRight();
}

#define ListRangeCommonOperation                                               \
  GetBucketAndLock(key);                                                       \
  IfKeyNotFoundThenReturn(key, {});                                            \
  IfKeyNotTypeThenReturn(key, OBJECT_LIST, {});                                \
  DList *list = (DList *)(bucket.content[key]->ptr);                           \
  int list_len = (int)list->Length();                                          \
  /* supported negative index here */                                          \
  if (begin < 0) {                                                             \
    begin = begin + list_len;                                                  \
  }                                                                            \
  if (end < 0) {                                                               \
    end = end + list_len;                                                      \
  }                                                                            \
  errcode = kOkCode;                                                           \
  /* handle negative index out of range */                                     \
  if (begin < 0 && end > 0) {                                                  \
    begin = 0;                                                                 \
  } else if ((begin > 0 && end < 0) || (begin < 0 && end < 0)) {               \
    return {};                                                                 \
  }                                                                            \
  UpdateLastVisitTime(key);

std::vector<DynamicString> KVContainer::ListRange(const Key &key, int begin, int end, int &errcode) {
  ListRangeCommonOperation;
  return RetrievePtr(key, DList)->RangeAsDynaStringVector(begin, end);
}

std::vector<std::string> KVContainer::ListRangeAsStdString(const Key &key, int begin, int end, int &errcode) {
  ListRangeCommonOperation;
  return RetrievePtr(key, DList)->RangeAsStdStringVector(begin, end);
}

#undef ListRangeCommonOperation

size_t KVContainer::ListLen(const Key &key, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, 0);
  IfKeyNotTypeThenReturn(key, OBJECT_LIST, 0);
  UpdateLastVisitTime(key);
  errcode = kOkCode;
  return RetrievePtr(key, DList)->Length();
}

DynamicString KVContainer::ListItemAtIndex(const Key &key, int index, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, DynamicString());
  IfKeyNotTypeThenReturn(key, OBJECT_LIST, DynamicString());
  DList *list = (DList *)(bucket.content[key]->ptr);
  /* support negative index */
  int list_len = (int) list->Length();
  if (index < 0) {
    index = index + list_len;
  }
  if (index < 0) {
    errcode = kFailCode;
    return DynamicString();
  }
  try {
    UpdateLastVisitTime(key);
    errcode = kOkCode;
    return DynamicString(list->operator[](index));
  } catch (const std::out_of_range &ex) {
    errcode = kFailCode;
  }
  return DynamicString();
}

bool KVContainer::ListSetItemAtIndex(const Key &key, int index, const std::string &val, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, false);
  IfKeyNotTypeThenReturn(key, OBJECT_LIST, false);
  DList *list = RetrievePtr(key, DList);
  /* support negative index */
  int list_len = (int) list->Length();
  if (index < 0) {
    index = index + list_len;
  }
  if (index < 0) {
    errcode = kFailCode;
    return false;
  }
  try {
    list->operator[](index).Reset(val);
    UpdateLastVisitTime(key);
    errcode = kOkCode;
    return true;
  } catch (const std::out_of_range &ex) {
    errcode = kOutOfRangeCode;
  } catch (const std::bad_alloc &ex) {
    errcode = kFailCode;
  }
  return false;
}

bool KVContainer::HashSetKV(const Key &key, const DictKey &field, const DictVal &value, int &errcode) {
  GetBucketAndLock(key);
  if (KeyNotFoundInBucket(key)) {
    /* create new hash object */
    auto obj = ConstructHashObjPtr();
    if (!obj) {
      errcode = kFailCode;
      return false;
    }
    ((Dict *) (obj->ptr))->Update(field, value);
    bucket.content[key] = obj;
    keys_pool_.emplace_back(bucket.content.find(key)->first);
  } else { /* found key */
    IfKeyNotTypeThenReturn(key, OBJECT_HASH, false);
    /* found hash and then update */
    RetrievePtr(key, Dict)->Update(field, value);
  }
  UpdateLastVisitTime(key);
  errcode = kOkCode;
  return true;
}

int KVContainer::HashSetKV(const Key &key, const std::vector<std::string> &fields,
                           const std::vector<std::string> &values, int &errcode) {
  if (fields.size() != values.size()) {
    errcode = kFailCode;
    return 0;
  }
  GetBucketAndLock(key);
  /* if not exists key, then create new hashtable */
  int count = 0;
  if (KeyNotFoundInBucket(key)) {
    auto obj = ConstructHashObjPtr();
    if (!obj) {
      errcode = kFailCode;
      return 0;
    }
    /* put field-value into key */
    bucket.content[key] = obj;
    keys_pool_.emplace_back(bucket.content.find(key)->first);
  } else { /* found existing key */
    IfKeyNotTypeThenReturn(key, OBJECT_HASH, 0);
  }
  Dict *p_dict = RetrievePtr(key, Dict);
  for (size_t i = 0; i < fields.size(); ++i) {
    if (p_dict->Update(fields[i], values[i]) != UNDEFINED) {
      ++count;
    }
  }
  UpdateLastVisitTime(key);
  errcode = kOkCode;
  return count;
}

DictVal KVContainer::HashGetValue(const Key &key, const DictKey &field, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, DictVal());
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, DictVal());
  UpdateLastVisitTime(key);
  errcode = kOkCode;
  try {
    return RetrievePtr(key, Dict)->At(field);
  } catch (const std::out_of_range &ex) {
    errcode = kKeyNotFoundCode;
    return DictVal();
  }
}

std::vector<DictVal> KVContainer::HashGetValue(const Key &key, const std::vector<std::string> &fields, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, {});
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, {});
  std::vector<DictVal> values;
  Dict *p_dict = RetrievePtr(key, Dict);
  for (const auto &field : fields) {
    try {
      values.emplace_back(p_dict->At(field));
    } catch (const std::out_of_range &) {
      /* this field can not be found in hashtable */
      values.emplace_back(DictVal());
    }
  }
  UpdateLastVisitTime(key);
  errcode = kOkCode;
  return values;
}

int KVContainer::HashDelField(const Key &key, const DictKey &field, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, false);
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, false);
  UpdateLastVisitTime(key);
  errcode = kOkCode;
  return RetrievePtr(key, Dict)->Erase(field);
}

int KVContainer::HashDelField(const Key &key, const std::vector<std::string> &fields, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, 0);
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, 0);
  int n_erased = 0;
  for (const auto &field : fields) {
    if (RetrievePtr(key, Dict)->Erase(field) == ERASED) {
      ++n_erased;
    }
  }
  UpdateLastVisitTime(key);
  errcode = kOkCode;
  return n_erased;
}

bool KVContainer::HashExistField(const Key &key, const DictKey &field, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, false);
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, false);
  UpdateLastVisitTime(key);
  errcode = kOkCode;
  return RetrievePtr(key, Dict)->CheckExists(field);
}

std::vector<DynamicString> KVContainer::HashGetAllEntries(const Key &key, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, {});
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, {});
  std::vector<HTEntry *> entries = RetrievePtr(key, Dict)->AllEntries();
  std::vector<DynamicString> entries_str;
  entries_str.reserve(entries.size() * 2);
  for (const auto &p_entry : entries) {
    entries_str.emplace_back(*p_entry->key);
    entries_str.emplace_back(*p_entry->value);
  }
  UpdateLastVisitTime(key);
  errcode = kOkCode;
  return entries_str;
}

std::vector<DictKey> KVContainer::HashGetAllFields(const Key &key, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, {});
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, {});
  UpdateLastVisitTime(key);
  errcode = kOkCode;
  return RetrievePtr(key, Dict)->AllKeys();
}

std::vector<DictVal> KVContainer::HashGetAllValues(const Key &key, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, {});
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, {});
  UpdateLastVisitTime(key);
  errcode = kOkCode;
  return RetrievePtr(key, Dict)->AllValues();
}

size_t KVContainer::HashLen(const Key &key, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, 0);
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, 0);
  UpdateLastVisitTime(key);
  errcode = kOkCode;
  return RetrievePtr(key, Dict)->Count();
}
