#ifndef __LKVDB_H__
#define __LKVDB_H__

#include <vector>
#include <string>

#include "str.h"

#define LKVDB_VERSION 1
#define LKVDB_MAGIC_NUMBER "LITEKV"

#define LKVDB_ITEM_START_FLAG 0xFF /* the flag to indicate an entry starts */
#define LKVDB_ITEM_END_FLAG 0XFE   /* the flag to indicate an entry ends */

class LKVDb {
public:
  explicit LKVDb();

  void Save(const std::string& src, const std::vector<char>& buf);

  static void Load(const std::string& src);

};

#endif  // __LKVDB_H__