#ifndef __BIN_OBJ_H__
#define __BIN_OBJ_H__

#include <vector>
#include <cstddef>

#define LKVBD_TYPE_KEY 0
#define LKVBD_TYPE_INT 1
#define LKVBD_TYPE_STRING 2
#define LKVBD_TYPE_LIST 3
#define LKVBD_TYPE_HASH 4
#define LKVBD_TYPE_SET 5

class Serializable {
public:
  virtual size_t Serialize(std::vector<char>& buf) const = 0;

  virtual ~Serializable() = default;
};

#endif  // __BIN_OBJ_H__