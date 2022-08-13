#ifndef __HASHSET_H__
#define __HASHSET_H__

#include "hash.h"
#include "rehashable.h"

class HashSetImpl;
class HashSet;

/* each entry for hashset_impl, each entry has key,
 * and a pointer pointing to the next entry instance */
struct HSEntry {
  HEntryKey *key = nullptr;
  HSEntry *next = nullptr;

  explicit HSEntry(const char *key) : key(new HEntryKey(key, strlen(key))) {}

  HSEntry(const char *key, size_t keylen) : key(new HEntryKey(key, keylen)) {}

  explicit HSEntry(HEntryKey *key) : key(key) {}

  ~HSEntry() {
    if (key != nullptr) {
      delete key;
      key = nullptr;
    }
    next = nullptr;
  }
};

/* hashset implementation */
class HashSetImpl : public HashStructBase<HSEntry> {
  friend class HashSet;

public:
  typedef HSEntry entry_type;

  explicit HashSetImpl(unsigned long init_size = 16) : HashStructBase<HSEntry>(init_size) {}

  int AddEntry(HSEntry* new_entry);

  /* add new item */
  int Add(const HEntryKey &item);

  int Add(const std::string& item) {
    return Add(HEntryKey(item));
  }
};

/* set implementation based on HashSetImpl */
class HashSet : public Rehashable<HashSetImpl> {
public:
  explicit HashSet(double max_load_factor = 1.0) : Rehashable<HashSetImpl>(max_load_factor) {}

  int Insert(const HEntryKey &item);

  int Insert(const std::string& item) {
    return Insert(HEntryKey(item));
  }
};

#endif // __HASHSET_H__