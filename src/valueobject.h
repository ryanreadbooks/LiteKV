#ifndef __VALUE_OBJECT_H__
#define __VALUE_OBJECT_H__

#include <memory>
#include <string>

#include "serializable.h"
#include "dlist.h"
#include "hashdict.h"
#include "hashset.h"
#include "net/time_event.h"
#include "str.h"

struct ValueObject;

typedef StaticString Key;
typedef std::shared_ptr<StaticString> KeyPtr;
typedef std::shared_ptr<ValueObject> ValueObjectPtr;

#define OBJECT_INT 1    /* int object */
#define OBJECT_STRING 2 /* string object */
#define OBJECT_LIST 3   /* list object */
#define OBJECT_HASH 4   /* hash object */
#define OBJECT_SET 5    /* set object */

/**
 * @brief wrapper for value stored
 *
 */
struct ValueObject {
  /* object type */
  unsigned char type;

  /* last visited time for lru key eviction */
  uint64_t lv_time;

  /* pointer to real content */
  void *ptr;

  ValueObject() = default;

  ValueObject(unsigned char type, void *ptr) : type(type), lv_time(GetCurrentMs()), ptr(ptr) {}

  void FreePtr() {
    if (type != OBJECT_INT && ptr) { /* no need to free int object */
      if (type == OBJECT_STRING) {
        delete reinterpret_cast<DynamicString *>(ptr);
      } else if (type == OBJECT_LIST) {
        delete reinterpret_cast<DList *>(ptr);
      } else if (type == OBJECT_HASH) {
        delete reinterpret_cast<HashDict *>(ptr);
      } else if (type == OBJECT_SET) {
        delete reinterpret_cast<HashSet *>(ptr);
      }
      ptr = nullptr;
    }
  }

  int64_t ToInt64() const {
    if (type != OBJECT_INT) return 0;
    return reinterpret_cast<int64_t>(ptr);
  }

  DynamicString *ToDynamicString() const {
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

  size_t Serialize(std::vector<char> &buf);
};

ValueObject *ConstructIntObj(int64_t intval);

ValueObjectPtr ConstructIntObjPtr(int64_t intval);

ValueObject *ConstructStrObj(const std::string &strval);

ValueObjectPtr ConstructStrObjPtr(const std::string &strval);

ValueObject *ConstructDListObj();

ValueObjectPtr ConstructDListObjPtr();

ValueObject *ConstructHashObj();

ValueObjectPtr ConstructHashObjPtr();

ValueObject *ConstructSetObj();

ValueObjectPtr ConstructSetObjPtr();

#endif  // __VALUE_OBJECT_H__
