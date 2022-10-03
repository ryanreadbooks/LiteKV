#include <cassert>
#include <algorithm>
#include <sstream>
#include <chrono>

#include "commands.h"
#include "../core.h"
#include "utils.h"

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
    /* hash operation */
    {"hset",      HSetCommand},   /* set field in the hash on given key to value. */
    {"hget",      HGetCommand},   /* return the value associated with field in the hash on given key */
    {"hdel",      HDelCommand},   /* delete specified fields in the hash on given key */
    {"hexists",   HExistsCommand},/* check specified field exists in the hash on given key */
    {"hgetall",   HGetAllCommand},/* get all field-value pairs in the hash on given key */
    {"hkeys",     HKeysCommand},  /* get all fields in the hash on given key */
    {"hvals",     HValsCommand},  /* get all values in the hash on given key */
    {"hlen",      HLenCommand},   /* get number of field-value pairs in the hash on given key */
    /* set operation */
    {"sadd",        SAddCommand},         /* add members into the set */
    {"sismember",   SIsMemberCommand},    /* check a member is inside set */
    {"smismember",  SMIsMemberCommand},   /* check multiple members are inside set */
    {"smembers",    SMembersCommand},     /* get all members inside set */
    {"srem",        SRemCommand},         /* remove the specified member inside set */
    {"scard",       SCardCommand},        /* get the number of members inside set */
    {"spop",        SPopCommand},         /* TODO not supported yet, pop a member from set */
    /* pub/sub operations */
    {"publish",   PubSubPublishCommand},        /* publish a message to specific channel */
    {"subscribe", PubSubSubscribeCommand},      /* subscribe to specific channels */
    {"unsubscribe", PubSubUnsubscribeCommand},  /* unsubscribe from specific channels */
};

static int sEvictPolicy = EVICTION_POLICY_RANDOM;

#define IfFailReturn(errcode, retval) \
  do {                                \
    if (errcode == kFailCode) {       \
      return retval;                  \
    }                                 \
  } while (0)

#define IfWrongTypeReturn(errcode)   \
  do {                               \
    if (errcode == kWrongTypeCode) { \
      return kWrongTypeMsg;          \
    }                                \
  } while (0)

#define IfKeyNotFoundReturn(errcode)   \
  do {                                 \
    if (errcode == kKeyNotFoundCode) { \
      return kNilMsg;                  \
    }                                  \
  } while (0)

#define AddIntoAppendableDirectly(cmds) \
  do {                                  \
    if (sync && appendable) {           \
      appendable->Append(cmds);         \
    }                                   \
  } while (0)

// FIXME optimize syntax check
static bool CheckSyntax(const CommandCache &cmd, int8_t n_key_required, int8_t n_operands_required, bool even = false) {
  size_t len = cmd.argv.size();
  if (even && (len & 1) != 0) { /* if even flag is true, the number of argv (len) must be even */
    return false;
  }
  if (n_key_required == 0) { /* no key is required, len must be 1 */
    return len == 1;
  }
  if (n_key_required == 1) {  /* single key is required */
    if (n_operands_required == -1) {  /* but the number of operands is unlimited */
      return len >= 3;
    } else {
      return len == (size_t) (n_operands_required + 2);
    }
  }
  if (n_key_required == -1) { /* number of keys not limited, but len should be at least 2 */
    return len >= 2;
  }
  if (n_key_required == -2) { /* the number of key is not limited, can be one or more */
    return len >= 1;
  }
  return false;
}

#define CheckSyntaxHelper(cmd, n_key_required, n_operands_required, even, name)\
  do {                                                                         \
    if (!CheckSyntax(cmd, n_key_required, n_operands_required, even)) {        \
      return PackErrMsg("ERROR", "incorrect number of arguments for " #name    \
                                 " command");                                  \
    }                                                                          \
  } while (0)

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
  return kStrValPrefix + std::to_string(value.Length()) + kCRLF + value.ToStdString() + kCRLF;
}

static void PackStringValueIntoStream(std::stringstream& ss, const std::string& value) {
  ss << kStrValPrefix << value.size() << kCRLF << value << kCRLF;
}

static void PackStringValueIntoStream(std::stringstream& ss, const DynamicString& value) {
  if (!value.Null()) {
    ss << kStrValPrefix << value.Length() << kCRLF << value << kCRLF;
  } else {
    ss << kNilMsg;
  }
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
    // arr_ss << PackStringValueReply(value);
    PackStringValueIntoStream(arr_ss, value);
  }
  return arr_ss.str();
}

static std::string PackArrayMsg(const std::vector<std::string> &array) {
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

static std::string PackIntArrayMsg(const std::vector<int>& array) {
  std::stringstream ss;
  ss << kArrayPrefix << array.size() << kCRLF;
  for (const int& elem : array) {
    ss << kIntPrefix << elem << kCRLF;
  }
  return ss.str();
}

Engine::Engine(KVContainer *container, Config *config) :
    container_(container), config_(config) {
  assert(config_ != nullptr);
  sEvictPolicy = config_->LruEnabled() ? EVICTION_POLICY_LRU : EVICTION_POLICY_RANDOM;
  std::thread bg_worker(std::bind(&Engine::UpdateMemInfo, this));
  worker_.swap(bg_worker);
}

Engine::~Engine() {
  stopped_.store(true);
  worker_.join();
}

std::string Engine::HandleCommand(EventLoop *loop, CommandCache &cmds, bool sync, Session* sess, OptionalHandlerParams* params) {
  size_t argc = cmds.argc;
  std::vector<std::string> &argv = cmds.argv;
  assert(argc == argv.size());
  /* Commands contents
  *  +------------+----------+-------------------------+
  *  |   argv[0]  |  argv[1] |   argv[2] ... argv[n]   |
  *  +------------+----------+-------------------------+
  *  |   opcode   |   key    |         operands        |
  *  +------------+----------+-------------------------+
  */
  std::transform(argv[0].begin(), argv[0].end(), argv[0].begin(), ::tolower);
  std::string opcode = argv[0];
  if (!OpCodeValid(opcode)) {
    return kInvalidOpCodeMsg;
  }
  CommandCache cmd;
  if (appending_) {
    if (sync) {
      appending_->SetAutoFlush(true);
    } else {
      appending_->SetAutoFlush(false);
    }
  }
  /* lazy eviction, once at a time to avoid massive time consumption */
  /* FIXME: this is slow if too many keys to evict */
  if (IfNeedKeyEviction()) {
    /* we can get the evicted keys from ans */
    auto ans = container_->KeyEviction(sEvictPolicy, 16);
    ans.insert(ans.begin(), "del");
    cmd.argc = ans.size();
    cmd.argv = ans;
    cmd.inited = true;
    if (appending_) {
      appending_->Append(cmd);
    }
    cmd.Clear();
  }
  /* sync to control whether sync commands to appending_ for persistence */
  /* sync == true: sync; flag == false: no sync */
  return sOpCommandMap[opcode](loop, container_, appending_, cmds, sync, sess, params);
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

#define __PARAMETERS_LIST EventLoop *loop, KVContainer *holder, AppendableFile *appendable, \
                          const CommandCache &cmds, bool sync, Session* sess, OptionalHandlerParams* params

std::string OverviewCommand(__PARAMETERS_LIST) {
  /* usage: overview */
  CheckSyntaxHelper(cmds, 0, 0, false, 'overview');
  return PackArrayMsg(holder->Overview());
}

std::string NumItemsCommand(__PARAMETERS_LIST) {
  /* usage: total */
  CheckSyntaxHelper(cmds, 0, 0, false, 'total');
  return PackIntReply(holder->NumItems());
}

std::string PingCommand(__PARAMETERS_LIST) {
  /* usage: ping */
  CheckSyntaxHelper(cmds, 0, 0, false, 'ping');
  return kPONGMsg;
}

std::string EvictCommand(__PARAMETERS_LIST) {
  /* usage: evict number */
  CheckSyntaxHelper(cmds, 1, 0, false, 'evict');
  size_t n;
  const std::string &n_req_del = cmds.argv[1];
  if (!CanConvertToUInt64(n_req_del, n)) {
    return kInvalidIntegerMsg;
  }
  std::vector<std::string> ans = holder->KeyEviction(sEvictPolicy, n);
  if (sync && appendable) {
    CommandCache cmd;
    ans.insert(ans.begin(), "del");
    cmd.argc = ans.size();
    cmd.argv = ans;
    cmd.inited = true;
    appendable->Append(cmd);
  }
  return PackIntReply((int)ans.size() - 1);
}

std::string DelCommand(__PARAMETERS_LIST) {
  /* usage: del key1 key2 key3 ... */
  CheckSyntaxHelper(cmds, -1, 0, false, 'del'); /* multiple keys supported */
  size_t n = holder->Delete(std::vector<std::string>(cmds.argv.begin() + 1, cmds.argv.end()));
  AddIntoAppendableDirectly(cmds);
  return PackIntReply(n);
}

std::string ExistsCommand(__PARAMETERS_LIST) {
  /* usage: exists key1 key2 key3 ... */
  CheckSyntaxHelper(cmds, -1, 0, false, 'exists');
  auto keys = std::vector<std::string>(cmds.argv.begin() + 1, cmds.argv.end());
  int n = holder->KeyExists(keys);
  return PackIntReply(n);
}

std::string TypeCommand(__PARAMETERS_LIST) {
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

std::string ExpireCommand(__PARAMETERS_LIST) {
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
    if (sync && appendable) {
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

std::string ExpireAtCommand(__PARAMETERS_LIST) {
  /* usage: expireat key unix_sec */
  CheckSyntaxHelper(cmds, 1, 1, false, 'expireat');
  const std::string &key = cmds.argv[1];
  int64_t unix_sec;
  if (CanConvertToInt64(cmds.argv[2], unix_sec)) {
    /* leave to expire command handle it */
    int64_t now = GetCurrentSec();
    int64_t interval = std::max(unix_sec - now, 0l);  /* seconds */
    const_cast<CommandCache &>(cmds).argv[2] = std::to_string(interval); /* force modification to const */
    return ExpireCommand(loop, holder, appendable, cmds, sync, sess, params);
  }
  return kInvalidIntegerMsg;
}

std::string TTLCommand(__PARAMETERS_LIST) {
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

std::string SetCommand(__PARAMETERS_LIST) {
  /* usage: set key value */
  CheckSyntaxHelper(cmds, 1, 1, false, 'set');
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
    AddIntoAppendableDirectly(cmds);
    return kOkMsg;
  }
  return kNotOkMsg;
}

std::string GetCommand(__PARAMETERS_LIST) {
  /* usage: get key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'get');
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
  IfKeyNotFoundReturn(errcode);
  IfWrongTypeReturn(errcode);

  return kNilMsg;
}

#define IfInt64OverflowThenReturn(errcode) \
  do {                                     \
    if (errcode == kOverflowCode) {        \
      return kInt64OverflowMsg;            \
    }                                      \
  } while (0)

#define IncrDecrCommonHelper(cmds, operation, appendable, sync) \
  do {                                                          \
    const std::string &key = cmds.argv[1];                      \
    int errcode = 0;                                            \
    int64_t ans = holder->operation(key, errcode);              \
    if (errcode == kOkCode) {                                   \
      AddIntoAppendableDirectly(cmds);                          \
      return PackIntReply(ans);                                 \
    }                                                           \
    IfWrongTypeReturn(errcode);                                 \
    IfInt64OverflowThenReturn(errcode);                         \
    return kNotOkMsg;                                           \
  } while (0)

#define IncrDecrCommonHelper2(cmds, operation, appendable, sync) \
  const std::string &key = cmds.argv[1];                         \
  const std::string &num_str = cmds.argv[2];                     \
  int64_t num;                                                   \
  if (!CanConvertToInt64(num_str, num)) {                        \
    return kInvalidIntegerMsg;                                   \
  }                                                              \
  if (num < 0) {                                                 \
    /* ensure num >= 0 && num <= INT64_MAX */                    \
    return kInvalidIntegerMsg;                                   \
  }                                                              \
  int errcode = 0;                                               \
  int64_t ans = holder->operation(key, num, errcode);            \
  if (errcode == kOkCode) {                                      \
    AddIntoAppendableDirectly(cmds);                             \
    return PackIntReply(ans);                                    \
  }                                                              \
  IfWrongTypeReturn(errcode);                                    \
  IfInt64OverflowThenReturn(errcode);                            \
  return kNotOkMsg;

std::string IncrCommand(__PARAMETERS_LIST) {
  /* usage: incr key */
  /* This operation is limited to 64 bit signed integers. */
  CheckSyntaxHelper(cmds, 1, 0, false, 'incr');
  IncrDecrCommonHelper(cmds, IncrInt, appendable, sync);
}

std::string DecrCommand(__PARAMETERS_LIST) {
  /* usage: decr key */
  /* This operation is limited to 64 bit signed integers. */
  CheckSyntaxHelper(cmds, 1, 0, false, 'decr');
  IncrDecrCommonHelper(cmds, DecrInt, appendable, sync);
}

std::string IncrByCommand(__PARAMETERS_LIST) {
  /* incrby key value */
  CheckSyntaxHelper(cmds, 1, 1, false, 'incrby');
  IncrDecrCommonHelper2(cmds, IncrIntBy, appendable, sync)
}

std::string DecrByCommand(__PARAMETERS_LIST) {
  /* decrby key value */
  CheckSyntaxHelper(cmds, 1, 1, false, 'decrby');
  IncrDecrCommonHelper2(cmds, DecrIntBy, appendable, sync)
}

#undef IncrDecrCommonHelper
#undef IfInt64OverflowThenReturn

std::string StrlenCommand(__PARAMETERS_LIST) {
  /* usage: strlen key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'strlen');
  const std::string &key = cmds.argv[1];
  int errcode = 0;
  size_t len = holder->StrLen(key, errcode);
  if (errcode == kOkCode) {
    return PackIntReply(len);
  }
  IfWrongTypeReturn(errcode);
  return PackIntReply(0);
}

std::string AppendCommand(__PARAMETERS_LIST) {
  /* usage: append key value */
  CheckSyntaxHelper(cmds, 1, 1, false, 'append');
  const std::string &key = cmds.argv[1];
  const std::string &value = cmds.argv[2];
  int errcode;
  size_t after_len = holder->Append(key, value, errcode);
  if (errcode == kOkCode) {
    AddIntoAppendableDirectly(cmds);
    return PackIntReply(after_len);
  }
  IfKeyNotFoundReturn(errcode);
  IfWrongTypeReturn(errcode);
  return kNotOkMsg;
}

std::string GetRangeCommand(__PARAMETERS_LIST) {
  /* usage: getrange key begin end */
  return kNotSupportedYetMsg;
}

std::string SetRangeCommand(__PARAMETERS_LIST) {
  return kNotSupportedYetMsg;
}

std::string LLenCommand(__PARAMETERS_LIST) {
  /* usage: llen key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'llen');
  const std::string &key = cmds.argv[1];
  int errcode;
  size_t list_len = holder->ListLen(key, errcode);
  if (errcode == kOkCode) {
    return PackIntReply(list_len);
  }
  IfWrongTypeReturn(errcode);
  return PackIntReply(0);
}

#define ListPopCommandCommon(cmds, operation, appendable, sync) \
  do {                                                          \
    const std::string &key = cmds.argv[1];                      \
    int errcode;                                                \
    auto popped = holder->operation(key, errcode);              \
    if (errcode == kOkCode) {                                   \
      if (!popped.Empty()) {                                    \
        AddIntoAppendableDirectly(cmds);                        \
        return PackStringValueReply(popped.ToStdString());      \
      }                                                         \
      return kNilMsg;                                           \
    } else if (errcode == kWrongTypeCode) {                     \
      return kWrongTypeMsg;                                     \
    }                                                           \
  } while (0)

std::string LPopCommand(__PARAMETERS_LIST) {
  /* usage: lpop key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'lpop');
  ListPopCommandCommon(cmds, LeftPop, appendable, sync);
  return kNilMsg;
}

std::string RPopCommand(__PARAMETERS_LIST) {
  /* usage: rpop key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'rpop');
  ListPopCommandCommon(cmds, RightPop, appendable, sync);
  return kNilMsg;
}

#undef ListPopCommandCommon

#define ListPushCommandCommon(cmds, operation, appendable, sync)               \
  do {                                                                         \
    const std::string &key = cmds.argv[1];                                     \
    const std::vector<std::string> &args = cmds.argv;                          \
    int errcode;                                                               \
    size_t list_len = holder->operation(                                       \
        key, std::vector<std::string>(args.begin() + 2, args.end()), errcode); \
    if (errcode == kOkCode) {                                                  \
      AddIntoAppendableDirectly(cmds);                                         \
      return PackIntReply(list_len);                                           \
    } else if (errcode == kWrongTypeCode) {                                    \
      return kWrongTypeMsg;                                                    \
    }                                                                          \
  } while (0)

std::string LPushCommand(__PARAMETERS_LIST) {
  /* usage: lpush key value1 value2 ... */
  CheckSyntaxHelper(cmds, 1, -1, false, 'lpush');
  ListPushCommandCommon(cmds, LeftPush, appendable, sync);
  return kNotOkMsg;
}

std::string RPushCommand(__PARAMETERS_LIST) {
  /* usage: rpush key value1 value2 ... */
  CheckSyntaxHelper(cmds, 1, -1, false, 'rpush');
  ListPushCommandCommon(cmds, RightPush, appendable, sync);
  return kNotOkMsg;
}

#undef ListPushCommandCommon

std::string LRangeCommand(__PARAMETERS_LIST) {
  /* usage: lrange key begin end */
  CheckSyntaxHelper(cmds, 1, 2, false, 'lrange');
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
      auto ret =  PackArrayMsg(ranges);
      return ret;
    }
  } else {
    IfWrongTypeReturn(errcode);
  }
  return kArrayEmptyMsg;
}

std::string LInsertCommand(__PARAMETERS_LIST) {
  /* TODO */
  return kNotSupportedYetMsg;
}

std::string LRemCommand(__PARAMETERS_LIST) {
  /* TODO */
  return kNotSupportedYetMsg;
}

std::string LSetCommand(__PARAMETERS_LIST) {
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
    AddIntoAppendableDirectly(cmds);
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

std::string LIndexCommand(__PARAMETERS_LIST) {
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
  IfWrongTypeReturn(errcode);
  /* if lindex encounters out of range, or key not found we can simply return nil */
  return kNilMsg;
}

std::string HSetCommand(__PARAMETERS_LIST) {
  /* usage: hset key field1 value1 field2 value2 ... */
  CheckSyntaxHelper(cmds, 1, -1, true, 'hset');
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
  int count = holder->HashUpdateKV(Key(key), fields, values, errcode);
  if (errcode == kOkCode && count != 0) {
    AddIntoAppendableDirectly(cmds);
    return kOkMsg;
  }
  IfWrongTypeReturn(errcode);
  return kNotOkMsg;
}

std::string HGetCommand(__PARAMETERS_LIST) {
  /* usage: hget key field1 field2 ...*/
  CheckSyntaxHelper(cmds, 1, -1, false, 'hget');
  const std::string &key = cmds.argv[1];
  int errcode;
  Key k = Key(key);
  if (cmds.argv.size() == 3) {  /* only get one field */
    HEntryVal val = holder->HashGetValue(k, HEntryKey(cmds.argv[2]), errcode);
    if (errcode == kOkCode) {
      return PackStringValueReply(val);
    }
    IfKeyNotFoundReturn(errcode);
    IfWrongTypeReturn(errcode);
  } else {  /* get multiple fields */
    std::vector<std::string> fields(cmds.argv.begin() + 2, cmds.argv.end());
    std::vector<HEntryVal> values = holder->HashGetValue(k, fields, errcode);
    /* pack into array and return */
    if (!values.empty()) {
      return PackArrayMsg(values);
    }
    /* return query result of every key */
    return PackEmptyArrayMsg(fields.size());
  }
  return kArrayEmptyMsg;
}

std::string HDelCommand(__PARAMETERS_LIST) {
  /* usage: hdel key field1 field2 ...*/
  CheckSyntaxHelper(cmds, 1, -1, false, 'hdel');
  const std::string &key = cmds.argv[1];
  int errcode;
  /* get all deleting fields */
  std::vector<std::string> fields(cmds.argv.begin() + 2, cmds.argv.end());
  size_t n_deleted = holder->HashDelField(Key(key), fields, errcode);
  IfWrongTypeReturn(errcode);
  AddIntoAppendableDirectly(cmds);
  return PackIntReply(n_deleted);
}

std::string HExistsCommand(__PARAMETERS_LIST) {
  /* usage: hexists key field */
  CheckSyntaxHelper(cmds, 1, 1, false, 'hexists');
  const std::string &key = cmds.argv[1];
  const std::string &field = cmds.argv[2];
  int errcode;
  auto ans = holder->HashExistField(key, field, errcode);
  IfWrongTypeReturn(errcode);
  return PackBoolReply(ans);
}

#define HKeysValsEntriesCommon(operation)      \
  const std::string &key = cmds.argv[1];       \
  int errcode;                                 \
  auto keys = holder->operation(key, errcode); \
  if (errcode == kWrongTypeCode) {             \
    return kWrongTypeMsg;                      \
  }                                            \
  if (!keys.empty()) {                         \
    return PackArrayMsg(keys);                 \
  }                                            \
  return kArrayEmptyMsg;

std::string HGetAllCommand(__PARAMETERS_LIST) {
  /* usage: hgetall key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'hgetall');
  HKeysValsEntriesCommon(HashGetAllEntries)
}

std::string HKeysCommand(__PARAMETERS_LIST) {
  /* usage: hkeys key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'hkeys');
  HKeysValsEntriesCommon(HashGetAllFields)
}

std::string HValsCommand(__PARAMETERS_LIST) {
  /* usage: hvals key*/
  CheckSyntaxHelper(cmds, 1, 0, false, 'hvals');
  HKeysValsEntriesCommon(HashGetAllValues)
}

#undef HKeysValsEntriesCommon

std::string HLenCommand(__PARAMETERS_LIST) {
  /* usage: hlen key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'hlen');
  const std::string &key = cmds.argv[1];
  int errcode;
  size_t len = holder->HashLen(key, errcode);
  IfWrongTypeReturn(errcode);
  return PackIntReply(len);
}

std::string SAddCommand(__PARAMETERS_LIST) {
  /* usage: sadd key member1 member2 ... */
  CheckSyntaxHelper(cmds, 1, -1, false, 'sadd');
  const std::string &key = cmds.argv[1];
  int errcode;
  /* fixme: extra memory cost */
  std::vector<std::string> members(cmds.argv.begin() + 2, cmds.argv.end()); 
  int n_added = holder->SetAddItem(key, members, errcode);
  IfWrongTypeReturn(errcode);
  IfFailReturn(errcode, PackIntReply(0));
  AddIntoAppendableDirectly(cmds);
  return PackIntReply(n_added);
}

std::string SIsMemberCommand(__PARAMETERS_LIST) {
  /* usage: sismember key member */
  CheckSyntaxHelper(cmds, 1, 1, false, 'sismember');
  const std::string &key = cmds.argv[1];
  const std::string &member = cmds.argv[2];
  int errcode;
  auto ret = holder->SetIsMember(key, member, errcode);
  IfWrongTypeReturn(errcode);
  return PackBoolReply(ret);
}

std::string SMIsMemberCommand(__PARAMETERS_LIST) {
  /* usage: smismember key member1 member2 ... */
  CheckSyntaxHelper(cmds, 1, -1, false, 'smismember');
  const std::string &key = cmds.argv[1];
  std::vector<std::string> members(cmds.argv.begin() + 2, cmds.argv.end());
  int errcode;
  auto ret = holder->SetMIsMember(key, members, errcode);
  IfWrongTypeReturn(errcode);
  return PackIntArrayMsg(ret);
}

std::string SMembersCommand(__PARAMETERS_LIST) {
  /* usage: smembers key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'smembers');
  const std::string &key = cmds.argv[1];
  int errcode;
  auto ret = holder->SetGetMembers(key, errcode);
  IfWrongTypeReturn(errcode);
  return PackArrayMsg(ret);
}

std::string SRemCommand(__PARAMETERS_LIST) {
  /* usage: srem key member1 member2 ... */
  CheckSyntaxHelper(cmds, 1, -1, false, 'srem');
  const std::string &key = cmds.argv[1];
  std::vector<std::string> members(cmds.argv.begin() + 2, cmds.argv.end());
  int errcode;
  auto ret = holder->SetRemoveMembers(key, members, errcode);
  IfWrongTypeReturn(errcode);
  AddIntoAppendableDirectly(cmds);
  return PackIntReply(ret);
}

std::string SCardCommand(__PARAMETERS_LIST) {
  /* usage: scard key */
  CheckSyntaxHelper(cmds, 1, 0, false, 'scard');
  const std::string &key = cmds.argv[1];
  int errcode;
  auto ret = holder->SetGetMemberCount(key, errcode);
  IfWrongTypeReturn(errcode);
  return PackIntReply(ret);
}

std::string SPopCommand(__PARAMETERS_LIST) {
  return kNotSupportedYetMsg; 
}

std::string PackPublishMessage(const std::string& chan_name, const std::string& message) {
  std::stringstream ss;
  ss << "*3\r\n"
    << "$7\r\n"
    << "message\r\n"
    << '$' << chan_name.size() << kCRLF
    << chan_name << kCRLF
    << '$' << message.size() << kCRLF
    << message << kCRLF;

  return ss.str();
}

std::string PubSubPublishCommand(__PARAMETERS_LIST) {
  /* usage: publish channel message */
  assert(params->server != nullptr);
  CheckSyntaxHelper(cmds, 1, 1, false, 'publish');
  const std::string &channel = cmds.argv[1];
  const std::string& message = cmds.argv[2];
  if (params->server->HasSubscriptionChannel(channel)) { /* has corresponding channel */
    /* relay message to all sessions that subscribed to this channel */
    size_t count = 0;
    for (auto&& sess_ptr: params->server->GetSubscriptionSessions(channel)) {
      const std::string &relay_str = PackPublishMessage(channel, message);
      // FIXME BUG: session do not clean thoroughly, have remaining data in write buffer
      size_t nbytes = WriteFromBuf(sess_ptr->fd, relay_str.c_str(), relay_str.size());  /* write message directly to session client */
      if (nbytes == relay_str.size()) {
        count ++;
      }
    }
    return PackIntReply(count);
  }
  return kInt0Msg;
}

void PackSubscriptionResArrayIntoStream(std::stringstream& ss, const char* cmd_name, const std::string& chan_name, size_t num, bool fill_minus_1 = false) {
  /* pack format => *3\r\n${len(cmd_name)}\r\n{cmd_name}\r\n${len(chan_name)}\r\n{chan_name}\r\n:{num}\r\n */
  ss << "*3\r\n"
    << '$' << strlen(cmd_name) << kCRLF
    << cmd_name << kCRLF;
  if (!fill_minus_1) {
    ss << '$' << chan_name.size() << kCRLF
      << chan_name << kCRLF;
  } else {
    ss << "$-1" << kCRLF;
  }
  ss << ':' << num << kCRLF;
}

std::string PubSubSubscribeCommand(__PARAMETERS_LIST) {
  /* usage: subscribe chan1 chan2 ... */
  assert(params->server != nullptr);
  assert(sess != nullptr);
  CheckSyntaxHelper(cmds, -1, -1, false, 'subscribe');
  /* use server instance to add subscription */
  const std::vector<std::string>& channels = cmds.argv;
  std::stringstream ss;
  for (size_t i = 1; i < channels.size(); ++i) {
    sess->subscribed_channels.insert(channels[i]); /* if exists, then override the old one */
    sess->SetPubSubMode();
    params->server->AddSessionToSubscription(channels[i], sess);
    PackSubscriptionResArrayIntoStream(ss, "subscribe", channels[i], sess->subscribed_channels.size());
  }
  return ss.str();
}

std::string PubSubUnsubscribeCommand(__PARAMETERS_LIST) {
  /* usage: unsubscribe chan1 chan2 ... */
  assert(params->server != nullptr);
  assert(sess != nullptr);
  CheckSyntaxHelper(cmds, -2, -1, false, 'unsubscribe');
  //  const std::vector<std::string>& channels = cmds.argv;
  std::vector<std::string> channels;
  if (cmds.argv.size() >= 2) {  /* unsubscribe the specified channels */
    channels.reserve(cmds.argv.size());
    channels.assign(cmds.argv.begin() + 1, cmds.argv.end());
  } else {   /* unsubscribe with no specific channel names, then unsubscribe all channels */
    channels.reserve(sess->subscribed_channels.size());
    channels.assign(sess->subscribed_channels.begin(), sess->subscribed_channels.end());
  }
  std::stringstream ss;
  if (channels.empty()) {
    /* no channels need to unsubscribe */
    PackSubscriptionResArrayIntoStream(ss, "unsubscribe", "", 0, true);
  } else {
    for (auto&& ch : channels) {
      sess->subscribed_channels.erase(ch);
      sess->UnsetPubSubMode();
      params->server->RemoveSessionFromSubscription(ch, sess);
      PackSubscriptionResArrayIntoStream(ss,"unsubscribe", ch, sess->subscribed_channels.size(), false);
    }
  }
  return ss.str();
}

