#ifndef __VALUE_OBJECT_H__
#define __VALUE_OBJECT_H__

#include <string>
#include <memory>

#include "str.h"
#include "dlist.h"

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
  /* ojbect type */
  unsigned char type;

  /* last visited timestamp */
  uint64_t last_visit_time;

  /* CAS */
  int cas; /* reserved, not used */
  
  /* expiration timestamp (seconds since epoch) */
  uint64_t exp_time;

  /* pointer to real content */
  void *ptr;

  ValueObject() {}

  ValueObject(unsigned char type, uint64_t created_time, int cas, uint64_t exp_time, void* ptr) : 
    type(type), last_visit_time(created_time), cas(cas), 
    exp_time(exp_time), ptr(ptr) {}

  void FreePtr() {
    if (type != OBJECT_INT && ptr) { /* no need to free int object */
      if (type == OBJECT_STRING) {
        delete reinterpret_cast<DynamicString*> (ptr);
      } else if (type == OBJECT_LIST) {
        delete reinterpret_cast<DList *>(ptr);
      } else if (type == OBJECT_HASH) {
        // TODO delete hash object here
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

ValueObject *ConstructIntObj(int64_t intval, uint64_t exp_time);

ValueObjectPtr ConstructIntObjPtr(int64_t intval, uint64_t exp_time);

ValueObject *ConstructStrObj(const std::string &strval, uint64_t exp_time);

ValueObjectPtr ConstructStrObjPtr(const std::string &strval, uint64_t exp_time);

ValueObject *ConstructDListObj();

ValueObjectPtr ConstructDListObjPtr();

uint64_t GetCurrentSec();

#endif  // __VALUE_OBJECT_H__
