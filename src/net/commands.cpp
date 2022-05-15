#include <cassert>
#include <algorithm>
#include <sstream>

#include "commands.h"
#include "../core.h"

std::unordered_map<std::string, CommandHandler> Engine::sOpCommandMap = {
    /* generic command */
    {"overview",  OverviewCommand},/* get database overview  */
    {"total",     NumItemsCommand},/* get database number of items  */
    {"ping",      PingCommand},   /* ping-pong test */
    {"evict",     EvictCommand},   /* evict keys */
    {"del",       DelCommand},    /* delete given keys */
    {"exists",    ExistsCommand}, /* check if given keys exist */
    {"type",      TypeCommand},   /* query object type */
    {"expire",    ExpireCommand}, /* set the key expiration */
    {"expireat",  ExpireAtCommand},/* set key expiration at unix-timestamp*/
    {"ttl",       TTLCommand},    /* get the time to live of a key */
    /* int or string command */
    {"set",       SetCommand},    /* set given key to string or int */
    {"get",       GetCommand},    /* get value on given key */
    /* int command */
    {"incr",      IncrCommand},   /* increase int value by 1 on given key */
    {"decr",      DecrCommand},   /* decrease int value by 1 on given key */
    {"incrby",    IncrByCommand}, /* increase int value by n on given key */
    {"decrby",    DecrByCommand}, /* decrease int value by n on given key */
    /* string command */
    {"strlen",    StrlenCommand}, /* get the len of string on given key */
    {"append",    AppendCommand}, /* append value on given key */
    {"getrange",  GetRangeCommand}, /* TODO not supported, get range value on given key */
    {"setrange",  SetRangeCommand}, /* TODO not supported, set range value on given key */
    /* list command */
    {"llen",      LLenCommand},   /* get the length of list on given key */
    {"lpop",      LPopCommand},   /* left pop one value from list on given key */
    {"lpush",     LPushCommand},  /* left push values from list on given key */
    {"rpop",      RPopCommand},   /* right pop one value from list on given key */
    {"rpush",     RPushCommand},  /* right push values from list on given key */
    {"lrange",    LRangeCommand}, /* get value in range of list on given key */
    {"linsert",   LInsertCommand},/* TODO not supported, insert value into list on given key */
    {"lrem",      LRemCommand},   /* TODO not supported, remove element from list on given key */
    {"lsetindex", LSetCommand},   /* set element from list at index on given key  */
    {"lindex",    LIndexCommand}, /* get element in list at index on given key */
    /* hashtable operation */
    {"hset",      HSetCommand},   /* set field in the hash on given key to value. */
    {"hget",      HGetCommand},   /* return the value associated with field in the hash on given key */
    {"hdel",      HDelCommand},   /* delete specified fields in the hash on given key */
    {"hexists",   HExistsCommand},/* check specified field exists in the hash on given key */
    {"hgetall",   HGetAllCommand},/* get all field-value pairs in the hash on given key */
    {"hkeys",     HKeysCommand},  /* get all fields in the hash on given key */
    {"hvals",     HValsCommand},  /* get all values in the hash on given key */
    {"hlen",      HLenCommand},   /* get number of field-value pairs in the hash on given key */
};

static std::unordered_map<std::string, TimeEvent *> sExpiresMap;
static int sEvictPolicy = EVICTION_POLICY_RANDOM;

#define IfWrongTypeReturn(errcode) \
  if (errcode == kWrongTypeCode) {  \
    return kWrongTypeMsg; \
  }

#define IfKeyNotFoundReturn(errcode)  \
  if (errcode == kKeyNotFoundCode) {  \
    return kNilMsg; \
  }

// FIXME optimize syntax check
static bool CheckSyntax(const CommandCache &cmd, int8_t n_key_required, int8_t n_operands_required, bool even = false) {
  size_t len = cmd.argv.size();
  if (even && (len & 1) != 0) {
    return false;
  }
  if (n_key_required == 0) {
    return len == 1;
  }
  if (n_key_required == 1) {
    if (n_operands_required == -1) {
      return len >= 3;
    } else {
      return len == (size_t) (n_operands_required + 2);
    }
  }
  if (n_key_required == -1) {
    return len >= 2;
  }
  return false;
}

#define CheckSyntaxHelper(cmd, n_key_required, n_operands_required, even, name) \
if(!CheckSyntax(cmd, n_key_required, n_operands_required, even)) { \
  return PackErrMsg("ERROR", "incorrect number of arguments for "#name" command");\
}

static std::string PackIntReply(long value) {
  return kIntPrefix + std::to_string(value) + kCRLF;
}

static std::string PackBoolReply(bool yes) {
  if (yes) {
    return kInt1Msg;
  }
  return kInt0Msg;
}

static std::string PackStringValueReply(const std::string &value) {
  size_t len = value.size();
  return kStrValPrefix + std::to_string(len) + kCRLF + value + kCRLF;
}

static std::string PackStringValueReply(const DynamicString &value) {
  if (value.Null()) {
    return kNilMsg;
  }
  return kStrValPrefix + std::to_string(value.Length()) + kCRLF + value.ToStdString() + kCRLF;
}

static std::string PackStringMsgReply(const std::string &msg) {
  return kStrMsgPrefix + msg + kCRLF;
}

static std::string PackErrMsg(const char *et, const char *msg) {
  std::stringstream ers;
  ers << kErrPrefix << et << ' ' << msg << kCRLF;
  return ers.str();
}

static std::string PackArrayMsg(const std::vector<DynamicString> &array) {
  std::stringstream arr_ss;
  size_t len = array.size();
  arr_ss << kArrayPrefix << len << kCRLF;
  for (const auto &value : array) {
    arr_ss << PackStringValueReply(value);
  }
  return arr_ss.str();
}

static std::string PackEmptyArrayMsg(size_t size) {
  std::stringstream arr_ss;
  arr_ss << kArrayPrefix << size << kCRLF;
  for (size_t i = 0; i < size; ++i) {
    arr_ss << kNilMsg;
  }
  return arr_ss.str();
}

Engine::Engine(KVContainer *container, Config *config) :
    container_(container), config_(config) {
  sEvictPolicy = config_->LruEnabled() ? EVICTION_POLICY_LRU : EVICTION_POLICY_RANDOM;
  std::thread bg_worker(std::bind(&Engine::UpdateMemInfo, this));
  worker_.swap(bg_worker);
}

Engine::~Engine() {
  stopped_.store(true);
  worker_.join();
}

std::string Engine::HandleCommand(EventLoop *loop, const CommandCache &cmds, bool sync) {
  size_t argc = cmds.argc;
  const std::vector<std::string> &argv = cmds.argv;
  assert(argc == argv.size());
  /* Commands contents
  *  +------------+----------+-------------------------+
  *  |   argv[0]  |  argv[1] |   argv[2] ... argv[n]   |
  *  +------------+----------+-------------------------+
  *  |   opcode   |   key    |         operands        |
  *  +------------+----------+-------------------------+
  */
  std::string opcode = argv[0];
  std::transform(opcode.begin(), opcode.end(), opcode.begin(), ::tolower);
  if (!OpCodeValid(opcode)) {
    return kInvalidOpCodeMsg;
  }
  /* TODO slow if too many to evict */
  CommandCache cmd;
  if (sync) {
    appending_->SetAutoFlush(true);
  } else {
    appending_->SetAutoFlush(false);
  }
  /* lazy eviction, once at a time to avoid massive time consumption */
  if (IfNeedKeyEviction()) {
    auto ans = container_->KeyEviction(sEvictPolicy, 16);
    ans.insert(ans.begin(), "del");
    cmd.argc = ans.size();
    cmd.argv = ans;
    cmd.inited = true;
    appending_->Append(cmd);
    cmd.Clear();
  }
  /* sync to control whether sync commands to appending_ for persistence */
  /* sync == true: sync; flag == false: no sync */
  return sOpCommandMap[opcode](loop, container_, appending_, cmds, sync);
}

void Engine::HandleCommand(EventLoop *loop, const CommandCache &cmds, Buffer &out_buf) {
  std::string response = HandleCommand(loop, cmds);
  out_buf.Append(response);
}

bool Engine::OpCodeValid(const std::string &opcode) {
  return sOpCommandMap.find(opcode) != sOpCommandMap.end();
}

bool Engine::RestoreFromAppendableFile(EventLoop *loop, AppendableFile *history) {
  if (history) {
    appending_ = history;
    history->ReadFromScratch(this, loop);
    std::cout << "Database restore from disk...\n";
    return true;
  }
  return false;
}

bool Engine::IfNeedKeyEviction() {
  if (config_) {
    double ratio = config_->LruTriggerRatio();
    size_t mem_limit = config_->MaxMemLimit();  /* in MB */
    return (size_t) ((double) mem_limit * 1024 * ratio) < cur_rss_size_;
  }
  return false;
}

void Engine::UpdateMemInfo() {
  while (!stopped_) {
    CatSelfMemInfo(cur_vm_size_, cur_rss_size_);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

std::string OverviewCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: overview */
  CheckSyntaxHelper(cmds, 0, 0, false, 'overview')
  return PackArrayMsg(holder->Overview());
}

std::string NumItemsCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: total */
  CheckSyntaxHelper(cmds, 0, 0, false, 'total')
  return PackIntReply(holder->NumItems());
}

std::string PingCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: ping */
  CheckSyntaxHelper(cmds, 0, 0, false, 'ping')
  return kPONGMsg;
}

std::string EvictCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: evict number */
  CheckSyntaxHelper(cmds, 1, 0, false, 'evict')
  size_t n;
  const std::string &n_req_del = cmds.argv[1];
  if (!CanConvertToUInt64(n_req_del, n)) {
    return kInvalidIntegerMsg;
  }
  std::vector<std::string> ans = holder->KeyEviction(sEvictPolicy, n);
  if (sync) {
    CommandCache cmd;
    ans.insert(ans.begin(), "del");
    cmd.argc = ans.size();
    cmd.argv = ans;
    cmd.inited = true;
    appendable->Append(cmd);
  }
  return PackIntReply((int)ans.size() - 1);
}

std::string DelCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: del key1 key2 key3 ... */
  CheckSyntaxHelper(cmds, -1, 0, false, 'del')  /* multiple keys supported */
  size_t n = holder->Delete(std::vector<std::string>(cmds.argv.begin() + 1, cmds.argv.end()));
  if (sync) {
    appendable->Append(cmds);
  }
  return PackIntReply(n);
}

std::string ExistsCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: exists key1 key2 key3 ... */
  CheckSyntaxHelper(cmds, -1, 0, false, 'exists')
  auto keys = std::vector<std::string>(cmds.argv.begin() + 1, cmds.argv.end());
  int n = holder->KeyExists(keys);
  return PackIntReply(n);
}

std::string TypeCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: type key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'type');
  int obj_type = holder->QueryObjectType(cmds.argv[1]);
  if (obj_type == OBJECT_INT) {
    return PackStringMsgReply("int");
  } else if (obj_type == OBJECT_STRING) {
    return PackStringMsgReply("string");
  } else if (obj_type == OBJECT_LIST) {
    return PackStringMsgReply("list");
  } else if (obj_type == OBJECT_HASH) {
    return PackStringMsgReply("hash");
  }
  return PackStringMsgReply("none");
}

std::string ExpireCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: expire key time */
  CheckSyntaxHelper(cmds, 1, 1, false, 'expire');
  /* find key */
  const std::string &key = cmds.argv[1];
  if (!holder->KeyExists(key)) {
    return kInt0Msg; /* key not found, can not set expiration */
  }
  int64_t interval; /* beware that this interval is in the unit of second */
  if (CanConvertToInt64(cmds.argv[2], interval)) {
    if (sExpiresMap.find(key) == sExpiresMap.end()) { /* no expiration set for key currently */
      /* create new time event for deletion */
      if (interval > 0) {
        sExpiresMap[key] = loop->AddTimeEvent((uint64_t) interval * 1000ul, [key, holder]() {
          holder->Delete(Key(key));
          sExpiresMap.erase(key);
        }, 1);
      } else if (interval == 0) {
        /* delete right now */
        holder->Delete(key);
      }
    } else {  /* already has expiration on key */
      /* update already existing time event */
      if (sExpiresMap[key] == nullptr) return kInt0Msg;
      long ev_id = sExpiresMap[key]->id;
      if (interval >= 0) {
        if (!loop->UpdateTimeEvent(ev_id, interval * 1000ul, 1)) {
          return kInt0Msg;
        }
      } else {
        /* remove expiration for key */
        if (!loop->RemoveTimeEvent(ev_id)) {
          return kInt0Msg;
        }
        sExpiresMap.erase(key);
      }
    }
    /* if expiration set success, return 1 */
    if (sync) {
      if (interval >= 0) {
        /* sync as expireat format, use absolute timestamp */
        auto now = GetCurrentSec();
        auto &cmd = const_cast<CommandCache &>(cmds);
        cmd.argv[0] = "expireat";
        cmd.argv[2] = std::to_string(now + interval);
        appendable->Append(cmd);
      } else {
        /* remove expiration for key, we can set key to its current value to do it */
        int errcode = 0;
        std::vector<std::string> recovered_cmd = holder->RecoverCommandFromValue(key, errcode);
        if (errcode == kOkCode && !recovered_cmd.empty()) {
          CommandCache cache;
          cache.inited = true;
          cache.argc = recovered_cmd.size();
          cache.argv = recovered_cmd;
          appendable->Append(cache);
        }
      }
    }
    return kInt1Msg;
  }
  return kInvalidIntegerMsg;
}

std::string ExpireAtCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: expireat key unix_sec */
  CheckSyntaxHelper(cmds, 1, 1, false, 'expireat')
  const std::string &key = cmds.argv[1];
  int64_t unix_sec;
  if (CanConvertToInt64(cmds.argv[2], unix_sec)) {
    /* leave to expire command handle it */
    int64_t now = GetCurrentSec();
    int64_t interval = std::max(unix_sec - now, 0l);  /* seconds */
    const_cast<CommandCache &>(cmds).argv[2] = std::to_string(interval); /* force modification to const */
    return ExpireCommand(loop, holder, appendable, cmds, sync);
  }
  return kInvalidIntegerMsg;
}

std::string TTLCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: ttl key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'ttl');
  const std::string &key = cmds.argv[1];
  bool found_in_holder = holder->KeyExists(key);
  if (!found_in_holder) {
    return kIntMinus2Msg;
  }
  bool found_in_expires = sExpiresMap.find(key) != sExpiresMap.end();
  if (!found_in_expires) {
    return kIntMinus1Msg;
  }
  /* get ttl in seconds */
  uint64_t ttl = (sExpiresMap[key]->when - GetCurrentMs()) / 1000;
  return PackIntReply(ttl);
}

std::string SetCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: set key value */
  CheckSyntaxHelper(cmds, 1, 1, false, 'set')
  const std::string &key = cmds.argv[1];
  const std::string &value = cmds.argv[2];
  int64_t val;
  bool res = false;
  if (CanConvertToInt64(value, val)) {
    /* store as integer type */
    res = holder->SetInt(key, val);
  } else {
    /* store as string type */
    res = holder->SetString(key, value);
  }
  if (res) {
    if (sync) {
      appendable->Append(cmds);
    }
    return kOkMsg;
  }
  return kNotOkMsg;
}

std::string GetCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: get key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'get')
  const std::string &key = cmds.argv[1];
  int errcode = 0;
  ValueObjectPtr val = holder->Get(key, errcode);
  if (errcode == kOkCode) {
    if (val->type == OBJECT_INT) {
      int64_t intval = reinterpret_cast<int64_t>(val->ptr);
      return PackStringValueReply(std::to_string(intval));
    } else {
      /* string type underneath */
      return PackStringValueReply(val->ToStdString());
    }
  }
  IfKeyNotFoundReturn(errcode)
  IfWrongTypeReturn(errcode)

  return kNilMsg;
}

#define IfInt64OverflowThenReturn(errcode) \
  if (errcode == kOverflowCode) { \
    return kInt64OverflowMsg; \
  }

#define IncrDecrCommonHelper(cmds, operation, appendable, sync) \
  const std::string &key = cmds.argv[1];  \
  int errcode = 0;  \
  int64_t ans = holder->operation(key, errcode); \
  if (errcode == kOkCode) { \
    if (sync) { \
      appendable->Append(cmds); \
    } \
    return PackIntReply(ans); \
  } \
  IfWrongTypeReturn(errcode)  \
  IfInt64OverflowThenReturn(errcode)  \
  return kNotOkMsg;

#define IncrDecrCommonHelper2(cmds, operation, appendable, sync) \
  const std::string& key = cmds.argv[1];  \
  const std::string& num_str = cmds.argv[2];  \
  int64_t num;  \
  if (!CanConvertToInt64(num_str, num)) { \
    return kInvalidIntegerMsg;  \
  } \
  if (num < 0) {  \
    /* ensure num >= 0 && num <= INT64_MAX */ \
    return kInvalidIntegerMsg;  \
  } \
  int errcode = 0;  \
  int64_t ans = holder->operation(key, num, errcode); \
  if (errcode == kOkCode) { \
    if (sync) { \
      appendable->Append(cmds); \
    } \
    return PackIntReply(ans); \
  } \
  IfWrongTypeReturn(errcode)  \
  IfInt64OverflowThenReturn(errcode)  \
  return kNotOkMsg;

std::string IncrCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: incr key */
  /* This operation is limited to 64 bit signed integers. */
  CheckSyntaxHelper(cmds, 1, 0, false, 'incr')
  IncrDecrCommonHelper(cmds, IncrInt, appendable, sync)
}

std::string DecrCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: decr key */
  /* This operation is limited to 64 bit signed integers. */
  CheckSyntaxHelper(cmds, 1, 0, false, 'decr')
  IncrDecrCommonHelper(cmds, DecrInt, appendable, sync)
}

std::string IncrByCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* incrby key value */
  CheckSyntaxHelper(cmds, 1, 1, false, 'incrby')
  IncrDecrCommonHelper2(cmds, IncrIntBy, appendable, sync)
}

std::string DecrByCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* decrby key value */
  CheckSyntaxHelper(cmds, 1, 1, false, 'decrby')
  IncrDecrCommonHelper2(cmds, DecrIntBy, appendable, sync)
}

#undef IncrDecrCommonHelper
#undef IfInt64OverflowThenReturn

std::string StrlenCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: strlen key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'strlen')
  const std::string &key = cmds.argv[1];
  int errcode = 0;
  size_t len = holder->StrLen(key, errcode);
  if (errcode == kOkCode) {
    return PackIntReply(len);
  }
  IfWrongTypeReturn(errcode)
  return PackIntReply(0);
}

std::string AppendCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: append key value */
  CheckSyntaxHelper(cmds, 1, 1, false, 'append')
  const std::string &key = cmds.argv[1];
  const std::string &value = cmds.argv[2];
  int errcode;
  size_t after_len = holder->Append(key, value, errcode);
  if (errcode == kOkCode) {
    if (sync) {
      appendable->Append(cmds);
    }
    return PackIntReply(after_len);
  }
  IfKeyNotFoundReturn(errcode)
  IfWrongTypeReturn(errcode)
  return kNotOkMsg;
}

std::string GetRangeCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: getrange key begin end */
  return kNotSupportedYetMsg;
}

std::string SetRangeCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  return kNotSupportedYetMsg;
}

std::string LLenCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: llen key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'llen');
  const std::string &key = cmds.argv[1];
  int errcode;
  size_t list_len = holder->ListLen(key, errcode);
  if (errcode == kOkCode) {
    return PackIntReply(list_len);
  }
  IfWrongTypeReturn(errcode)
  return PackIntReply(0);
}

#define ListPopCommandCommon(cmds, operation, appendable, sync) \
  const std::string &key = cmds.argv[1];  \
  int errcode;  \
  auto popped = holder->operation(key, errcode);  \
  if (errcode == kOkCode) { \
    if (!popped.Empty()) {  \
      if (sync) { \
        appendable->Append(cmds); \
      } \
      return PackStringValueReply(popped.ToStdString());  \
    } \
    return kNilMsg; \
  } else if (errcode == kWrongTypeCode) { \
    return kWrongTypeMsg; \
  }

std::string LPopCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: lpop key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'lpop')
  ListPopCommandCommon(cmds, LeftPop, appendable, sync)
  return kNilMsg;
}

std::string RPopCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: rpop key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'rpop')
  ListPopCommandCommon(cmds, RightPop, appendable, sync)
  return kNilMsg;
}

#undef ListPopCommandCommon

#define ListPushCommandCommon(cmds, operation, appendable, sync) \
  const std::string& key = cmds.argv[1];  \
  const std::vector<std::string>& args = cmds.argv; \
  int errcode;  \
  size_t list_len = holder->operation(key, std::vector<std::string>(args.begin() + 2, args.end()), errcode); \
  if (errcode == kOkCode) { \
    if (sync) { \
      appendable->Append(cmds); \
    } \
    return PackIntReply(list_len);  \
  } else if (errcode == kWrongTypeCode) { \
    return kWrongTypeMsg; \
  }

std::string LPushCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: lpush key value1 value2 ... */
  CheckSyntaxHelper(cmds, 1, -1, false, 'lpush')
  ListPushCommandCommon(cmds, LeftPush, appendable, sync)
  return kNotOkMsg;
}

std::string RPushCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: rpush key value1 value2 ... */
  CheckSyntaxHelper(cmds, 1, -1, false, 'rpush')
  ListPushCommandCommon(cmds, RightPush, appendable, sync)
  return kNotOkMsg;
}

#undef ListPushCommandCommon

std::string LRangeCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: lrange key begin end */
  CheckSyntaxHelper(cmds, 1, 2, false, 'lrange')
  const std::string &key = cmds.argv[1];
  const std::string &begin = cmds.argv[2];
  const std::string &end = cmds.argv[3];
  int errcode;
  int begin_idx, end_idx;
  if (!CanConvertToInt32(begin, begin_idx) || !CanConvertToInt32(end, end_idx)) {
    /* range index no valid */
    return kInvalidIntegerMsg;
  }
  std::vector<DynamicString> ranges = holder->ListRange(key, begin_idx, end_idx, errcode);
  if (errcode == kOkCode) {
    if (!ranges.empty()) {
      return PackArrayMsg(ranges);
    }
  } else {
    IfWrongTypeReturn(errcode)
  }
  return kArrayEmptyMsg;
}

std::string LInsertCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* TODO */
  return kNotSupportedYetMsg;
}

std::string LRemCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* TODO */
  return kNotSupportedYetMsg;
}

std::string LSetCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: lsetindex key index value */
  CheckSyntaxHelper(cmds, 1, 2, false, 'lsetindex');
  const std::string &key = cmds.argv[1];
  const std::string &index = cmds.argv[2];
  int idx;
  if (!CanConvertToInt32(index, idx)) {
    return kInvalidIntegerMsg;
  }
  const std::string &value = cmds.argv[3];
  int errcode;
  holder->ListSetItemAtIndex(key, idx, value, errcode);
  if (errcode == kOkCode) {
    if (sync) {
      appendable->Append(cmds);
    }
    return kOkMsg;
  } else if (errcode == kWrongTypeCode) {
    return kWrongTypeMsg;
  } else if (errcode == kOutOfRangeCode) { /* if lsetindex encounters out of range index, we can not set it */
    return kOutOfRangeMsg;
  } else if (errcode == kKeyNotFoundCode) {
    return kNoSuchKeyMsg;
  }
  return kNotOkMsg;
}

std::string LIndexCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: lindex key index */
  CheckSyntaxHelper(cmds, 1, 1, false, 'lindex');
  const std::string &key = cmds.argv[1];
  const std::string &index = cmds.argv[2];
  int idx;
  if (!CanConvertToInt32(index, idx)) {
    return kInvalidIntegerMsg;
  }
  int errcode;
  auto item = holder->ListItemAtIndex(key, idx, errcode);
  if (errcode == kOkCode) {
    return PackStringValueReply(item.ToStdString());
  }
  IfWrongTypeReturn(errcode)
  /* if lindex encounters out of range, or key not found we can simply return nil */
  return kNilMsg;
}

std::string HSetCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: hset key field1 value1 field2 value2 ... */
  CheckSyntaxHelper(cmds, 1, -1, true, 'hset')
  const std::string &key = cmds.argv[1];
  /* extract vector of fields and values */
  size_t n = (cmds.argv.size() - 2) >> 1;
  std::vector<std::string> fields, values;
  fields.reserve(n);
  values.reserve(n);
  for (auto it = cmds.argv.begin() + 2; it != cmds.argv.end(); it += 2) {
    fields.emplace_back(*it);
    values.emplace_back(*(it + 1));
  }
  int errcode;
  int count = holder->HashSetKV(Key(key), fields, values, errcode);
  if (errcode == kOkCode && count != 0) {
    if (sync) {
      appendable->Append(cmds);
    }
    return kOkMsg;
  }
  IfWrongTypeReturn(errcode)
  return kNotOkMsg;
}

std::string HGetCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: hget key field1 field2 ...*/
  CheckSyntaxHelper(cmds, 1, -1, false, 'hget')
  const std::string &key = cmds.argv[1];
  int errcode;
  Key k = Key(key);
  if (cmds.argv.size() == 3) {
    DictVal val = holder->HashGetValue(k, DictKey(cmds.argv[2]), errcode);
    if (errcode == kOkCode) {
      return PackStringValueReply(val);
    }
    IfKeyNotFoundReturn(errcode)
    IfWrongTypeReturn(errcode)
  } else {
    std::vector<std::string> fields(cmds.argv.begin() + 2, cmds.argv.end());
    std::vector<DictVal> values = holder->HashGetValue(k, fields, errcode);
    /* pack into array and return */
    if (!values.empty()) {
      return PackArrayMsg(values);
    }
    /* return query result of every key */
    return PackEmptyArrayMsg(fields.size());
  }
  return kArrayEmptyMsg;
}

std::string HDelCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: hdel key field1 field2 ...*/
  CheckSyntaxHelper(cmds, 1, -1, false, 'hdel')
  const std::string &key = cmds.argv[1];
  int errcode;
  /* get all deleting fields */
  std::vector<std::string> fields(cmds.argv.begin() + 2, cmds.argv.end());
  size_t n_deleted = holder->HashDelField(Key(key), fields, errcode);
  IfWrongTypeReturn(errcode)
  if (sync) {
    appendable->Append(cmds);
  }
  return PackIntReply(n_deleted);
}

std::string HExistsCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: hexists key field */
  CheckSyntaxHelper(cmds, 1, 1, false, 'hexists')
  const std::string &key = cmds.argv[1];
  const std::string &field = cmds.argv[2];
  int errcode;
  auto ans = holder->HashExistField(key, field, errcode);
  IfWrongTypeReturn(errcode)
  return PackBoolReply(ans);
}

#define HKeysValsEntriesCommon(operation) \
  const std::string &key = cmds.argv[1];  \
  int errcode;  \
  auto keys = holder->operation(key, errcode); \
  if (errcode == kWrongTypeCode) {  \
    return kWrongTypeMsg; \
  } \
  if (!keys.empty()) {  \
    return PackArrayMsg(keys);  \
  } \
  return kArrayEmptyMsg; \


std::string HGetAllCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: hgetall key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'hgetall')
  HKeysValsEntriesCommon(HashGetAllEntries)
}

std::string HKeysCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: hkeys key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'hkeys')
  HKeysValsEntriesCommon(HashGetAllFields)
}

std::string HValsCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: hvals key*/
  CheckSyntaxHelper(cmds, 1, 0, false, 'hvals')
  HKeysValsEntriesCommon(HashGetAllValues)
}

#undef HKeysValsEntriesCommon

std::string HLenCommand(EventLoop *loop, KVContainer *holder, AppendableFile *appendable, const CommandCache &cmds, bool sync) {
  /* usage: hlen key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'hlen')
  const std::string &key = cmds.argv[1];
  int errcode;
  size_t len = holder->HashLen(key, errcode);
  IfWrongTypeReturn(errcode)
  return PackIntReply(len);
}

