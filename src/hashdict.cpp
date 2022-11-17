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
  if (new_entry != nullptr) {
    int added = NEW_ADDED;
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
    return added;
  }
  return UNDEFINED;
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

int HashDict::Update(const HEntryKey &key, const HEntryVal &val) {
  if (CheckNeedRehash()) {
    PerformRehash();
  }
  if (cur_ht_->RehashDone()) {
    return cur_ht_->UpdateKV(key, val);
  } else {
    if (cur_ht_ != nullptr) {
      HTEntry *entry = cur_ht_->FindEntry(key);
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

size_t HashDict::Serialize(std::vector<char> &buf) const {
  size_t len = Count();
  if (len == 0) {
    return 0;
  }
  unsigned char len_enc_buf[10] = {0};
  uint8_t len_enc_size = EncodeVarUnsignedInt64(len, len_enc_buf);
  /* put number of entry first */
  buf.insert(buf.end(), len_enc_buf, len_enc_buf + len_enc_size);

  // std::vector<char> tmp_buf_key, tmp_buf_val;
  // tmp_buf_key.reserve(16);
  // tmp_buf_val.reserve(16);

  /* iterate every entries and serialize them to binary data */
  for (auto &entry : AllEntries()) {
    entry->key->Serialize(buf);
    entry->value->Serialize(buf);
  }
  return buf.size();
}