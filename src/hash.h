#ifndef __HASH_H__
#define __HASH_H__

#include <type_traits>
#include <vector>
#include "str.h"

typedef DynamicString HEntryKey;
typedef DynamicString HEntryVal;
typedef DynamicString *HEntryKeyPtr;
typedef DynamicString *HEntryValPtr;

#define UNDEFINED -1
#define NEW_ADDED 0
#define UPDATED 1
#define EXISTED 2
#define ERASED 1
#define NOT_ERASED 0

template <typename>
class Rehashable;
class HashDict;
class HashSet;

#define has_member(s, same_type)                                                       \
  template <typename T, typename R = void>                                             \
  struct has_member_##s {                                                              \
    template <typename T_>                                                             \
    static auto check(T_) -> typename std::decay<decltype(T_::s)>::type;               \
    static void check(...);                                                            \
    using type = decltype(check(std::declval<T>()));                                   \
    enum {                                                                             \
      value = same_type ? !std::is_void<type>::value && std::is_same<type, T *>::value \
                        : !std::is_void<type>::value && std::is_same<type, R>::value   \
    };                                                                                 \
  }

/* static check EntryType */
has_member(key, false);
has_member(next, true);

template <typename EntryType>
class HashStructBase {
  template <typename> friend class Rehashable;
  friend class HashDict;
  friend class HashSet;

  static_assert(has_member_key<EntryType, HEntryKey*>::value,
                "EntryType must have a `key` member of type HEntryKey*");
  static_assert(has_member_next<EntryType>::value,
                "EntryType must have a `next` member of type EntryType*");

public:
  explicit HashStructBase(unsigned long init_size = 16) : slot_size_(init_size), count_(0) {
    /* allocate array space */
    table_ = new EntryType *[init_size]; /* all pointers */
    if (table_ != nullptr) {
      memset(table_, 0, sizeof(EntryType *) * init_size);
    } else {
      std::cerr << "unable to allocate " << (sizeof(EntryType *) * init_size)
                << " bytes memory for HashStructBase\n";
    }
  }
  /* free all EntryType node */
  virtual ~HashStructBase() {
    if (table_ != nullptr) {
      for (size_t i = 0; i < slot_size_; ++i) {
        if (table_[i]) {
          EntryType *head = table_[i];
          EntryType *deleting;
          while (head) {
            deleting = head->next;
            delete head;
            head = deleting;
          }
          table_[i] = nullptr;
        }
      }
      delete[] table_; /* free the underneath array from heap */
      table_ = nullptr;
    }
  }

  bool IsValid() const { return table_ != nullptr; }

  int EraseKey(const HEntryKey &key);

  int EraseKey(const std::string &key) { return EraseKey(HEntryKey(key)); }

  std::vector<HEntryKey> AllKeys() const;

  bool CheckExists(const HEntryKey &key) const;

  bool CheckExists(const std::string &key) const { return CheckExists(HEntryKey(key)); }

  inline size_t Count() const { return count_; }

  std::vector<EntryType *> AllEntries() const;

  inline double LoadFactor() const { return ((double)count_) / ((double)slot_size_); }

  inline unsigned long SlotCount() const { return slot_size_; }

protected:
  inline unsigned long CalculateSlotIndex(const HEntryKey &key) const {
    return key.Hash() % slot_size_;
  }

  EntryType *FindEntry(const HEntryKey &key) const;

  long NextOccupiedSlotIdx(unsigned long) const;

  void StepRehashingIdx();

  bool RehashDone() const { return rehashing_idx_ == -1; }

  inline bool EnsureAllSlotsEmpty() const {
    for (size_t i = 0; i < slot_size_; ++i) {
      if (table_[i] != nullptr) {
        return false;
      }
    }
    return true;
  }

protected:
  /* pointer to table array */
  EntryType **table_;
  /* array size (slot size) */
  unsigned long slot_size_;
  /* total number of key-value pairs  */
  unsigned long count_ = 0;
  /* rehashing index: next slot to be moved */
  long rehashing_idx_ = -1;
};

template <typename EntryType>
int HashStructBase<EntryType>::EraseKey(const HEntryKey &key) {
  /* return the number of deleted key, only 0 (key not found) and 1 (key found and deleted) are
   * possible */
  uint64_t slot_idx = CalculateSlotIndex(key);
  EntryType *head = table_[slot_idx];
  if (head == nullptr) return 0;
  EntryType *prev = nullptr;
  while (head != nullptr && *head->key != key) {
    prev = head;
    head = head->next;
  }
  if (head != nullptr) { /* key found */
    if (prev == nullptr) {
      table_[slot_idx] = head->next;
    } else {
      prev->next = head->next;
    }
    delete head;
    --count_;
    return ERASED;
  } else { /* key not found */
    return NOT_ERASED;
  }
}

template <typename EntryType>
std::vector<HEntryKey> HashStructBase<EntryType>::AllKeys() const {
  if (count_ == 0) return {};
  std::vector<HEntryKey> keys;
  for (size_t i = 0; i < slot_size_; ++i) {
    EntryType *head = table_[i];
    while (head) {
      keys.emplace_back(*head->key);
      head = head->next;
    }
  }
  return keys;
}

template <typename EntryType>
bool HashStructBase<EntryType>::CheckExists(const HEntryKey &key) const {
  return FindEntry(key) != nullptr;
}

template <typename EntryType>
std::vector<EntryType *> HashStructBase<EntryType>::AllEntries() const {
  if (count_ == 0) return {};
  std::vector<EntryType *> entries;
  for (size_t i = 0; i < slot_size_; ++i) {
    EntryType *head = table_[i];
    while (head) {
      entries.emplace_back(head);
      head = head->next;
    }
  }
  return entries;
}

template <typename EntryType>
EntryType *HashStructBase<EntryType>::FindEntry(const HEntryKey &key) const {
  uint64_t slot_idx = CalculateSlotIndex(key);
  EntryType *head = table_[slot_idx];
  while (head) {
    if (*head->key == key) {
      return head;
    }
    head = head->next;
  }
  return nullptr;
}

template <typename EntryType>
long HashStructBase<EntryType>::NextOccupiedSlotIdx(unsigned long begin) const {
  for (size_t i = begin + 1; i < slot_size_; ++i) {
    if (table_[i]) {
      return i;
    }
  }
  return -1;
}

template <typename EntryType>
void HashStructBase<EntryType>::StepRehashingIdx() {
  for (size_t i = rehashing_idx_ + 1; i < slot_size_; ++i) {
    if (table_[i]) {
      rehashing_idx_ = i;
      return;
    }
  }
  rehashing_idx_ = -1;
}

#endif  // __HASH_H__