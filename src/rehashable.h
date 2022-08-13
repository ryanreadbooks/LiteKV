#ifndef __REHASHABLE_H__
#define __REHASHABLE_H__

#include <type_traits>
#include "hash.h"

constexpr static int kGrowFactor = 2;

/* class describing the rehashing behaviour in hashtable or hashset */
template <typename ImplType>
class Rehashable {
  static_assert(std::is_base_of<HashStructBase<typename ImplType::entry_type>, ImplType>::value,
                "ImplType must be children of HashStructBase<EntryType>");

public:
  typedef typename ImplType::entry_type EntryType;

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

  int Erase(const HEntryKey &key);

  int Erase(const std::string &key) { return Erase(HEntryKey(key)); }

  std::vector<HEntryKey> AllKeys() const;

  bool CheckExists(const HEntryKey &key);

  bool CheckExists(const std::string &key) { return CheckExists(HEntryKey(key)); }

  inline size_t Count() const {
    if (cur_ht_ != nullptr && backup_ht_ == nullptr) return cur_ht_->count_;
    if (cur_ht_ == nullptr && backup_ht_ != nullptr) return backup_ht_->count_;
    if (cur_ht_ != nullptr && backup_ht_ != nullptr) return cur_ht_->count_ + backup_ht_->count_;
    return 0;
  }

  std::vector<EntryType *> AllEntries() const;

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

template <typename ImplType>
int Rehashable<ImplType>::Erase(const HEntryKey &key) {
  if (CheckNeedRehash()) {
    PerformRehash();
  }
  if (cur_ht_ != nullptr && cur_ht_->EraseKey(key) == ERASED) {
    return ERASED;
  }
  return (backup_ht_ != nullptr && backup_ht_->EraseKey(key) == ERASED) ? ERASED : NOT_ERASED;

}

template <typename ImplType>
std::vector<HEntryKey> Rehashable<ImplType>::AllKeys() const {
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

template <typename ImplType>
bool Rehashable<ImplType>::CheckExists(const HEntryKey &key) {
  if (cur_ht_ && cur_ht_->CheckExists(key)) {
    return true;
  }
  return backup_ht_ && backup_ht_->CheckExists(key);
}

template <typename ImplType>
std::vector<typename Rehashable<ImplType>::EntryType *> Rehashable<ImplType>::AllEntries() const {
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

template <typename ImplType>
bool Rehashable<ImplType>::PerformRehash() {
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

template <typename ImplType>
void Rehashable<ImplType>::RehashMoveSlots(int n) {
  do {
    if (PerformRehash()) {
      break;
    }
    --n;
  } while (n > 0);
}

#endif  // __REHASHABLE_H__