#include "lkvdb.h"

#define LKV_NOT_RECOGNIZED_MSG "LiteKV binary format not recognized. Parsing exited.\n"

#define CHECK_REMAIN_BYTES(target)       \
  if (remain < target) {                 \
    std::cerr << LKV_NOT_RECOGNIZED_MSG; \
    return;                              \
  }

#define AddTimerEventToKey()                                                \
  do {                                                                      \
    sExpiresMap[std_key] = loop->AddTimeEvent(interval,                     \
                                              [&key, &std_key, holder]() {  \
                                                holder->Delete(Key(key));   \
                                                sExpiresMap.erase(std_key); \
                                              },                            \
                                              1);                           \
  } while (0)

void LiteKVSave(const std::string& dst, const std::vector<char>& buf) {
  if (dst.empty()) return;
  std::string tmp_dst = "tmp_" + dst;
  std::ofstream ofs(tmp_dst, std::ios::trunc | std::ios::out); /* always truncate existing file */

  if (!ofs.is_open()) {
    std::cerr << "Can not create file " << tmp_dst << " for lkvdb persistence\n";
    return;
  }

  /* magic number and version come first */
  char magic[strlen(LKVDB_MAGIC_NUMBER) + 4 + 1];
  snprintf(magic, sizeof(magic), "%s%04d", LKVDB_MAGIC_NUMBER, LKVDB_VERSION);
  ofs.write(magic, LKVDB_HEADER_SIZE); /*magic number is 10 bytes: LITEKVXXXX */

  /* put buffer into ofstream */
  ofs.write(buf.data(), buf.size());

  ofs.flush();
  ofs.close();

  rename(tmp_dst.c_str(), dst.c_str());
}

void LiteKVBackgroundSave(const std::string& dst, const std::vector<char>& buf) {
  /* we use fork to perform background save */
}

static void Advance(char*& cursor, size_t& remain, size_t step) {
  cursor += step;
  remain -= step;
}

/* decode an integer from cursor */
static uint64_t DecodeInteger(char*& cursor, size_t& remain) {
  uint64_t valen = 0;
  uint8_t i = 0;
  for (uint32_t shift = 0; shift <= 63 && i < 10; shift += 7, i++) {
    if (remain < 1) {
      return valen;
    }
    uint64_t byte = *(reinterpret_cast<uint8_t*>(cursor));
    Advance(cursor, remain, 1);
    if (byte & 0b10000000) {
      valen |= ((byte & 0b01111111) << shift);
    } else {
      valen |= (byte << shift);
      return valen;
    }
  }
  return valen;
}

/* decode a string from cursor */
static StaticString DecodeStaticString(char*& cursor, size_t& remain) {
  uint64_t str_len = DecodeInteger(cursor, remain);
  if (str_len == 0 || remain < str_len) {
    return StaticString();
  }
  /* copy the next str_len bytes */
  StaticString ans(cursor, str_len);
  Advance(cursor, remain, str_len);
  return ans;
}

/* decode a string from cursor */
static std::string DecodeStdString(char*& cursor, size_t& remain) {
  uint64_t str_len = DecodeInteger(cursor, remain);
  if (str_len == 0 || remain < str_len) {
    return "";
  }
  /* copy the next str_len bytes */
  std::string ans(cursor, str_len);
  Advance(cursor, remain, str_len);
  return ans;
}

static DList& DecodeDList(char*& cursor, size_t& remain, DList& ans) {
  /* the length of the list */
  size_t count = DecodeInteger(cursor, remain);
  for (size_t i = 0; i < count; ++i) {
    std::string elem = DecodeStdString(cursor, remain);
    if (elem.empty()) {
      break;
    }
    ans.PushRight(elem.data(), elem.size());
  }
  return ans;
}

static HashDict& DecodeHashDict(char*& cursor, size_t& remain, HashDict& ans) {
  size_t count = DecodeInteger(cursor, remain);
  for (size_t i = 0; i < count; ++i) {
    std::string key = DecodeStdString(cursor, remain);
    if (key.empty()) {
      break;
    }
    std::string value = DecodeStdString(cursor, remain);
    if (value.empty()) {
      break;
    }
    ans.Update(key, value);
  }
  return ans;
}

static HashSet& DecodeHashSet(char*& cursor, size_t& remain, HashSet& ans) {
  size_t count = DecodeInteger(cursor, remain);
  for (size_t i = 0; i < count; ++i) {
    std::string elem = DecodeStdString(cursor, remain);
    if (elem.empty()) {
      break;
    }
    ans.Insert(elem);
  }
  return ans;
}

void LiteKVLoad(const std::string& src, KVContainer* holder, EventLoop* loop,
                const std::function<void(size_t&, size_t&)>& callback) {
  if (!holder || !loop) return;
  if (src.empty()) return;
  FileFdRAII handle(src.c_str(), O_RDONLY);
  int fd = handle.Fd();

  if (fd == -1) {
    std::cerr << "Can not open file " << src << "(" << strerror(errno) << ")\n";
    return;
  }
  struct stat fd_stat = {0};
  if (fstat(fd, &fd_stat) == -1) {
    std::cerr << "Can not get fstat for" << src << "(" << strerror(errno) << ")\n";
    return;
  }
  size_t srcfile_size = fd_stat.st_size;

  if (srcfile_size < LKVDB_HEADER_SIZE) {
    std::cerr << LKV_NOT_RECOGNIZED_MSG;
    return;
  }

  MmapRAII mmap_handle(fd, srcfile_size, PROT_READ, MAP_PRIVATE, 0);
  void* cptr = mmap_handle.Ptr();
  if (cptr == nullptr) {
    std::cerr << "Can not mmap for file " << src << "(" << strerror(errno) << ")\n";
    return;
  }

  /* read content from mmap and reconstruct all data */
  /* header (magic number + version) comes first */
  char* cursor = (char*)cptr;
  size_t remain = srcfile_size;

  MagicHeader header = {.magic = {0}, .version = {0}};
  memcpy(&header, cursor, sizeof(header));
  if (strncmp(header.magic, "LITEKV", 6) != 0) {
    std::cerr << LKV_NOT_RECOGNIZED_MSG;
    return;
  }
  std::cout << "LiteKV binary format version : " << header.version[0] << ' ' << header.version[1]
            << ' ' << header.version[2] << ' ' << header.version[3] << '\n';
  Advance(cursor, remain, sizeof(header));

  /* next 8 bytes are interpreted as an uin64_t, representing the number of
   * records stored */
  CHECK_REMAIN_BYTES(8);
  uint64_t len = *(uint64_t*)(cursor);
  std::cout << len << " records are stored in binary\n";
  Advance(cursor, remain, 8);

  /* start reading the rest binary data item by item */
  for (size_t idx = 1; idx <= len; ++idx) {
    /* must start with 0xFF */
    CHECK_REMAIN_BYTES(1);
    uint8_t start_flag = *cursor;
    if (start_flag != LKVDB_ITEM_START_FLAG) {
      std::cerr << LKV_NOT_RECOGNIZED_MSG;
      return;
    }
    Advance(cursor, remain, 1);

    /* then is item 1-byte type */
    CHECK_REMAIN_BYTES(1);
    int type = (int)(char)(*cursor);
    Advance(cursor, remain, 1);
    if (type != LKVBD_TYPE_INT && type != LKVBD_TYPE_STRING && type != LKVBD_TYPE_LIST &&
        type != LKVBD_TYPE_HASH && type != LKVBD_TYPE_SET) {
      std::cerr << LKV_NOT_RECOGNIZED_MSG;
      return;
    }
    std::cout << "=========\n";
    std::cout << "type = " << type << '\n';

    /* then comes expiration flag */
    CHECK_REMAIN_BYTES(1);
    int expire_flag = (int)(char)(*cursor);
    Advance(cursor, remain, 1);
    uint64_t exp_timestamp = 0;
    if (expire_flag) {
      /* next 8 bytes form expiration timestamp */
      CHECK_REMAIN_BYTES(8);
      exp_timestamp = *(uint64_t*)(cursor);
      Advance(cursor, remain, 8);
    }

    /* starts reading key-value data */
    /* key comes first, variable length appears first */
    StaticString key = DecodeStaticString(cursor, remain);
    std::string std_key = key.ToStdString();
    if (!key.Empty()) {
      std::cout << "Key is " << key << '\n';
    }

    /* then comes value, interpreting it depending on its type */
    uint64_t current = GetCurrentMs();
    uint64_t interval = exp_timestamp - current;
    int errcode;
    if (type == LKVBD_TYPE_INT) {
      int64_t val = DecodeInteger(cursor, remain);
      if ((expire_flag && exp_timestamp > current) || !expire_flag) {
        holder->SetInt(key, val);
        if (expire_flag) {
          /* set timer to expire this key */
          AddTimerEventToKey();
        }
        std::cout << "val = " << val << '\n';
      }
    } else if (type == LKVBD_TYPE_STRING) {
      std::string str = DecodeStdString(cursor, remain);
      if ((expire_flag && exp_timestamp > current) || !expire_flag) {
        holder->SetString(key, str);
        std::cout << "str = " << str << '\n';
        if (expire_flag) {
          AddTimerEventToKey();
        }
      }
    } else if (type == LKVBD_TYPE_LIST) {
      DList li;
      DecodeDList(cursor, remain, li);
      if ((expire_flag && exp_timestamp > current) || !expire_flag) {
        for (auto& item : li.RangeAsStdStringVector()) {
          holder->RightPush(key, item, errcode);
          std::cout << item << ", ";
        }
        std::cout << '\n';
        if (expire_flag) {
          AddTimerEventToKey();
        }
      }
    } else if (type == LKVBD_TYPE_HASH) {
      HashDict hd;
      DecodeHashDict(cursor, remain, hd);
      if ((expire_flag && exp_timestamp > current) || !expire_flag) {
        for (auto& item : hd.AllEntries()) {
          std::cout << *(item->key) << " |-> " << *(item->value) << ", ";
          holder->HashUpdateKV(key, *(item->key), *(item->value), errcode);
        }
        std::cout << '\n';
        if (expire_flag) {
          AddTimerEventToKey();
        }
      }
    } else if (type == LKVBD_TYPE_SET) {
      HashSet hs;
      DecodeHashSet(cursor, remain, hs);
      if ((expire_flag && exp_timestamp > current) || !expire_flag) {
        for (auto& item : hs.AllEntries()) {
          holder->SetAddItem(key, *(item->key), errcode);
          std::cout << *(item->key) << ' ';
        }
        std::cout << '\n';
        if (expire_flag) {
          AddTimerEventToKey();
        }
      }
    } else {
      std::cerr << LKV_NOT_RECOGNIZED_MSG;
      return;
    }

    /* must end with 0xFE*/
    CHECK_REMAIN_BYTES(1)
    uint8_t end_flag = (uint8_t)(char)(*cursor);
    Advance(cursor, remain, 1);
    if (end_flag != LKVDB_ITEM_END_FLAG) {
      std::cerr << LKV_NOT_RECOGNIZED_MSG;
      return;
    }
    std::cout << "+++++++++\n";
    if (callback) {
      callback(len, idx);
    }
  }
}
