#ifndef __VALUE_OBJECT_H__
#define __VALUE_OBJECT_H__

#include <string>
#include <memory>

#include "str.h"
#include "dlist.h"
#include "dict.h"

struct ValueObject;

typedef StaticString Key;
typedef std::shared_ptr<StaticString> KeyPtr;
typedef std::shared_ptr<ValueObject> ValueObjectPtr;

#define OBJECT_INT 1    /* int object */
#define OBJECT_STRING 2 /* string object */
#define OBJECT_LIST 3   /* list object */
#define OBJECT_HASH 4   /* hash object */

/**
 * @brief wrapper for value stored
 * 
 */
struct ValueObject {
  /* object type */
  unsigned char type;

  /* CAS */
  int cas; /* reserved, not used */

  /* pointer to real content */
  void *ptr;

  ValueObject() {}

  ValueObject(unsigned char type, void* ptr) :
    type(type), cas(0), ptr(ptr) {}

  void FreePtr() {
    if (type != OBJECT_INT && ptr) { /* no need to free int object */
      if (type == OBJECT_STRING) {
        delete reinterpret_cast<DynamicString*> (ptr);
      } else if (type == OBJECT_LIST) {
        delete reinterpret_cast<DList *>(ptr);
      } else if (type == OBJECT_HASH) {
        delete reinterpret_cast<Dict *>(ptr);
      }
      ptr = nullptr;
    }
  }

  int64_t ToInt64() const {
    if (type != OBJECT_INT)
      return 0;
    return reinterpret_cast<int64_t>(ptr);
  }

  DynamicString* ToDynamicString() const {
    if (type != OBJECT_STRING) {
      return nullptr;
    }
    return (DynamicString *)ptr;
  }

  std::string ToStdString() const {
    if (type != OBJECT_STRING) {
      return "";
    }
    return ((DynamicString *)ptr)->ToStdString();
  }

  ~ValueObject() {
    if (ptr != nullptr) {
      FreePtr();
    }
  }
};

ValueObject *ConstructIntObj(int64_t intval);

ValueObjectPtr ConstructIntObjPtr(int64_t intval);

ValueObject *ConstructStrObj(const std::string &strval);

ValueObjectPtr ConstructStrObjPtr(const std::string &strval);

ValueObject *ConstructDListObj();

ValueObjectPtr ConstructDListObjPtr();

ValueObject *ConstructHashObj();

ValueObjectPtr ConstructHashObjPtr();

#endif  // __VALUE_OBJECT_H__
