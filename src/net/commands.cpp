#include <cassert>
#include <algorithm>
#include <sstream>

#include "commands.h"

std::unordered_map<std::string, CommandHandler> Engine::sOpCommandMap = {
    /* generic */
    {"ping",      PingCommand},   /* ping-pong test */
    {"del",       DelCommand},    /* delete given keys */
    {"exists",    ExistsCommand}, /* check if given keys exist */
    {"type",      TypeCommand},   /* query object type */
    /* int or string */
    {"set",       SetCommand},    /* set given key to string or int */
    {"get",       GetCommand},    /* get value on given key */
    /* for int */
    {"incr",      IncrCommand},   /* TODO not supported, increase int value by 1 on given key */
    {"decr",      DecrCommand},   /* TODO not supported, decrease int value by 1 on given key */
    {"incrby",    IncrByCommand}, /* TODO not supported, increase int value by n on given key */
    {"decrby",    DecrByCommand}, /* TODO not supported, decrease int value by n on given key */
    /* for string */
    {"strlen",    StrlenCommand}, /* get the len of string on given key */
    {"append",    AppendCommand}, /* append value on given key */
    {"getrange",  GetRangeCommand}, /* TODO not supported, get range value on given key */
    {"setrange",  SetRangeCommand}, /* TODO not supported, set range value on given key */
    /* for list */
    {"llen",      LLenCommand},   /* get the length of list on given key */
    {"lpop",      LPopCommand},   /* left pop one value from list on given key */
    {"lpush",     LPushCommand},  /* left push values from list on given key */
    {"rpop",      RPopCommand},   /* right pop one value from list on given key */
    {"rpush",     RPushCommand},  /* right push values from list on given key */
    {"lrange",    LRangeCommand}, /* get value in range of list on given key */
    {"linsert",   LInsertCommand},/* TODO not supported, insert value into list on given key */
    {"lrem",      LRemCommand},   /* TODO not supported, remove element from list on given key */
    {"lsetindex", LSetCommand},   /* set element from list at index on given key  */
    {"lindex",    LIndexCommand}  /* get element in list at index on given key */
};

// FIXME optimize syntax check
static bool CheckSyntax(const CommandCache &cmd, int8_t n_key_required, int8_t n_operands_required) {
  if (n_key_required == 0) {
    return cmd.argv.size() == 1;
  }
  if (n_key_required == 1) {
    if (n_operands_required == -1) {
      return cmd.argv.size() >= 3;
    } else {
      return cmd.argv.size() == (size_t)(n_operands_required + 2);
    }
//    if (n_operands_required == 0) {
//      return cmd.argv.size() == 2;
//    } else if (n_operands_required == 1) {
//      return cmd.argv.size() == 3;
//    } else if (n_operands_required == -1) {
//      return cmd.argv.size() >= 3;
//    } else if (n_operands_required == 2) {
//      return cmd.argv.size() == 4;
//    } else {
//      return false;
//    }
  }
  if (n_key_required == -1) {
    return cmd.argv.size() >= 2;
  }
  return false;
}

#define CheckSyntaxHelper(cmd, n_key_required, n_operands_required, errmsg) \
if(!CheckSyntax(cmd, n_key_required, n_operands_required)) { \
  return PackErrMsg("ERROR", errmsg);\
}

static std::string PackIntReply(int value) {
  return kIntPrefix + std::to_string(value) + kCRLF;
}

static std::string PackStringValueReply(const std::string &value) {
  size_t len = value.size();
  return kStrValPrefix + std::to_string(len) + kCRLF + value + kCRLF;
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
    arr_ss << '$' << value.Length() << kCRLF
           << value << kCRLF;
  }
  return arr_ss.str();
}

std::string Engine::HandleCommand(const CommandCache &cmds) {
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
  return sOpCommandMap[opcode](container_, cmds);
}

void Engine::HandleCommand(const CommandCache &cmds, Buffer &out_buf) {
  std::string response = HandleCommand(cmds);
  out_buf.Append(response);
}

bool Engine::OpCodeValid(const std::string &opcode) {
  return sOpCommandMap.find(opcode) != sOpCommandMap.end();
}

std::string PingCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: ping */
  CheckSyntaxHelper(cmds, 0, 0, "")
  return kPONGMsg;
}

std::string DelCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: del key1 key2 key3 ... */
  CheckSyntaxHelper(cmds, -1, 0, "incorrect number of arguments for 'del' command")  /* multiple keys supported */
  size_t n = holder->Delete(std::vector<std::string>(cmds.argv.begin() + 1, cmds.argv.end()));
  return PackIntReply(n);
}

std::string ExistsCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: exists key1 key2 key3 ... */
  CheckSyntaxHelper(cmds, -1, 0, "incorrect number of arguments for 'exists' command")
  auto keys = std::vector<std::string>(cmds.argv.begin() + 1, cmds.argv.end());
  int n = holder->KeyExists(keys);
  return PackIntReply(n);
//  return kKeyNoExistsMsg;
}

std::string TypeCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: type key */
  CheckSyntaxHelper(cmds, 1, 0, "incorrect number of arguments for 'type' command");
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

std::string SetCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: set key value */
  CheckSyntaxHelper(cmds, 1, 1, "incorrect number of arguments for 'set' command")
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
  if (res) return kOkMsg;
  return kNotOkMsg;
}

std::string GetCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: get key */
  CheckSyntaxHelper(cmds, 1, 0, "incorrect number of arguments for 'get' command")
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
  } else if (errcode == kKeyNotFoundCode) {
    return kNilMsg;
  } else if (errcode == kWrongTypeCode) {
    return kWrongTypeMsg;
  }
  return kNilMsg;
}

std::string IncrCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: incr key */
  return kNotOkMsg;
}

std::string DecrCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: decr key */
  return kNotOkMsg;
}

std::string IncrByCommand(KVContainer *holder, const CommandCache &) {
  /* usage: incrby key value */
  return kNotOkMsg;
}

std::string DecrByCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: decrby key value */
  return kNotOkMsg;
}

std::string StrlenCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: strlen key */
  CheckSyntaxHelper(cmds, 1, 0, "incorrect number of arguments for 'strlen' command")
  const std::string &key = cmds.argv[1];
  int errcode = 0;
  size_t len = holder->StrLen(key, errcode);
  if (errcode == kOkCode) {
    return PackIntReply(len);
  } else if (errcode == kWrongTypeCode) {
    return kWrongTypeMsg;
  }
  return PackIntReply(0);
}

std::string AppendCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: append key value */
  CheckSyntaxHelper(cmds, 1, 1, "incorrect number of arguments for 'append' command")
  const std::string &key = cmds.argv[1];
  const std::string &value = cmds.argv[2];
  int errcode;
  size_t after_len = holder->Append(key, value, errcode);
  if (errcode == kOkCode) {
    return PackIntReply(after_len);
  } else if (errcode == kWrongTypeCode) {
    return kWrongTypeMsg;
  }
  return kNotOkMsg;
}

std::string GetRangeCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: getrange key begin end */
  return kNotOkMsg;
}

std::string SetRangeCommand(KVContainer *holder, const CommandCache &cmds) {
  return kNotOkMsg;
}

std::string LLenCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: llen key */
  CheckSyntaxHelper(cmds, 1, 0, "incorrect number of arguments for 'llen' command");
  const std::string& key = cmds.argv[1];
  int errcode;
  size_t list_len = holder->ListLen(key, errcode);
  if (errcode == kOkCode) {
    return PackIntReply(list_len);
  } else if (errcode == kWrongTypeCode) {
    return kWrongTypeMsg;
  }
  return PackIntReply(0);
}

#define ListPopCommandCommon(operator) \
  const std::string &key = cmds.argv[1];  \
  int errcode;  \
  auto popped = holder->operator(key, errcode);  \
  if (errcode == kOkCode) { \
    if (!popped.Empty()) {  \
      return PackStringValueReply(popped.ToStdString());  \
    } \
    return kNilMsg; \
  } else if (errcode == kWrongTypeCode) { \
    return kWrongTypeMsg; \
  }

std::string LPopCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: lpop key */
  CheckSyntaxHelper(cmds, 1, 0, "incorrect number of arguments for 'lpop' command")
  ListPopCommandCommon(LeftPop)
  return kNilMsg;
}

std::string RPopCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: rpop key */
  CheckSyntaxHelper(cmds, 1, 0, "incorrect number of arguments for 'rpop' command")
  ListPopCommandCommon(RightPop)
  return kNilMsg;
}

#undef ListPopCommandCommon

#define ListPushCommandCommon(operator) \
  const std::string& key = cmds.argv[1];  \
  const std::vector<std::string>& args = cmds.argv; \
  int errcode;  \
  size_t list_len = holder->operator(key, std::vector<std::string>(args.begin() + 2, args.end()), errcode); \
  if (errcode == kOkCode) { \
    return PackIntReply(list_len);  \
  } else if (errcode == kWrongTypeCode) { \
    return kWrongTypeMsg; \
  }

std::string LPushCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: lpush key value1 value2 ... */
  CheckSyntaxHelper(cmds, 1, -1, "incorrect number of arguments for 'lpush' command")
  ListPushCommandCommon(LeftPush)
  return kNotOkMsg;
}

std::string RPushCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: rpush key value1 value2 ... */
  CheckSyntaxHelper(cmds, 1, -1, "incorrect number of arguments for 'rpush' command")
  ListPushCommandCommon(RightPush)
  return kNotOkMsg;
}

#undef ListPushCommandCommon

std::string LRangeCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: lrange key begin end */
  CheckSyntaxHelper(cmds, 1, 2, "incorrect number of arguments for 'lrange' command")
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
  } else if (errcode == kWrongTypeCode) {
    return kWrongTypeMsg;
  }
  return kArrayEmptyMsg;
}

std::string LInsertCommand(KVContainer *holder, const CommandCache &cmds) {
  /* TODO */
  return kNotOkMsg;
}

std::string LRemCommand(KVContainer *holder, const CommandCache &cmds) {
  /* TODO */
  return kNotOkMsg;
}

std::string LSetCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: lsetindex key index value */
  CheckSyntaxHelper(cmds, 1, 2, "incorrect number of arguments for 'lsetindex' command");
  const std::string &key = cmds.argv[1];
  const std::string& index = cmds.argv[2];
  int idx;
  if (!CanConvertToInt32(index, idx)) {
    return kInvalidIntegerMsg;
  }
  const std::string& value = cmds.argv[3];
  int errcode;
  bool ok = holder->ListSetItemAtIndex(key, idx, value, errcode);
  if (errcode == kOkCode) {
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

std::string LIndexCommand(KVContainer *holder, const CommandCache &cmds) {
  /* usage: lindex key index */
  CheckSyntaxHelper(cmds, 1, 1, "incorrect number of arguments for 'lindex' command");
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
  } else if (errcode == kWrongTypeCode) {
    return kWrongTypeMsg;
  }
  /* if lindex encounters out of range, or key not found we can simply return nil */
  return kNilMsg;
}
