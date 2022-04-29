#include <sys/time.h>

#include "valueobject.h"

ValueObject* ConstructIntObj(int64_t intval, uint64_t exp_time) {
  ValueObject *obj = new(std::nothrow) ValueObject;
  if (obj == nullptr)
    return nullptr;
  obj->type = OBJECT_INT;
  obj->last_visit_time = GetCurrentSec();
  obj->cas = 0;
  obj->exp_time = exp_time;
  /* cast int64_t(8 bytes) to void* pointer(8 bytes) */
  obj->ptr = reinterpret_cast<void *>(intval);
  return obj;
};

ValueObjectPtr ConstructIntObjPtr(int64_t intval, uint64_t exp_time) {
  void *data_ptr = reinterpret_cast<void *>(intval);
  try {
    return std::make_shared<ValueObject>(OBJECT_INT, GetCurrentSec(), 0, exp_time, data_ptr);
  } catch (const std::bad_alloc& ex) {
    return ValueObjectPtr();
  }
}

ValueObject* ConstructStrObj(const std::string& strval, uint64_t exp_time) {
  ValueObject *obj = new(std::nothrow) ValueObject;
  if (obj == nullptr)
    return nullptr;
  obj->type = OBJECT_STRING;
  obj->last_visit_time = GetCurrentSec();
  obj->cas = 0;
  obj->exp_time = exp_time;
  /* use dynamic string */
  DynamicString *p_str = new DynamicString(strval);
  obj->ptr = p_str;
  return obj;
}

ValueObjectPtr ConstructStrObjPtr(const std::string& strval, uint64_t exp_time) {
  try {
    DynamicString *ds_ptr = new DynamicString(strval);
    return std::make_shared<ValueObject>(OBJECT_STRING, GetCurrentSec(), 0, exp_time, (void*)ds_ptr);
  }
  catch (const std::bad_alloc &ex) {
    return ValueObjectPtr();
  }
}


ValueObject *ConstructDListObj() {
  ValueObject *obj = new(std::nothrow) ValueObject;
  if (obj == nullptr) {
    return nullptr;
  }
  obj->type = OBJECT_LIST;
  obj->last_visit_time = GetCurrentSec();
  obj->cas = 0;
  obj->exp_time = 0;
  /* construct dlist object */
  DList *p_list = new DList;
  obj->ptr = p_list;
  return obj;
}

ValueObjectPtr ConstructDListObjPtr() {
  try {
    DList *list_ptr = new DList;
    return std::make_shared<ValueObject>(OBJECT_LIST, GetCurrentSec(), 0, 0, (void *)list_ptr);
  } catch (const std::bad_alloc &ex) {
    return ValueObjectPtr();
  }
}

uint64_t GetCurrentSec() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec;
}
