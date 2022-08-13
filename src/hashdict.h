#ifndef __HASHDICT_H__
#define __HASHDICT_H__

#include <vector>
#include "str.h"
#include "hash.h"
#include "rehashable.h"


class HashTable;
class HashDict;

struct HTEntry {
  HEntryKey *key = nullptr;
  HEntryVal *value = nullptr;
  HTEntry *next = nullptr;

  HTEntry(const char *key, const char *value) :
      key(new HEntryKey(key, strlen(key))),
      value(new HEntryVal(value, strlen(value))) {}

  HTEntry(const char *key, size_t keylen, const char *value, size_t vallen) :
      key(new HEntryKey(key, keylen)),
      value(new HEntryVal(value, vallen)) {}

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
  explicit HashTable(unsigned long init_size = 16) : HashStructBase<HTEntry>(init_size) {}

  int UpdateKV(const HEntryKey &key, const HEntryKey &val);

  int UpdateKV(const std::string &key, const std::string &val) {
    return UpdateKV(HEntryKey(key), HEntryVal(val));
  }

  int AddEntry(HTEntry* new_entry);

  std::vector<HEntryVal> AllValues() const;

  HEntryVal &At(const HEntryKey &key);

  HEntryVal &At(const std::string &key) {
    return At(HEntryKey(key));
  }
};

class HashDict : public Rehashable<HashTable, HTEntry> {
public:
  explicit HashDict(double max_load_factor = 1.0) : Rehashable<HashTable, HTEntry>(max_load_factor) {}

//  ~HashDict() {
//    if (cur_ht_ != nullptr) {
//      delete cur_ht_;
//      cur_ht_ = nullptr;
//    }
//    if (backup_ht_ != nullptr) {
//      delete backup_ht_;
//      backup_ht_ = nullptr;
//    }
//  }

  /* api */
  int Update(const HEntryKey &key, const HEntryVal &val);

  int Update(const std::string &key, const std::string &val) {
    return Update(HEntryKey(key), HEntryVal(val));
  }
//
//  int Erase(const HEntryKey &key);
//
//  int Erase(const std::string &key) {
//    return Erase(HEntryKey(key));
//  }

//  std::vector<HEntryKey> AllKeys() const;

  std::vector<HEntryVal> AllValues() const;

//  bool CheckExists(const HEntryKey &key);

//  bool CheckExists(const std::string &key) {
//    return CheckExists(HEntryKey(key));
//  }
//
//  inline size_t Count() const {
//    if (cur_ht_ != nullptr && backup_ht_ == nullptr) return cur_ht_->count_;
//    if (cur_ht_ == nullptr && backup_ht_ != nullptr) return backup_ht_->count_;
//    if (cur_ht_ != nullptr && backup_ht_ != nullptr) return cur_ht_->count_ + backup_ht_->count_;
//    return 0;
//  }

//  std::vector<HTEntry *> AllEntries() const;

  HEntryVal &At(const HEntryKey &key);

  HEntryVal &At(const std::string &key) {
    return At(HEntryKey(key));
  }

//private:
//  bool PerformRehash();
//
//  inline bool CheckNeedRehash() const {
//    bool need_rehash = (cur_ht_->rehashing_idx_ == -1 && cur_ht_->LoadFactor() > max_load_factor_)
//                       || cur_ht_->rehashing_idx_ != -1;
//    if (need_rehash && cur_ht_->rehashing_idx_ == -1) {
//      cur_ht_->StepRehashingIdx();
//    }
//    return need_rehash;
//  }
//
//  void RehashMoveSlots(int n);
//
//private:
//  /* master hashtable */
//  HashTable *cur_ht_ = nullptr;
//  /* hashtable for gradual rehashing */
//  HashTable *backup_ht_ = nullptr;
//  /* maximum load factor allowed */
//  double max_load_factor_ = 1.0;
};

#endif // __HASHDICT_H__