#include "valueobject.h"
#include <typeinfo>

size_t ValueObject::Serialize(std::vector<char> &buf) {
  if (type != OBJECT_INT && ptr == nullptr) {
    return 0;
  }
  /* we encode this value as variable integer */
  if (type == OBJECT_INT) {
    unsigned char tmp[10] = {0};
    /* encode signed 64-bit integer the same way as encoding unsigned 64-bit integer */
    uint8_t enc_len = EncodeVarSignedInt64(ToInt64(), tmp);
    buf.insert(buf.end(), tmp, tmp + enc_len);
  } else {
    if (type == OBJECT_HASH) {
      reinterpret_cast<HashDict *>(ptr)->Serialize(buf);
    } else if (type == OBJECT_SET) {
      reinterpret_cast<HashSet *>(ptr)->Serialize(buf);
    } else {
      Serializable *parent = reinterpret_cast<Serializable *>(ptr);
      parent->Serialize(buf);
    }
  }
  return buf.size();
}

// FIXME: use macro to remove code redundancy

ValueObject *ConstructIntObj(int64_t intval) {
  ValueObject *obj = new (std::nothrow) ValueObject;
  if (obj == nullptr) return nullptr;
  obj->type = OBJECT_INT;
  obj->lv_time = GetCurrentMs();
  /* cast int64_t(8 bytes) to void* pointer(8 bytes) */
  obj->ptr = reinterpret_cast<void *>(intval);
  return obj;
};

ValueObjectPtr ConstructIntObjPtr(int64_t intval) {
  void *data_ptr = reinterpret_cast<void *>(intval);
  try {
    return std::make_shared<ValueObject>(OBJECT_INT, data_ptr);
  } catch (const std::bad_alloc &ex) {
    return ValueObjectPtr();
  }
}

ValueObject *ConstructStrObj(const std::string &strval) {
  ValueObject *obj = new (std::nothrow) ValueObject;
  if (obj == nullptr) return nullptr;
  obj->type = OBJECT_STRING;
  obj->lv_time = GetCurrentMs();
  /* use dynamic string */
  DynamicString *p_str = new (std::nothrow) DynamicString(strval);
  if (p_str == nullptr) return nullptr;
  obj->ptr = p_str;
  return obj;
}

ValueObjectPtr ConstructStrObjPtr(const std::string &strval) {
  try {
    DynamicString *ds_ptr = new DynamicString(strval);
    return std::make_shared<ValueObject>(OBJECT_STRING, (void *)ds_ptr);
  } catch (const std::bad_alloc &ex) {
    return ValueObjectPtr();
  }
}

ValueObject *ConstructDListObj() {
  ValueObject *obj = new (std::nothrow) ValueObject;
  if (obj == nullptr) {
    return nullptr;
  }
  obj->type = OBJECT_LIST;
  obj->lv_time = GetCurrentMs();
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
  ValueObject *obj = new (std::nothrow) ValueObject;
  if (obj == nullptr) {
    return nullptr;
  }
  obj->type = OBJECT_HASH;
  obj->lv_time = GetCurrentMs();
  HashDict *p_dict = new (std::nothrow) HashDict;
  if (p_dict == nullptr) return nullptr;
  obj->ptr = p_dict;
  return obj;
}

ValueObjectPtr ConstructHashObjPtr() {
  try {
    HashDict *dict_ptr = new HashDict;
    return std::make_shared<ValueObject>(OBJECT_HASH, (void *)dict_ptr);
  } catch (const std::bad_alloc &ex) {
    return ValueObjectPtr();
  }
}

ValueObject *ConstructSetObj() {
  ValueObject *obj = new (std::nothrow) ValueObject;
  if (obj == nullptr) {
    return nullptr;
  }
  obj->type = OBJECT_SET;
  obj->lv_time = GetCurrentMs();
  HashSet *p_set = new (std::nothrow) HashSet;
  if (p_set == nullptr) return nullptr;
  obj->ptr = p_set;
  return obj;
}

ValueObjectPtr ConstructSetObjPtr() {
  try {
    HashSet *set_ptr = new HashSet;
    return std::make_shared<ValueObject>(OBJECT_SET, (void *)set_ptr);
  } catch (const std::bad_alloc &ex) {
    return ValueObjectPtr();
  }
}