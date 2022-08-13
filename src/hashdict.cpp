#include "hashdict.h"

int HashTable::UpdateKV(const HEntryKey &key, const HEntryKey &val) {
  uint64_t slot_idx = CalculateSlotIndex(key);
  int added = NEW_ADDED;
  if (table_[slot_idx] == nullptr) { /* given key nor found, create new item */
    table_[slot_idx] = new HTEntry(new HEntryKey(key), new HEntryVal(val));
    ++count_;
  } else {
    /* check if key is on linked list */
    HTEntry *entry = FindEntry(key);
    if (entry == nullptr) {
      /* add new HTEntry at the head of linked list */
      HTEntry *new_entry = new HTEntry(new HEntryKey(key), new HEntryVal(val));
      HTEntry *head = table_[slot_idx];
      table_[slot_idx] = new_entry;
      new_entry->next = head;
      ++count_;
    } else { /* update existing value on given key */
      added = UPDATED;
      entry->value->Reset(val);
    }
  }
  return added; /* return 0 means new added, return 1 means update existing key */
}

int HashTable::AddEntry(HTEntry *new_entry) {
  int added = NEW_ADDED;
  if (new_entry != nullptr) {
    uint64_t slot_idx = CalculateSlotIndex(*new_entry->key);
    if (table_[slot_idx] == nullptr) {
      table_[slot_idx] = new_entry;
      ++count_;
    } else {
      HTEntry *entry = FindEntry(*new_entry->key);
      if (entry == nullptr) { /* place entry at the head of list*/
        HTEntry *head = table_[slot_idx];
        table_[slot_idx] = new_entry;
        new_entry->next = head;
        ++count_;
      } else { /* update existing entry */
        added = UPDATED;
        entry->value->Reset(*new_entry->value); /* FIXME: seems unreasonable */
      }
    }
  }
  return added;
}

std::vector<HEntryVal> HashTable::AllValues() const {
  if (count_ == 0) return {};
  std::vector<HEntryVal> values;
  for (size_t i = 0; i < slot_size_; ++i) {
    HTEntry *head = table_[i];
    while (head) {
      values.emplace_back(*head->value);
      head = head->next;
    }
  }
  return values;
}

HEntryVal &HashTable::At(const HEntryKey &key) {
  /* if key does not match the key of any element in the container,
   * the function throw out_of_range exception
   */
  HTEntry *entry = FindEntry(key);
  if (entry == nullptr) {
    throw std::out_of_range(key.ToStdString() + " not found in hashtable");
  }
  return *entry->value;
}

/***********　HashDict impl　************/

//HashDict::HashDict(double max_load_factor) : cur_ht_(new HashTable),
//                                     max_load_factor_(max_load_factor) {
//}

int HashDict::Update(const HEntryKey &key, const HEntryVal &val) {
  if (CheckNeedRehash()) {
    PerformRehash();
  }
  if (cur_ht_->RehashDone()) {
    return cur_ht_->UpdateKV(key, val);
  } else {
    if (cur_ht_ != nullptr) {
      HTEntry* entry = cur_ht_->FindEntry(key);
      if (entry != nullptr) { /* this is an update operation */
        entry->value->Reset(val);
        return UPDATED;
      }
    }
    /* this is an add-new-element operation */
    if (backup_ht_ != nullptr) { /* insert into new hashtable */
      return backup_ht_->UpdateKV(key, val);
    }
  }
  return UNDEFINED;
}

//int HashDict::Erase(const HEntryKey &key) {
//  if (CheckNeedRehash()) {
//    PerformRehash();
//  }
//  if (cur_ht_ != nullptr && cur_ht_->EraseKey(key) == ERASED) {
//    return ERASED;
//  }
//  return (backup_ht_ != nullptr && backup_ht_->EraseKey(key) == ERASED) ? ERASED : NOT_ERASED;
//}

//std::vector<HEntryKey> HashDict::AllKeys() const {
//  if (Count() == 0) return {};
//  std::vector<HEntryKey> keys;
//  if (cur_ht_ != nullptr) {
//    auto tmp = cur_ht_->AllKeys();
//    keys.insert(keys.end(), tmp.begin(), tmp.end());
//  }
//  if (backup_ht_ != nullptr) {
//    auto tmp = backup_ht_->AllKeys();
//    keys.insert(keys.end(), tmp.begin(), tmp.end());
//  }
//  return keys;
//}

std::vector<HEntryVal> HashDict::AllValues() const {
  if (Count() == 0) return {};
  std::vector<HEntryVal> values;
  if (cur_ht_ != nullptr) {
    auto tmp = cur_ht_->AllValues();
    values.insert(values.end(), tmp.begin(), tmp.end());
  }
  if (backup_ht_ != nullptr) {
    auto tmp = backup_ht_->AllValues();
    values.insert(values.end(), tmp.begin(), tmp.end());
  }
  return values;
}

//bool HashDict::CheckExists(const HEntryKey &key) {
//  if (cur_ht_ && cur_ht_->CheckExists(key)) {
//    return true;
//  }
//  return backup_ht_ && backup_ht_->CheckExists(key);
//}

//std::vector<HTEntry *> HashDict::AllEntries() const {
//  if (Count() == 0) return {};
//  std::vector<HTEntry *> entries;
//  if (cur_ht_ != nullptr) {
//    auto tmp = cur_ht_->AllEntries();
//    entries.insert(entries.end(), tmp.begin(), tmp.end());
//  }
//  if (backup_ht_ != nullptr) {
//    auto tmp = backup_ht_->AllEntries();
//    entries.insert(entries.end(), tmp.begin(), tmp.end());
//  }
//  return entries;
//}

HEntryVal &HashDict::At(const HEntryKey &key) {
  if (CheckNeedRehash()) {
    PerformRehash();
  }
  if (cur_ht_ != nullptr) {
    try {
      return cur_ht_->At(key);
    } catch (const std::out_of_range &ex) {
      if (backup_ht_ != nullptr) {
        return backup_ht_->At(key);
      }
    }
  }
  throw std::out_of_range(key.ToStdString() + " not found in hashtable");
}

//bool HashDict::PerformRehash() {
//  /* rehashing implementation */
//  if (backup_ht_ == nullptr) {
//    size_t new_slot_size = cur_ht_->slot_size_ * kGrowFactor;
//    backup_ht_ = new HashTable(new_slot_size);
//  }
//  if (!cur_ht_->RehashDone()) {
//    HTEntry *head = cur_ht_->table_[cur_ht_->rehashing_idx_];
//    cur_ht_->table_[cur_ht_->rehashing_idx_] = nullptr; /* disconnect with slot */
//    /* move every entry from old hashtable to new hashtable */
//    HTEntry *next = nullptr;
//    while (head) {
//      next = head->next;
//      head->next = nullptr;
//      --(cur_ht_->count_);
//      backup_ht_->AddEntry(head);
//      head = next;
//    }
//    cur_ht_->StepRehashingIdx();
//  }
//  /* check if entry movement already finished */
//  if (cur_ht_->RehashDone() && cur_ht_->EnsureAllSlotsEmpty()) {
//    /* free old hashtable and set backup hashtable to new hashtable */
//    delete cur_ht_;
//    cur_ht_ = backup_ht_;
//    backup_ht_ = nullptr;
//    return true;
//  }
//  return false;
//}

//void HashDict::RehashMoveSlots(int n) {
//  do {
//    if (PerformRehash()) {
//      break;
//    }
//    --n;
//  } while (n > 0);
//}


