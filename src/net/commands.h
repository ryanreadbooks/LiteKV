#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "../core.h"
#include "buffer.h"
#include "net.h"

static const std::array<const char*, 3> kErrStrTable{"WRONGREQ",
                                                     "WRONGTYPE",
                                                     "ERROR"};
static const char kErrPrefix = '-';
static const char kStrMsgPrefix = '+';
static const char kIntPrefix = ':';
static const char kStrValPrefix = '$';
static const char kArrayPrefix = '*';
static const char* kCRLF = "\r\n";
#define kInt1Msg ":1\r\n"
#define kInt0Msg ":0\r\n"
#define kNilMsg "$-1\r\n";
#define kOkMsg "+OK\r\n"
#define kPONGMsg "+PONG\r\n";
#define kArrayEmptyMsg "*0\r\n"
#define kNotOkMsg "-ERROR failed\r\n"
#define kNoSuchKeyMsg "-ERROR no such key"
#define kWrongTypeMsg "-WRONGTYPE operation to a key holding wrong type of value\r\n"
#define kOutOfRangeMsg "-ERROR index out of range\r\n"
#define kInvalidOpCodeMsg "-ERROR unsupported command\r\n"
#define kInvalidIntegerMsg "-ERROR index or value is not an integer"
#define kNotSupportedYetMsg "-ERROR command not supported yet"

enum ErrType {
  WRONGREQ = 0, /* can not parse request */
  WRONGTYPE = 1,/* wrong type for operator */
  ERROR = 2       /* others */
};

typedef std::string (*CommandHandler)(KVContainer *, const CommandCache &);

class Engine {
public:
  explicit Engine(KVContainer *container) : container_(container) {};

  ~Engine() {};

  std::string HandleCommand(const CommandCache &cmds);

  void HandleCommand(const CommandCache &cmds, Buffer &out_buf);

  static bool OpCodeValid(const std::string &opcode);

private:
  KVContainer *container_;
  static std::unordered_map<std::string, CommandHandler> sOpCommandMap;
};
/* generic command */
std::string PingCommand(KVContainer *, const CommandCache&);
std::string DelCommand(KVContainer *, const CommandCache&);
std::string ExistsCommand(KVContainer *, const CommandCache&);
std::string TypeCommand(KVContainer *, const CommandCache&);
/* int or string command */
std::string SetCommand(KVContainer *, const CommandCache&);
std::string GetCommand(KVContainer *, const CommandCache&);
/* int command */
std::string IncrCommand(KVContainer *, const CommandCache&);
std::string DecrCommand(KVContainer *, const CommandCache&);
std::string IncrByCommand(KVContainer *, const CommandCache&);
std::string DecrByCommand(KVContainer *, const CommandCache&);
/* string command */
std::string StrlenCommand(KVContainer *, const CommandCache&);
std::string AppendCommand(KVContainer *, const CommandCache&);
std::string GetRangeCommand(KVContainer *, const CommandCache&);
std::string SetRangeCommand(KVContainer *, const CommandCache&);
/* list command */
std::string LLenCommand(KVContainer *, const CommandCache&);
std::string LPopCommand(KVContainer *, const CommandCache&);
std::string LPushCommand(KVContainer *, const CommandCache&);
std::string RPopCommand(KVContainer *, const CommandCache&);
std::string RPushCommand(KVContainer *, const CommandCache&);
std::string LRangeCommand(KVContainer *, const CommandCache&);
std::string LInsertCommand(KVContainer *, const CommandCache&);
std::string LRemCommand(KVContainer *, const CommandCache&);
std::string LSetCommand(KVContainer *, const CommandCache&);
std::string LIndexCommand(KVContainer *holder, const CommandCache &cmds);
/* hash command */
std::string HSetCommand(KVContainer *holder, const CommandCache &cmds);
std::string HGetCommand(KVContainer *holder, const CommandCache &cmds);
std::string HDelCommand(KVContainer *holder, const CommandCache &cmds);
std::string HExistsCommand(KVContainer *holder, const CommandCache &cmds);
std::string HGetAllCommand(KVContainer *holder, const CommandCache &cmds);
std::string HKeysCommand(KVContainer *holder, const CommandCache &cmds);
std::string HValsCommand(KVContainer *holder, const CommandCache &cmds);
std::string HLenCommand(KVContainer *holder, const CommandCache &cmds);

# endif // __ENGINE_H__