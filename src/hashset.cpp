#include "hashset.h"

int HashSetImpl::AddEntry(HSEntry* new_entry) {
  if (new_entry != nullptr) {
    int added = NEW_ADDED;
    uint64_t slot_idx = CalculateSlotIndex(*new_entry->key);
    if (table_[slot_idx] == nullptr) {
      table_[slot_idx] = new_entry;
      ++count_;
    } else {
      HSEntry *entry = FindEntry(*new_entry->key);
      if (entry == nullptr) {/* place entry at the head of list*/
        HSEntry *head = table_[slot_idx];
        table_[slot_idx] = new_entry;
        new_entry->next = head;
        ++count_;
      } else { /* item already exists */
        return EXISTED;
      }
    }
    return added;
  }
  return UNDEFINED;
}

int HashSetImpl::Add(const HEntryKey &item) {
  uint64_t slot_idx = CalculateSlotIndex(item);
  int added = NEW_ADDED;
  if (table_[slot_idx] == nullptr) {/* given key nor found, create new item */
    table_[slot_idx] = new HSEntry(new HEntryKey(item));
    ++count_;
  } else {
    /* maybe item already exists */
    HSEntry *entry = FindEntry(item);
    if (entry == nullptr) {
      /* add new HSEntry at the head of linked list */
      HSEntry *new_entry = new HSEntry(new HEntryKey(item));
      HSEntry *head = table_[slot_idx];
      table_[slot_idx] = new_entry;
      new_entry->next = head;
      ++count_;
    } else {
      /* item already exists, do noting */
      added = EXISTED;
    }
  }
  return added;
}

/***********　HashSet impl　************/

int HashSet::Insert(const HEntryKey &item) {
  if (CheckNeedRehash()) {
    PerformRehash();
  }
  if (cur_ht_->RehashDone()) {
    return cur_ht_->Add(item);
  } else { /* in the middle of rehashing */
    if (cur_ht_ != nullptr) {
      HSEntry *entry = cur_ht_->FindEntry(item);
      if (entry != nullptr) {
        /* no operation is needed cause item already exists */
        return EXISTED;
      }
    }
    /* we are in the middle of performing rehashing,
     * newly inserted item should be inserted into backup */
    if (backup_ht_ != nullptr) {
      return backup_ht_->Add(item);
    }
  }
  return UNDEFINED;
}
