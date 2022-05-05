#include <sstream>
#include "core.h"

/* Get bucket according to key and lock the bucket */
#define GetBucketAndLock(key) \
  Bucket &bucket = GetBucket(key); \
  std::mutex &mtx = bucket.mtx; \
  LockGuard lck(mtx) \

#define KeyNotFoundInBucket(key) \
  bucket.content.find(key) == bucket.content.end()

#define KeyFoundInBucket(key) \
  bucket.content.find(key) != bucket.content.end()

/* if key is not in bucket, then return retval */
#define IfKeyNotFoundThenReturn(key, retval) \
  if (KeyNotFoundInBucket(key)) { \
    errcode = kKeyNotFoundCode; \
    return retval;  \
  }

/* if key is not type of target_type, then return retval */
#define IfKeyNotTypeThenReturn(key, target_type, retval) \
  if (bucket.content[key]->type != target_type) { \
    errcode = kWrongTypeCode; \
    return retval; \
  }

#define IfKeyNeitherTypeThenReturn(key, option1, option2, retval) \
  if (bucket.content[key]->type != option1 && bucket.content[key]->type != option2) { \
    errcode = kWrongTypeCode; \
    return retval;  \
  }

#define RetrievePtr(key, Type) ((Type *) (bucket.content[key]->ptr))

std::string KVContainer::Overview() const {
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
  ss << "Number of int: " << n_int << std::endl
     << "Number of string: " << n_str << std::endl
     << "Number of list: " << n_list << ", total elements: " << n_list_elem << std::endl
     << "Number of hash: " << n_dict << ", total entries: " << n_dict_entry << std::endl;
  return ss.str();
}

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

std::vector<std::string> KVContainer::RecoverCommand(const std::string &key, int &errcode) {
  /* given an existing key, recover a command to set the key and value */
  Key k(key);
  GetBucketAndLock(k);
  IfKeyNotFoundThenReturn(k, {})
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
      const HTEntry& entry = *p_entry;
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
    bucket.content[key] = iptr;
  } else {
    // check existing key is type int
    if (bucket.content[key]->type != OBJECT_INT) {
      // delete old value and replace it with new intval
      bucket.content[key]->FreePtr();
    }
    // replace old value for int, and reset exp_time if needed
    bucket.content[key]->type = OBJECT_INT;
    bucket.content[key]->ptr = reinterpret_cast<void *>(intval);
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
    errcode = kOkCode;
    return iptr->ToInt64();
  } else {
    /* add increment on existing value */
    IfKeyNotTypeThenReturn(key, OBJECT_INT, 0)
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
    errcode = kOkCode;
    return iptr->ToInt64();
  } else {
    /* add decrement on existing value */
    IfKeyNotTypeThenReturn(key, OBJECT_INT, 0)
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
  } else {
    // check existing key is type string
    auto type = bucket.content[key]->type;
    if (type == OBJECT_STRING) {
      /* override existing string object */
      RetrievePtr(key, DynamicString)->Reset(value);
    } else if (type == OBJECT_LIST) {
      bucket.content[key]->FreePtr();
    } else if (type == OBJECT_INT) {
      /* construct a new dynamic string object */
      DynamicString *dsptr = new(std::nothrow) DynamicString(value);
      if (dsptr == nullptr) {
        return false;
      }
      bucket.content[key]->ptr = (void *) dsptr;
    }
    bucket.content[key]->type = OBJECT_STRING;
  }
  return true;
}


size_t KVContainer::StrLen(const Key &key, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, 0)
  if (bucket.content[key]->type == OBJECT_INT) {
    errcode = kOkCode;
    int64_t num = reinterpret_cast<int64_t>(bucket.content[key]->ptr);
    return std::to_string(num).size();
  } else if (bucket.content[key]->type == OBJECT_STRING) {
    errcode = kOkCode;
    return RetrievePtr(key, DynamicString)->Length();
  }
  errcode == kWrongTypeCode;
  return 0;
}

ValueObjectPtr KVContainer::Get(const Key &key, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, nullptr)
  // invalid type for get operation
  IfKeyNeitherTypeThenReturn(key, OBJECT_INT, OBJECT_STRING, nullptr)
  errcode = kOkCode;
  return bucket.content[key];
}

ValueObjectPtr KVContainer::Get(const std::string &key, int &errcode) {
  return Get(Key(key), errcode);
}

bool KVContainer::Delete(const Key &key) {
  GetBucketAndLock(key);
  return bucket.content.erase(key) == 1;
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
  IfKeyNotFoundThenReturn(key, false)
//  IfKeyNeitherTypeThenReturn(key, OBJECT_INT, OBJECT_STRING, false)
  if (bucket.content[key]->type == OBJECT_STRING) {
    ((DynamicString *) bucket.content[key]->ptr)->Append(val);
  } else if (bucket.content[key]->type == OBJECT_INT) {
    /* append operation will make int turn to string */
    int64_t num = bucket.content[key]->ToInt64();
    DynamicString *dsptr = new(std::nothrow) DynamicString(std::to_string(num));
    dsptr->Append(val);
    /* change value object */
    bucket.content[key]->ptr = dsptr;
    bucket.content[key]->type = OBJECT_STRING;
  } else {
    errcode = kWrongTypeCode;
    return 0;
  }
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

#define ListPushAuxCommon(key) \
  GetBucketAndLock(key);  \
  if (KeyNotFoundInBucket(key)) { \
    /* create new dlist object */   \
    ValueObjectPtr obj = ConstructDListObjPtr();  \
    bucket.content[key] = obj;  \
    } else {  \
    /* check key validity */  \
    IfKeyNotTypeThenReturn(key, OBJECT_LIST, false) \
  }

#define ListPushAuxCommon2(key, val) \
  if (leftpush) { \
    RetrievePtr(key, DList)->PushLeft(val); \
  } else {  \
    RetrievePtr(key, DList)->PushRight(val);  \
  }

bool KVContainer::ListPushAux(const Key &key, const std::string &val, bool leftpush, int &errcode) {
  ListPushAuxCommon(key)
  ListPushAuxCommon2(key, val)
  errcode = kOkCode;
  return true;
}

size_t KVContainer::ListPushAux(const Key &key, const std::vector<std::string> &values, bool leftpush, int &errcode) {
  ListPushAuxCommon(key)
  for (const auto &val : values) {
    ListPushAuxCommon2(key, val)
  }
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
  IfKeyNotFoundThenReturn(key, DynamicString())
  /* key exists */
  IfKeyNotTypeThenReturn(key, OBJECT_LIST, DynamicString())
  errcode = kOkCode;
  if (leftpop) {
    return RetrievePtr(key, DList)->PopLeft();
  }
  return RetrievePtr(key, DList)->PopRight();
}

std::vector<DynamicString> KVContainer::ListRange(const Key &key, int begin, int end, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, {})
  IfKeyNotTypeThenReturn(key, OBJECT_LIST, {})
  /* supported negative index here */
  DList *list = (DList *) (bucket.content[key]->ptr);
  int list_len = (int) list->Length();
  if (begin < 0) {
    begin = begin + list_len;
  }
  if (end < 0) {
    end = end + list_len;
  }
  errcode = kOkCode;
  /* handle negative index out of range */
  if (begin < 0 && end > 0) {
    begin = 0;
  } else if ((begin > 0 && end < 0) || (begin < 0 && end < 0)) {
    return {};
  }
  return RetrievePtr(key, DList)->RangeAsDynaStringVector(begin, end);
}

size_t KVContainer::ListLen(const Key &key, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, 0)
  IfKeyNotTypeThenReturn(key, OBJECT_LIST, 0)
  errcode = kOkCode;
  return RetrievePtr(key, DList)->Length();
}

DynamicString KVContainer::ListItemAtIndex(const Key &key, int index, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, DynamicString())
  IfKeyNotTypeThenReturn(key, OBJECT_LIST, DynamicString())
  DList *list = (DList *) (bucket.content[key]->ptr);
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
    errcode = kOkCode;
    return DynamicString(list->operator[](index));
  } catch (const std::out_of_range &ex) {
    errcode = kFailCode;
  }
  return DynamicString();
}

bool KVContainer::ListSetItemAtIndex(const Key &key, int index, const std::string &val, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, false)
  IfKeyNotTypeThenReturn(key, OBJECT_LIST, false)
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
  } else { /* found key */
    IfKeyNotTypeThenReturn(key, OBJECT_HASH, false);
    /* found hash and then update */
    RetrievePtr(key, Dict)->Update(field, value);
  }
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
  } else { /* found existing key */
    IfKeyNotTypeThenReturn(key, OBJECT_HASH, 0)
  }
  Dict *p_dict = RetrievePtr(key, Dict);
  for (size_t i = 0; i < fields.size(); ++i) {
    if (p_dict->Update(fields[i], values[i]) != UNDEFINED) {
      ++count;
    }
  }
  errcode = kOkCode;
  return count;
}

DictVal KVContainer::HashGetValue(const Key &key, const DictKey &field, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, DictVal())
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, DictVal())
  errcode = kOkCode;
  try {
    return RetrievePtr(key, Dict)->At(field);
  } catch (const std::out_of_range &ex) {
    return DictVal();
  }
}

std::vector<DictVal> KVContainer::HashGetValue(const Key &key, const std::vector<std::string> &fields, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, {})
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, {})
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
  errcode = kOkCode;
  return values;
}

int KVContainer::HashDelField(const Key &key, const DictKey &field, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, false)
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, false)
  errcode = kOkCode;
  return RetrievePtr(key, Dict)->Erase(field);
}

int KVContainer::HashDelField(const Key &key, const std::vector<std::string> &fields, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, 0)
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, 0)
  int n_erased = 0;
  for (const auto &field : fields) {
    if (RetrievePtr(key, Dict)->Erase(field) == ERASED) {
      ++n_erased;
    }
  }
  errcode = kOkCode;
  return n_erased;
}

bool KVContainer::HashExistField(const Key &key, const DictKey &field, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, false)
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, false);
  errcode = kOkCode;
  return RetrievePtr(key, Dict)->CheckExists(field);
}

std::vector<DynamicString> KVContainer::HashGetAllEntries(const Key &key, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, {})
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, {});
  errcode = kOkCode;
  std::vector<HTEntry *> entries = RetrievePtr(key, Dict)->AllEntries();
  std::vector<DynamicString> entries_str;
  entries_str.reserve(entries.size() * 2);
  for (const auto &p_entry : entries) {
    entries_str.emplace_back(*p_entry->key);
    entries_str.emplace_back(*p_entry->value);
  }
  return entries_str;
}

std::vector<DictKey> KVContainer::HashGetAllFields(const Key &key, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, {})
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, {});
  errcode = kOkCode;
  return RetrievePtr(key, Dict)->AllKeys();
}

std::vector<DictVal> KVContainer::HashGetAllValues(const Key &key, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, {})
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, {});
  errcode = kOkCode;
  return RetrievePtr(key, Dict)->AllValues();
}

size_t KVContainer::HashLen(const Key &key, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, 0)
  IfKeyNotTypeThenReturn(key, OBJECT_HASH, 0);
  errcode = kOkCode;
  return RetrievePtr(key, Dict)->Count();
}
