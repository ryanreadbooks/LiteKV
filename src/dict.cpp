#include "dict.h"

int HashTable::UpdateKV(const DictKey &key, const DictKey &val) {
  uint64_t slot_idx = CalculateSlotIndex(key);
  int added = NEW_ADDED;
  if (table_[slot_idx] == nullptr) { /* given key nor found, create new item */
    table_[slot_idx] = new HTEntry(new DictKey(key), new DictVal(val));
    ++count_;
  } else {
    /* check if key is on linked list */
    HTEntry *entry = FindHTEntry(key);
    if (entry == nullptr) {
      /* add new HTEntry at the head of linked list */
      HTEntry *new_entry = new HTEntry(new DictKey(key), new DictVal(val));
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
      HTEntry *entry = FindHTEntry(*new_entry->key);
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

int HashTable::EraseKey(const DictKey &key) {
  /* return the number of deleted key, only 0 (key not found) and 1 (key found and deleted) are possible */
  uint64_t slot_idx = CalculateSlotIndex(key);
  HTEntry *head = table_[slot_idx];
  if (head == nullptr) return 0;
  HTEntry *prev = nullptr;
  while (head != nullptr && *head->key != key) {
    prev = head;
    head = head->next;
  }
  if (head != nullptr) {  /* key found */
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

std::vector<DictKey> HashTable::AllKeys() const {
  if (count_ == 0) return {};
  std::vector<DictKey> keys;
  for (size_t i = 0; i < slot_size_; ++i) {
    HTEntry *head = table_[i];
    while (head) {
      keys.emplace_back(*head->key);
      head = head->next;
    }
  }
  return keys;
}

std::vector<DictVal> HashTable::AllValues() const {
  if (count_ == 0) return {};
  std::vector<DictVal> values;
  for (size_t i = 0; i < slot_size_; ++i) {
    HTEntry *head = table_[i];
    while (head) {
      values.emplace_back(*head->value);
      head = head->next;
    }
  }
  return values;
}

bool HashTable::CheckExists(const DictKey &key) const {
  return FindHTEntry(key) != nullptr;
}

std::vector<HTEntry *> HashTable::AllEntries() const {
  if (count_ == 0) return {};
  std::vector<HTEntry *> entries;
  for (size_t i = 0; i < slot_size_; ++i) {
    HTEntry *head = table_[i];
    while (head) {
      entries.emplace_back(head);
      head = head->next;
    }
  }
  return entries;
}

DictVal &HashTable::At(const DictKey &key) {
  /* if key does not match the key of any element in the container,
   * the function throw out_of_range exception
   */
  HTEntry *entry = FindHTEntry(key);
  if (entry == nullptr) {
    throw std::out_of_range(key.ToStdString() + " not found in hashtable");
  }
  return *entry->value;
}

HTEntry *HashTable::FindHTEntry(const DictKey &key) const {
  uint64_t slot_idx = CalculateSlotIndex(key);
  HTEntry *head = table_[slot_idx];
  while (head) {
    if (*head->key == key) {
      return head;
    }
    head = head->next;
  }
  return nullptr;
}

long HashTable::NextOccupiedSlotIdx(unsigned long begin) const {
  for (size_t i = begin + 1; i < slot_size_; ++i) {
    if (table_[i]) {
      return i;
    }
  }
  return -1;
}

void HashTable::StepRehashingIdx() {
  for (size_t i = rehashing_idx_ + 1; i < slot_size_; ++i) {
    if (table_[i]) {
      rehashing_idx_ = i;
      return;
    }
  }
  rehashing_idx_ = -1;
}

Dict::Dict(double max_load_factor) : cur_ht_(new HashTable),
                                     max_load_factor_(max_load_factor) {
}

int Dict::Update(const DictKey &key, const DictVal &val) {
  if (CheckNeedRehash()) {
    PerformRehash();
  }
  if (cur_ht_->RehashDone()) {
    return cur_ht_->UpdateKV(key, val);
  } else {
    if (cur_ht_ != nullptr) {
      HTEntry* entry = cur_ht_->FindHTEntry(key);
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

int Dict::Erase(const DictKey &key) {
  if (CheckNeedRehash()) {
    PerformRehash();
  }
  if (cur_ht_ != nullptr && cur_ht_->EraseKey(key) == ERASED) {
    return ERASED;
  }
  return (backup_ht_ != nullptr && backup_ht_->EraseKey(key) == ERASED) ? ERASED : NOT_ERASED;
}

std::vector<DictKey> Dict::AllKeys() const {
  if (Count() == 0) return {};
  std::vector<DictKey> keys;
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

std::vector<DictVal> Dict::AllValues() const {
  if (Count() == 0) return {};
  std::vector<DictVal> values;
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

bool Dict::CheckExists(const DictKey &key) {
  if (cur_ht_ && cur_ht_->CheckExists(key)) {
    return true;
  }
  return backup_ht_ && backup_ht_->CheckExists(key);
}

std::vector<HTEntry *> Dict::AllEntries() const {
  if (Count() == 0) return {};
  std::vector<HTEntry *> entries;
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

DictVal &Dict::At(const DictKey &key) {
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

bool Dict::PerformRehash() {
  /* rehashing implementation */
  if (backup_ht_ == nullptr) {
    size_t new_slot_size = cur_ht_->slot_size_ * kGrowFactor;
    backup_ht_ = new HashTable(new_slot_size);
  }
  if (!cur_ht_->RehashDone()) {
    HTEntry *head = cur_ht_->table_[cur_ht_->rehashing_idx_];
    cur_ht_->table_[cur_ht_->rehashing_idx_] = nullptr; /* disconnect with slot */
    /* move every entry from old hashtable to new hashtable */
    HTEntry *next = nullptr;
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

void Dict::RehashMoveSlots(int n) {
  do {
    if (PerformRehash()) {
      break;
    }
    --n;
  } while (n > 0);
}


