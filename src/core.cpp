#include "core.h"
#include "valueobject.h"

/* Get bucket according to key and lock the bucket */
#define GetBucketAndLock(key) \
  Bucket &bucket = GetBucket(key); \
  std::mutex &mtx = bucket.mtx; \
  LockGuard lck(mtx) \

/* if key is not in bucket, then return retval */
#define IfKeyNotFoundThenReturn(key, retval) \
  if (bucket.content.find(key) == bucket.content.end()) { \
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

inline static void UpdateExpAndVisitTime(ValueObjectPtr &ptr, uint64_t exp_time) {
  ptr->exp_time = exp_time;
  ptr->last_visit_time = GetCurrentSec();
}

int KVContainer::QueryObjectType(const Key &key) {
  GetBucketAndLock(key);
  if (bucket.content.find(key) == bucket.content.end()) {
    return -1; // not found
  }
  return bucket.content[key]->type;
}

bool KVContainer::KeyExists(const Key &key) {
  GetBucketAndLock(key);
  return bucket.content.find(key) != bucket.content.end();
}

int KVContainer::KeyExists(const std::vector<std::string>& keys) {
  int ans = 0;
  for (auto& key : keys) {
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

bool KVContainer::SetInt(const Key &key, int64_t intval, uint64_t exp_time) {
  // get bucket
  GetBucketAndLock(key);
  std::cout << "setting key = " << key << std::endl;
  if (bucket.content.find(key) == bucket.content.end()) {
    // set new value
    auto sptr = ConstructIntObjPtr(intval, exp_time);
    if (!sptr) {
      return false;
    }
    bucket.content[key] = sptr;
  } else {
    // check existing key is type int
    if (bucket.content[key]->type != OBJECT_INT) {
      // delete old value and replace it with new intval
      bucket.content[key]->FreePtr();
    }
    // replace old value for int, and reset exp_time if needed
    bucket.content[key]->type = OBJECT_INT;
    bucket.content[key]->ptr = reinterpret_cast<void *>(intval);
    UpdateExpAndVisitTime(bucket.content[key], exp_time);
  }
  return true;
}

bool KVContainer::SetInt(const std::string &key, int64_t intval, uint64_t exp_time) {
  return SetInt(Key(key), intval, exp_time);
}

bool KVContainer::SetString(const Key &key, const std::string &value, uint64_t exp_time) {
  // get bucket
  GetBucketAndLock(key);
  if (bucket.content.find(key) == bucket.content.end()) {
    // set new value
    ValueObjectPtr sptr = ConstructStrObjPtr(value, exp_time);
    if (!sptr) {
      return false;
    }
    bucket.content[key] = sptr;
  } else {
    // check existing key is type string
    auto type = bucket.content[key]->type;
    if (type == OBJECT_STRING) {
      /* override existing string object */
      ((DynamicString *)bucket.content[key]->ptr)->Reset(value);
    } else if (type == OBJECT_LIST) {
      bucket.content[key]->FreePtr();
    } else if (type == OBJECT_INT) {
      /* construct a new dynamic string object */
      DynamicString *dsptr = new (std::nothrow) DynamicString(value);
      if (dsptr == nullptr) {
        return false;
      }
      bucket.content[key]->ptr = (void *)dsptr;
    }
    bucket.content[key]->type = OBJECT_STRING;
    UpdateExpAndVisitTime(bucket.content[key], exp_time);
  }
  return true;
}

bool KVContainer::SetString(const std::string &key, const std::string &value, uint64_t exp_time) {
  return SetString(Key(key), value, exp_time);
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
    return ((DynamicString*)(bucket.content[key]->ptr))->Length();
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
  for (const auto& key : keys) {
    if (Delete(key)) {
      ++n;
    }
  }
  return n;
}

size_t KVContainer::Append(const Key& key, const std::string& val, int& errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, false)
//  IfKeyNeitherTypeThenReturn(key, OBJECT_INT, OBJECT_STRING, false)
  if (bucket.content[key]->type == OBJECT_STRING) {
    ((DynamicString*)bucket.content[key]->ptr)->Append(val);
  } else if (bucket.content[key]->type == OBJECT_INT) {
    /* append operation will make int turn to string */
    int64_t num = bucket.content[key]->ToInt64();
    DynamicString *dsptr = new (std::nothrow) DynamicString(std::to_string(num));
    dsptr->Append(val);
    /* change value object */
    bucket.content[key]->ptr = dsptr;
    bucket.content[key]->type = OBJECT_STRING;
  } else {
    errcode = kWrongTypeCode;
    return 0;
  }
  errcode = kOkCode;
  return ((DynamicString*)(bucket.content[key]->ptr))->Length();
}

bool KVContainer::LeftPush(const Key &key, const std::string &val, int &errcode) {
  return ListPushAux(key, val, true, errcode);
}

size_t KVContainer::LeftPush(const std::string & key, const std::vector<std::string>& values, int& errcode) {
  return ListPushAux(Key(key), values, true, errcode);
}

bool KVContainer::RightPush(const Key &key, const std::string &val, int &errcode) {
  return ListPushAux(key, val, false, errcode);
}

size_t KVContainer::RightPush(const std::string & key, const std::vector<std::string>& values, int& errcode) {
  return ListPushAux(Key(key), values, false, errcode);
}

#define ListPushAuxCommon(key) \
  GetBucketAndLock(key);  \
  if (bucket.content.find(key) == bucket.content.end()) { \
    /* create new dlist object */   \
    ValueObjectPtr obj = ConstructDListObjPtr();  \
    bucket.content[key] = obj;  \
    } else {  \
    /* check key validity */  \
    IfKeyNotTypeThenReturn(key, OBJECT_LIST, false) \
  }

#define ListPushAuxCommon2(key, val) \
  if (leftpush) { \
    ((DList *)(bucket.content[key]->ptr))->PushLeft(val); \
  } else {  \
    ((DList *)(bucket.content[key]->ptr))->PushRight(val);  \
  }

bool KVContainer::ListPushAux(const Key &key, const std::string &val, bool leftpush, int &errcode) {
  ListPushAuxCommon(key)
  ListPushAuxCommon2(key, val)
  errcode = kOkCode;
  return true;
}

size_t KVContainer::ListPushAux(const Key& key, const std::vector<std::string>& values, bool leftpush, int& errcode) {
  ListPushAuxCommon(key)
  for (const auto& val : values) {
    ListPushAuxCommon2(key, val)
  }
  errcode = kOkCode;
  return ((DList *)(bucket.content[key]->ptr))->Length();
}

#undef ListPushAuxCommon2
#undef ListPushAuxCommon

DynamicString KVContainer::LeftPop(const Key& key, int& errcode) {
  return ListPopAux(key, true, errcode);
}

DynamicString KVContainer::RightPop(const Key& key, int& errcode) {
  return ListPopAux(key, false, errcode);
}

DynamicString KVContainer::ListPopAux(const Key &key, bool leftpop, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, DynamicString())
  /* key exists */
  IfKeyNotTypeThenReturn(key, OBJECT_LIST, DynamicString())
  errcode = kOkCode;
  if (leftpop) {
    return ((DList *)((bucket.content[key])->ptr))->PopLeft();
  }
  return ((DList *)((bucket.content[key])->ptr))->PopRight();
}

std::vector<DynamicString> KVContainer::ListRange(const Key& key, int begin, int end, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, {})
  IfKeyNotTypeThenReturn(key, OBJECT_LIST, {})
  /* supported negative index here */
  DList *list = (DList *)(bucket.content[key]->ptr);
  int list_len = (int)list->Length();
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
  return ((DList *)(bucket.content[key]->ptr))->RangeAsDynaStringVector(begin, end);
}

size_t KVContainer::ListLen(const Key& key, int& errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, 0)
  IfKeyNotTypeThenReturn(key, OBJECT_LIST, 0)
  errcode = kOkCode;
  return ((DList *)((bucket.content[key])->ptr))->Length();
}

DynamicString KVContainer::ListItemAtIndex(const Key& key, int index, int& errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, DynamicString())
  IfKeyNotTypeThenReturn(key, OBJECT_LIST, DynamicString())
  DList *list = (DList *)(bucket.content[key]->ptr);
  /* support negative index */
  int list_len = (int)list->Length();
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
  } catch (const std::out_of_range& ex) {
    errcode = kFailCode;
  }
  return DynamicString();
}

bool KVContainer::ListSetItemAtIndex(const Key &key, int index, const std::string &val, int &errcode) {
  GetBucketAndLock(key);
  IfKeyNotFoundThenReturn(key, false)
  IfKeyNotTypeThenReturn(key, OBJECT_LIST, false)
  DList *list = (DList *)(bucket.content[key]->ptr);
  /* support negative index */
  int list_len = (int)list->Length();
  if (index < 0) {
    index = index + list_len;
  }
  if (index < 0) {
    errcode = kFailCode;
    return false;
  }
  try {
    errcode = kOkCode;
    list->operator[](index).Reset(val);
    return true;
  } catch (const std::out_of_range& ex) {
    errcode = kOutOfRangeCode;
  } catch (const std::bad_alloc &ex) {
    errcode = kFailCode;
  }
  return false;
}
