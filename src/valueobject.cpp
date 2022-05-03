#include "valueobject.h"

ValueObject* ConstructIntObj(int64_t intval) {
  ValueObject *obj = new(std::nothrow) ValueObject;
  if (obj == nullptr)
    return nullptr;
  obj->type = OBJECT_INT;
  obj->cas = 0;
  /* cast int64_t(8 bytes) to void* pointer(8 bytes) */
  obj->ptr = reinterpret_cast<void *>(intval);
  return obj;
};

ValueObjectPtr ConstructIntObjPtr(int64_t intval) {
  void *data_ptr = reinterpret_cast<void *>(intval);
  try {
    return std::make_shared<ValueObject>(OBJECT_INT, data_ptr);
  } catch (const std::bad_alloc& ex) {
    return ValueObjectPtr();
  }
}

ValueObject* ConstructStrObj(const std::string& strval) {
  ValueObject *obj = new(std::nothrow) ValueObject;
  if (obj == nullptr)
    return nullptr;
  obj->type = OBJECT_STRING;
  obj->cas = 0;
  /* use dynamic string */
  DynamicString *p_str = new (std::nothrow) DynamicString(strval);
  if (p_str == nullptr) return nullptr;
  obj->ptr = p_str;
  return obj;
}

ValueObjectPtr ConstructStrObjPtr(const std::string& strval) {
  try {
    DynamicString *ds_ptr = new DynamicString(strval);
    return std::make_shared<ValueObject>(OBJECT_STRING, (void*)ds_ptr);
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
  obj->cas = 0;
  /* construct dlist object */
  DList *p_list = new (std::nothrow) DList;
  if (p_list == nullptr) return nullptr;
  obj->ptr = p_list;
  return obj;
}

ValueObjectPtr ConstructDListObjPtr() {
  try {
    DList *list_ptr = new DList;
    return std::make_shared<ValueObject>(OBJECT_LIST, (void *)list_ptr);
  } catch (const std::bad_alloc &ex) {
    return ValueObjectPtr();
  }
}

ValueObject *ConstructHashObj() {
  ValueObject *obj = new(std::nothrow) ValueObject;
  if (obj == nullptr) {
    return nullptr;
  }
  obj->type = OBJECT_HASH;
  obj->cas = 0;
  Dict* p_dict = new (std::nothrow) Dict;
  if (p_dict == nullptr) return nullptr;
  obj->ptr = p_dict;
  return obj;
}

ValueObjectPtr ConstructHashObjPtr() {
  try {
    Dict *dict_ptr = new Dict;
    return std::make_shared<ValueObject>(OBJECT_HASH, (void *)dict_ptr);
  } catch (const std::bad_alloc &ex) {
    return ValueObjectPtr();
  }
}

