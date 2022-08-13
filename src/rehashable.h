#ifndef __REHASHABLE_H__
#define __REHASHABLE_H__

#include "hash.h"

constexpr static int kGrowFactor = 2;

// TODO(220813): ensure ImplType must be children of HashStructBase
template <typename ImplType, typename EntryType>
class Rehashable {
public:
  explicit Rehashable(double max_load_factor = 1.0)
      : cur_ht_(new ImplType), max_load_factor_(max_load_factor) {}

  virtual ~Rehashable() {
    /* free cur_ht_ and backup_ht_ space */
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
  // dict特有
  //  int Update(const HEntryKey &key, const HEntryVal &val);

  // dict特有
  //  int Update(const std::string &key, const std::string &val) {
  //    return Update(HEntryKey(key), HEntryVal(val));
  //  }

  // 保留
  int Erase(const HEntryKey &key);

  // 保留
  int Erase(const std::string &key) { return Erase(HEntryKey(key)); }

  std::vector<HEntryKey> AllKeys() const;

  // dict特有
  //  std::vector<HEntryVal> AllValues() const;

  bool CheckExists(const HEntryKey &key);

  bool CheckExists(const std::string &key) { return CheckExists(HEntryKey(key)); }

  inline size_t Count() const {
    if (cur_ht_ != nullptr && backup_ht_ == nullptr) return cur_ht_->count_;
    if (cur_ht_ == nullptr && backup_ht_ != nullptr) return backup_ht_->count_;
    if (cur_ht_ != nullptr && backup_ht_ != nullptr) return cur_ht_->count_ + backup_ht_->count_;
    return 0;
  }

  std::vector<EntryType *> AllEntries() const;

  //  HEntryVal &At(const HEntryKey &key);

  //  HEntryVal &At(const std::string &key) {
  //    return At(HEntryKey(key));
  //  }

protected:
  bool PerformRehash();

  inline bool CheckNeedRehash() const {
    bool need_rehash =
        (cur_ht_->rehashing_idx_ == -1 && cur_ht_->LoadFactor() > max_load_factor_) ||
        cur_ht_->rehashing_idx_ != -1;
    if (need_rehash && cur_ht_->rehashing_idx_ == -1) {
      cur_ht_->StepRehashingIdx();
    }
    return need_rehash;
  }

  void RehashMoveSlots(int n);

protected:
  /* master hash container */
  ImplType *cur_ht_ = nullptr;
  /* backup hash container for gradual rehashing */
  ImplType *backup_ht_ = nullptr;
  /* maximum load factor allowed */
  double max_load_factor_ = 1.0;
};

template <typename ImplType, typename EntryType>
int Rehashable<ImplType, EntryType>::Erase(const HEntryKey &key) {
  if (CheckNeedRehash()) {
    PerformRehash();
  }
  if (cur_ht_ != nullptr && cur_ht_->EraseKey(key) == ERASED) {
    return ERASED;
  }
  return (backup_ht_ != nullptr && backup_ht_->EraseKey(key) == ERASED) ? ERASED : NOT_ERASED;

}

template <typename ImplType, typename EntryType>
std::vector<HEntryKey> Rehashable<ImplType, EntryType>::AllKeys() const {
  if (Count() == 0) return {};
  std::vector<HEntryKey> keys;
  if (cur_ht_ != nullptr) {
    auto tmp = cur_ht_->AllKeys();
    keys.insert(keys.end(), tmp.begin(), tmp.end());
  }
  if (backup_ht_ != nullptr) {
    auto tmp = backup_ht_->AllKeys();
    keys.insert(keys.end(), tmp.begin(), tmp.end());
  }
  return keys;
}

template <typename ImplType, typename EntryType>
bool Rehashable<ImplType, EntryType>::CheckExists(const HEntryKey &key) {
  if (cur_ht_ && cur_ht_->CheckExists(key)) {
    return true;
  }
  return backup_ht_ && backup_ht_->CheckExists(key);
}

template <typename ImplType, typename EntryType>
std::vector<EntryType *> Rehashable<ImplType, EntryType>::AllEntries() const {
  if (Count() == 0) return {};
  std::vector<EntryType *> entries;
  if (cur_ht_ != nullptr) {
    auto tmp = cur_ht_->AllEntries();
    entries.insert(entries.end(), tmp.begin(), tmp.end());
  }
  if (backup_ht_ != nullptr) {
    auto tmp = backup_ht_->AllEntries();
    entries.insert(entries.end(), tmp.begin(), tmp.end());
  }
  return entries;
}

template <typename ImplType, typename EntryType>
bool Rehashable<ImplType, EntryType>::PerformRehash() {
  /* rehashing implementation */
  if (backup_ht_ == nullptr) {
    size_t new_slot_size = cur_ht_->slot_size_ * kGrowFactor;
    backup_ht_ = new ImplType(new_slot_size);
  }
  if (!cur_ht_->RehashDone()) {
    EntryType *head = cur_ht_->table_[cur_ht_->rehashing_idx_];
    cur_ht_->table_[cur_ht_->rehashing_idx_] = nullptr; /* disconnect with slot */
    /* move every entry from old hashtable to new hashtable */
    EntryType *next = nullptr;
    while (head) {
      next = head->next;
      head->next = nullptr;
      --(cur_ht_->count_);
      backup_ht_->AddEntry(head);
      head = next;
    }
    cur_ht_->StepRehashingIdx();
  }
  /* check if entry movement already finished */
  if (cur_ht_->RehashDone() && cur_ht_->EnsureAllSlotsEmpty()) {
    /* free old hashtable and set backup hashtable to new hashtable */
    delete cur_ht_;
    cur_ht_ = backup_ht_;
    backup_ht_ = nullptr;
    return true;
  }
  return false;
}

template <typename ImplType, typename EntryType>
void Rehashable<ImplType, EntryType>::RehashMoveSlots(int n) {
  do {
    if (PerformRehash()) {
      break;
    }
    --n;
  } while (n > 0);
}

#endif  // __REHASHABLE_H__