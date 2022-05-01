#ifndef __DICT_H__
#define __DICT_H__

#include <vector>
#include "str.h"

typedef DynamicString DictKey;
typedef DynamicString DictVal;
typedef DynamicString *DictKeyPtr;
typedef DynamicString *DictValPtr;

#define kGrowFactor 2

class HashTable;
class Dict;

#define UNDEFINED -1
#define NEW_ADDED 0
#define UPDATED 1
#define ERASED 1
#define NOT_ERASED 0

struct HTEntry {
  DictKey *key = nullptr;
  DictVal *value = nullptr;
  HTEntry *next = nullptr;

  HTEntry(const char *key, const char *value) :
      key(new DictKey(key, strlen(key))),
      value(new DictVal(value, strlen(value))) {}

  HTEntry(const char *key, size_t keylen, const char *value, size_t vallen) :
      key(new DictKey(key, keylen)),
      value(new DictVal(value, vallen)) {}

  HTEntry(DictKey *key, DictVal *value) : key(key), value(value) {}

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

class HashTable {
  friend class Dict;

public:
  HashTable(unsigned long init_size = 16) : slot_size_(init_size), count_(0) {
    /* allocate array space */
    table_ = new(std::nothrow) HTEntry *[init_size];  /* all pointers */
    if (table_ == nullptr) {
      std::cerr << "unable to allocate " << (sizeof(HTEntry *) * init_size)
                << " bytes memory for hashtable\n";
      abort();
    }
    memset(table_, 0, sizeof(HTEntry *) * init_size);
  }

  ~HashTable() {
    if (table_ != nullptr) {
      for (size_t i = 0; i < slot_size_; ++i) {
        if (table_[i]) {
          HTEntry *head = table_[i];
          HTEntry *deleting;
          while (head) {
            deleting = head->next;
            delete head;
            head = deleting;
          }
          table_[i] = nullptr;
        }
      }
      delete[] table_;
      table_ = nullptr;
    }
  }

  int UpdateKV(const DictKey &key, const DictKey &val);

  int UpdateKV(const std::string &key, const std::string &val) {
    return UpdateKV(DictKey(key), DictVal(val));
  }

  int AddEntry(HTEntry* new_entry);

  int EraseKey(const DictKey &key);

  int EraseKey(const std::string &key) {
    return EraseKey(DictKey(key));
  }

  std::vector<DictKey> AllKeys() const;

  std::vector<DictVal> AllValues() const;

  bool CheckExists(const DictKey &key) const;

  bool CheckExists(const std::string &key) const {
    return CheckExists(DictKey(key));
  }

  inline size_t Count() const { return count_; }

  std::vector<HTEntry *> AllEntries() const;

  DictVal &At(const DictKey &key);

  DictVal &At(const std::string &key) {
    return At(DictKey(key));
  }

  inline double LoadFactor() const {
    return ((double) count_) / ((double) slot_size_);
  }

  inline unsigned long SlotCount() const { return slot_size_; }

private:
  inline unsigned long CalculateSlotIndex(const DictKey &key) const {
    return key.Hash() % slot_size_;
  }

  HTEntry *FindHTEntry(const DictKey &key) const;

  long NextOccupiedSlotIdx(unsigned long) const;

  void StepRehashingIdx();

  bool RehashDone() const { return rehashing_idx_ == -1; }

  inline bool EnsureAllSlotsEmpty() const {
    for (size_t i = 0 ; i < slot_size_; ++i) {
      if (table_[i] != nullptr) {
        return false;
      }
    }
    return true;
  }

private:
  /* pointer to table array */
  HTEntry **table_;
  /* array size (slot size) */
  unsigned long slot_size_;
  /* total number of key-value pairs  */
  unsigned long count_ = 0;
  /* rehashing index: next slot to be moved */
  long rehashing_idx_ = -1;
};

class Dict {
public:
  Dict(double max_load_factor = 1.0);

  ~Dict() {
    if (cur_ht_ != nullptr) {
      delete cur_ht_;
      cur_ht_ = nullptr;
    }
    if (backup_ht_ != nullptr) {
      delete backup_ht_;
      backup_ht_ = nullptr;
    }
  }

  /* api */
  int Update(const DictKey &key, const DictVal &val);

  int Update(const std::string &key, const std::string &val) {
    return Update(DictKey(key), DictVal(val));
  }

  int Erase(const DictKey &key);

  int Erase(const std::string &key) {
    return Erase(DictKey(key));
  }

  std::vector<DictKey> AllKeys() const;

  std::vector<DictVal> AllValues() const;

  bool CheckExists(const DictKey &key);

  bool CheckExists(const std::string &key) {
    return CheckExists(DictKey(key));
  }

  inline size_t Count() const {
    if (cur_ht_ != nullptr && backup_ht_ == nullptr) return cur_ht_->count_;
    if (cur_ht_ == nullptr && backup_ht_ != nullptr) return backup_ht_->count_;
    if (cur_ht_ != nullptr && backup_ht_ != nullptr) return cur_ht_->count_ + backup_ht_->count_;
    return 0;
  }

  std::vector<HTEntry *> AllEntries() const;

  DictVal &At(const DictKey &key);

  DictVal &At(const std::string &key) {
    return At(DictKey(key));
  }

private:
  void PerformRehash();

  inline bool CheckNeedRehash() const {
    bool need_rehash = (cur_ht_->rehashing_idx_ == -1 && cur_ht_->LoadFactor() > max_load_factor_)
                       || cur_ht_->rehashing_idx_ != -1;
    if (need_rehash && cur_ht_->rehashing_idx_ == -1) {
      cur_ht_->StepRehashingIdx();
    }
    return need_rehash;
  }

  void RehashMoveSlots(int n);

  void CheckRehashAndPerform();

private:
  /* master hashtable */
  HashTable *cur_ht_ = nullptr;
  /* hashtable for gradual rehashing */
  HashTable *backup_ht_ = nullptr;
  /* maximum load factor allowed */
  double max_load_factor_ = 1.0;
};

#endif // __DICT_H__