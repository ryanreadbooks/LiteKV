#ifndef __HASHDICT_H__
#define __HASHDICT_H__

#include <vector>
#include "hash.h"
#include "rehashable.h"
#include "str.h"

class HashTable;
class HashDict;

/* each entry for hashtable, each entry has key, value,
 * and a pointer pointing to the next entry instance */
struct HTEntry {
  HEntryKey *key = nullptr;
  HEntryVal *value = nullptr;
  HTEntry *next = nullptr;

  HTEntry(const char *key, const char *value)
      : key(new HEntryKey(key, strlen(key))), value(new HEntryVal(value, strlen(value))) {}

  HTEntry(const char *key, size_t keylen, const char *value, size_t vallen)
      : key(new HEntryKey(key, keylen)), value(new HEntryVal(value, vallen)) {}

  HTEntry(HEntryKey *key, HEntryVal *value) : key(key), value(value) {}

  ~HTEntry() {
    if (key != nullptr) {
      delete key;
      key = nullptr;
    }
    if (value != nullptr) {
      delete value;
      value = nullptr;
    }
    next = nullptr;
  }
};

class HashTable : public HashStructBase<HTEntry> {
  friend class HashDict;

public:
  typedef HTEntry entry_type;

  explicit HashTable(unsigned long init_size = 16) : HashStructBase<HTEntry>(init_size) {}

  int UpdateKV(const HEntryKey &key, const HEntryKey &val);

  int UpdateKV(const std::string &key, const std::string &val) {
    return UpdateKV(HEntryKey(key), HEntryVal(val));
  }

  /* needed for rehashing */
  int AddEntry(HTEntry *new_entry);

  std::vector<HEntryVal> AllValues() const;

  HEntryVal &At(const HEntryKey &key);

  HEntryVal &At(const std::string &key) { return At(HEntryKey(key)); }
};

/* implementation for hash structure */
class HashDict : public Rehashable<HashTable> {
public:
  explicit HashDict(double max_load_factor = 1.0)
      : Rehashable<HashTable>(max_load_factor) {}

  /**
   * Update key-value
   */
  int Update(const HEntryKey &key, const HEntryVal &val);

  /**
   * Update key-value
   */
  int Update(const std::string &key, const std::string &val) {
    return Update(HEntryKey(key), HEntryVal(val));
  }

  /**
   * Get all values from hashdict
   */
  std::vector<HEntryVal> AllValues() const;

  /**
   * Get value given specific key
   */
  HEntryVal &At(const HEntryKey &key);

  /**
   * Get value given specific key
   */
  HEntryVal &At(const std::string &key) { return At(HEntryKey(key)); }
};

#endif  // __HASHDICT_H__