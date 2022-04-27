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

int KVContainer::QueryObjectType(const std::string &key) {
  return QueryObjectType(Key(key));
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
  return true;
}

DynamicString KVContainer::LeftPop(const Key& key, int& errcode) {
  return std::move(ListPopAux(key, true, errcode));
}

DynamicString KVContainer::RightPop(const Key& key, int& errcode) {
  return std::move(ListPopAux(key, false, errcode));
}

DynamicString KVContainer::ListPopAux(const Key &key, bool leftpop, int &errcode) {
  Bucket &bucket = GetBucket(key);
  std::mutex &mtx = bucket.mtx;
  LockGuard lck(mtx);
  if (bucket.content.find(key) == bucket.content.end()) {
    errcode = kKeyNotFoundCode;
    return std::move(DynamicString());
  }
  /* key exists */
  if (bucket.content[key]->type != OBJECT_LIST) {
    errcode = kWrongTypeCode;
    return std::move(DynamicString());
  }
  if (leftpop) {
    return std::move(((DList *)((bucket.content[key])->ptr))->PopLeft());
  }
  return std::move(((DList *)((bucket.content[key])->ptr))->PopRight());
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
  return ((DList *)((bucket.content[key])->ptr))->Length();
}
