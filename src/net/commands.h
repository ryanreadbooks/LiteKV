#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "../core.h"
#include "../persistence.h"
#include "buffer.h"
#include "net.h"

static const std::array<const char *, 3> kErrStrTable{"WRONGREQ",
                                                      "WRONGTYPE",
                                                      "ERROR"};
static const char kErrPrefix = '-';
static const char kStrMsgPrefix = '+';
static const char kIntPrefix = ':';
static const char kStrValPrefix = '$';
static const char kArrayPrefix = '*';
static const char *kCRLF = "\r\n";
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
#define kInt64OverflowMsg "-ERROR integer overflow"
#define kNotSupportedYetMsg "-ERROR command not supported yet"

enum ErrType {
  WRONGREQ = 0, /* can not parse request */
  WRONGTYPE = 1,/* wrong type for operator */
  ERROR = 2       /* others */
};

class AppendableFile;

typedef std::string (*CommandHandler)(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

class Engine {
public:
  explicit Engine(KVContainer *container) : container_(container) {};

  ~Engine() {};

  std::string HandleCommand(EventLoop *loop, const CommandCache &cmds, bool flag = true);

  void HandleCommand(EventLoop *loop, const CommandCache &cmds, Buffer &out_buf);

  static bool OpCodeValid(const std::string &opcode);

  bool RestoreFromAppendableFile(EventLoop *loop, AppendableFile *history);

private:
  KVContainer *container_;  /* not own */
  AppendableFile *appending_; /* not own */
  static std::unordered_map<std::string, CommandHandler> sOpCommandMap;
};

/* generic command */
std::string PingCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string DelCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string ExistsCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string TypeCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string ExpireCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string ExpireAtCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string TTLCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

/* int or string command */
std::string SetCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string GetCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

/* int command */
std::string IncrCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string DecrCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string IncrByCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string DecrByCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

/* string command */
std::string StrlenCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string AppendCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string GetRangeCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string SetRangeCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

/* list command */
std::string LLenCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string LPopCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string LPushCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string RPopCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string RPushCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string LRangeCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string LInsertCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string LRemCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string LSetCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string LIndexCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

/* hash command */
std::string HSetCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string HGetCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string HDelCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string HExistsCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string HGetAllCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string HKeysCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string HValsCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

std::string HLenCommand(EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool);

# endif // __ENGINE_H__