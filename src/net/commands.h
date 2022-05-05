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
#define kIntMinus2Msg ":-2\r\n"
#define kIntMinus1Msg ":-1\r\n"
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

typedef std::string (*CommandHandler)(EventLoop*, KVContainer *, const CommandCache &);

class Engine {
public:
  explicit Engine(KVContainer *container) : container_(container) {};

  ~Engine() {};

  std::string HandleCommand(EventLoop* loop, const CommandCache &cmds);

  void HandleCommand(EventLoop* loop, const CommandCache &cmds, Buffer &out_buf);

  static bool OpCodeValid(const std::string &opcode);

private:
  KVContainer *container_;
  static std::unordered_map<std::string, CommandHandler> sOpCommandMap;
};
/* generic command */
std::string PingCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string DelCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string ExistsCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string TypeCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string ExpireCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string TTLCommand(EventLoop*, KVContainer *, const CommandCache&);
/* int or string command */
std::string SetCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string GetCommand(EventLoop*, KVContainer *, const CommandCache&);
/* int command */
std::string IncrCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string DecrCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string IncrByCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string DecrByCommand(EventLoop*, KVContainer *, const CommandCache&);
/* string command */
std::string StrlenCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string AppendCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string GetRangeCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string SetRangeCommand(EventLoop*, KVContainer *, const CommandCache&);
/* list command */
std::string LLenCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string LPopCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string LPushCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string RPopCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string RPushCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string LRangeCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string LInsertCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string LRemCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string LSetCommand(EventLoop*, KVContainer *, const CommandCache&);
std::string LIndexCommand(EventLoop*, KVContainer *holder, const CommandCache &cmds);
/* hash command */
std::string HSetCommand(EventLoop*, KVContainer *holder, const CommandCache &cmds);
std::string HGetCommand(EventLoop*, KVContainer *holder, const CommandCache &cmds);
std::string HDelCommand(EventLoop*, KVContainer *holder, const CommandCache &cmds);
std::string HExistsCommand(EventLoop*, KVContainer *holder, const CommandCache &cmds);
std::string HGetAllCommand(EventLoop*, KVContainer *holder, const CommandCache &cmds);
std::string HKeysCommand(EventLoop*, KVContainer *holder, const CommandCache &cmds);
std::string HValsCommand(EventLoop*, KVContainer *holder, const CommandCache &cmds);
std::string HLenCommand(EventLoop*, KVContainer *holder, const CommandCache &cmds);

# endif // __ENGINE_H__