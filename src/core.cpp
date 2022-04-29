#include "core.h"
#include "valueobject.h"

inline static void UpdateExpAndVisitTime(ValueObjectPtr &ptr, uint64_t exp_time) {
  ptr->exp_time = exp_time;
  ptr->last_visit_time = GetCurrentSec();
}

int KVContainer::QueryObjectType(const Key &key) {
  Bucket &bucket = GetBucket(key);
  std::mutex &mtx = bucket.mtx;
  LockGuard lck(mtx);
  if (bucket.content.find(key) == bucket.content.end()) {
    return -1; // not found
  }
  return bucket.content[key]->type;
}

bool KVContainer::KeyExists(const Key &key) {
  Bucket &bucket = GetBucket(key);
  std::mutex &mtx = bucket.mtx;
  // only lock corresponding bucket
  LockGuard lck(mtx);
  return bucket.content.find(key) != bucket.content.end();
}

size_t KVContainer::NumItems() const {
  size_t cnt = 0;
  for (const Bucket &bucket : bucket_) {
    LockGuard bklck(bucket.mtx);
    cnt += bucket.content.size();
  }
  return cnt;
}

bool KVContainer::SetInt(const Key &key, int64_t intval, uint64_t exp_time) {
  // get bucket
  Bucket &bucket = GetBucket(key);
  std::mutex &mtx = bucket.mtx;
  // only lock corresponding bucket
  LockGuard lck(mtx);
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
  Bucket &bucket = GetBucket(key);
  std::mutex &mtx = bucket.mtx;
  LockGuard lck(mtx);
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

ValueObjectPtr KVContainer::Get(const Key &key, int &errcode) {
  Bucket &bucket = GetBucket(key);
  std::mutex &mtx = bucket.mtx;
  LockGuard lck(mtx);
  if (bucket.content.find(key) == bucket.content.end()) {
    errcode = kKeyNotFoundCode;
    return ValueObjectPtr(); // not found
  }
  auto objptr = bucket.content[key];
  if (objptr->type != OBJECT_INT &&
      objptr->type != OBJECT_STRING) { // invalid type for get operation
    errcode = kWrongTypeCode;
    return ValueObjectPtr(); // nullptr
  }
  errcode = kOkCode;
  return objptr;
}

ValueObjectPtr KVContainer::Get(const std::string &key, int &errcode) {
  return Get(Key(key), errcode);
}

bool KVContainer::Delete(const Key &key) {
  Bucket &bucket = GetBucket(key);
  std::mutex &mtx = bucket.mtx;
  LockGuard lck(mtx);
  if (bucket.content.find(key) != bucket.content.end()) {
    /* remove value from corresponding bucket */
    return bucket.content.erase(key) == 1;
  }
  return false;
}

bool KVContainer::Delete(const std::string &key) { return Delete(Key(key)); }

bool KVContainer::LeftPush(const Key &key, const std::string &val, int &errcode) {
  return ListPushAux(key, val, true, errcode);
}

bool KVContainer::RightPush(const Key &key, const std::string &val, int &errcode) {
  return ListPushAux(key, val, false, errcode);
}

bool KVContainer::ListPushAux(const Key &key, const std::string &val, bool leftpush, int &errcode) {
  Bucket &bucket = GetBucket(key);
  std::mutex &mtx = bucket.mtx;
  LockGuard lck(mtx);
  if (bucket.content.find(key) == bucket.content.end()) {
    /* create new dlist object */
    ValueObjectPtr obj = ConstructDListObjPtr();
    bucket.content[key] = obj;
  } else {
    /* check key validity */
    if (bucket.content[key]->type != OBJECT_LIST) {
      errcode = kWrongTypeCode;
      return false;
    }
  }
  if (leftpush) {
    ((DList *)(bucket.content[key]->ptr))->PushLeft(val);
  } else {
    ((DList *)(bucket.content[key]->ptr))->PushRight(val);
  }
  errcode = kOkCode;
  return true;
}

DynamicString KVContainer::LeftPop(const Key& key, int& errcode) {
  return ListPopAux(key, true, errcode);
}

DynamicString KVContainer::RightPop(const Key& key, int& errcode) {
  return ListPopAux(key, false, errcode);
}

DynamicString KVContainer::ListPopAux(const Key &key, bool leftpop, int &errcode) {
  Bucket &bucket = GetBucket(key);
  std::mutex &mtx = bucket.mtx;
  LockGuard lck(mtx);
  if (bucket.content.find(key) == bucket.content.end()) {
    errcode = kKeyNotFoundCode;
    return DynamicString();
  }
  /* key exists */
  if (bucket.content[key]->type != OBJECT_LIST) {
    errcode = kWrongTypeCode;
    return DynamicString();
  }
  if (leftpop) {
    return ((DList *)((bucket.content[key])->ptr))->PopLeft();
  }
  errcode = kOkCode;
  return ((DList *)((bucket.content[key])->ptr))->PopRight();
}

std::vector<DynamicString> KVContainer::ListRange(const Key& key, int begin, int end, int &errcode) {
  Bucket &bucket = GetBucket(key);
  std::mutex &mtx = bucket.mtx;
  LockGuard lck(mtx);
  if (bucket.content.find(key) == bucket.content.end()) {
    errcode = kKeyNotFoundCode;
    return {};
  }
  if (bucket.content[key]->type != OBJECT_LIST) {
    errcode = kWrongTypeCode;
    return {};
  }
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
  Bucket &bucket = GetBucket(key);
  std::mutex &mtx = bucket.mtx;
  LockGuard lck(mtx);
  if (bucket.content.find(key) == bucket.content.end()) {
    errcode = kKeyNotFoundCode;
    return 0;
  }
  if (bucket.content[key]->type != OBJECT_LIST) {
    errcode = kWrongTypeCode;
    return 0;
  }
  errcode = kOkCode;
  return ((DList *)((bucket.content[key])->ptr))->Length();
}

DynamicString KVContainer::ListItemAtIndex(const Key& key, int index, int& errcode) {
  Bucket &bucket = GetBucket(key);
  std::mutex &mtx = bucket.mtx;
  LockGuard lck(mtx);
  if (bucket.content.find(key) == bucket.content.end()) {
    errcode = kKeyNotFoundCode;
    return DynamicString();
  }
  if (bucket.content[key]->type != OBJECT_LIST) {
    errcode = kWrongTypeCode;
    return DynamicString();
  }
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
  Bucket &bucket = GetBucket(key);
  std::mutex &mtx = bucket.mtx;
  LockGuard lck(mtx);
  if (bucket.content.find(key) == bucket.content.end()) {
    errcode = kKeyNotFoundCode;
    return false;
  }
  if (bucket.content[key]->type != OBJECT_LIST) {
    errcode = kWrongTypeCode;
    return false;
  }
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
  } catch (const std::bad_alloc &ex) {
    errcode = kFailCode;
  }
  return false;
}
