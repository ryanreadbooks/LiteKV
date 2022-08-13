#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <thread>
#include <atomic>

#include "../core.h"
#include "../persistence.h"
#include "../config.h"
#include "../mem.h"
#include "buffer.h"
#include "net.h"
#include "server.h"

static constexpr std::array<const char *, 3> kErrStrTable{"WRONGREQ",
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
#define kNilMsg "$-1\r\n"
#define kOkMsg "+OK\r\n"
#define kPONGMsg "+PONG\r\n"
#define kArrayEmptyMsg "*0\r\n"
#define kNotOkMsg "-ERROR failed\r\n"
#define kNoSuchKeyMsg "-ERROR no such key\r\n"
#define kWrongTypeMsg "-WRONGTYPE operation to a key holding wrong type of value\r\n"
#define kOutOfRangeMsg "-ERROR index out of range\r\n"
#define kInvalidOpCodeMsg "-ERROR unsupported command\r\n"
#define kInvalidIntegerMsg "-ERROR index or value is not an integer\r\n"
#define kInt64OverflowMsg "-ERROR integer overflow\r\n"
#define kNotSupportedYetMsg "-ERROR command not supported yet\r\n"

class Server;                 /* in server.h */
struct OptionalHandlerParams;  /* in server.h */
class AppendableFile;         /* in persistence.h */

/* the parameters list for `CommandHandler` function */
#define PARAMETERS_LIST EventLoop *, KVContainer *, AppendableFile *, const CommandCache &, bool, Session*, OptionalHandlerParams*

/* Not all CommandHandler function instance use all parameters */
typedef std::string (*CommandHandler)(PARAMETERS_LIST);

class Engine {
public:
  explicit Engine(KVContainer *container, Config *config);

  ~Engine();

  /**
   * Handle the command coming in.
   * @param loop The main event loop.
   * @param cmds The command to be handled.
   * @param sync Synchronize this command to file or not.
   * @return Reply string.
   */
  std::string HandleCommand(EventLoop *loop, CommandCache &cmds, bool sync = true, Session* sess = nullptr, OptionalHandlerParams* options = nullptr);

  /**
   * Check if operation code is valid.
   * @param opcode Given operation code
   * @return
   */
  static bool OpCodeValid(const std::string &opcode);

  bool RestoreFromAppendableFile(EventLoop *loop, AppendableFile *history);

private:
  bool IfNeedKeyEviction();

  void UpdateMemInfo();

private:
  KVContainer *container_ = nullptr;  /* not owned */
  AppendableFile *appending_ = nullptr; /* not owned */
  Config *config_ = nullptr;  /* not owned */
  static std::unordered_map<std::string, CommandHandler> sOpCommandMap;
  size_t cur_vm_size_;
  size_t cur_rss_size_;
  std::thread worker_;
  std::atomic<bool> stopped_;
};

/* generic command */
std::string OverviewCommand(PARAMETERS_LIST);

std::string NumItemsCommand(PARAMETERS_LIST);

std::string PingCommand(PARAMETERS_LIST);

std::string EvictCommand(PARAMETERS_LIST);

std::string DelCommand(PARAMETERS_LIST);

std::string ExistsCommand(PARAMETERS_LIST);

std::string TypeCommand(PARAMETERS_LIST);

std::string ExpireCommand(PARAMETERS_LIST);

std::string ExpireAtCommand(PARAMETERS_LIST);

std::string TTLCommand(PARAMETERS_LIST);

/* int or string command */
std::string SetCommand(PARAMETERS_LIST);

std::string GetCommand(PARAMETERS_LIST);

/* int command */
std::string IncrCommand(PARAMETERS_LIST);

std::string DecrCommand(PARAMETERS_LIST);

std::string IncrByCommand(PARAMETERS_LIST);

std::string DecrByCommand(PARAMETERS_LIST);

/* string command */
std::string StrlenCommand(PARAMETERS_LIST);

std::string AppendCommand(PARAMETERS_LIST);

std::string GetRangeCommand(PARAMETERS_LIST);

std::string SetRangeCommand(PARAMETERS_LIST);

/* list command */
std::string LLenCommand(PARAMETERS_LIST);

std::string LPopCommand(PARAMETERS_LIST);

std::string LPushCommand(PARAMETERS_LIST);

std::string RPopCommand(PARAMETERS_LIST);

std::string RPushCommand(PARAMETERS_LIST);

std::string LRangeCommand(PARAMETERS_LIST);

std::string LInsertCommand(PARAMETERS_LIST);

std::string LRemCommand(PARAMETERS_LIST);

std::string LSetCommand(PARAMETERS_LIST);

std::string LIndexCommand(PARAMETERS_LIST);

/* hash command */
std::string HSetCommand(PARAMETERS_LIST);

std::string HGetCommand(PARAMETERS_LIST);

std::string HDelCommand(PARAMETERS_LIST);

std::string HExistsCommand(PARAMETERS_LIST);

std::string HGetAllCommand(PARAMETERS_LIST);

std::string HKeysCommand(PARAMETERS_LIST);

std::string HValsCommand(PARAMETERS_LIST);

std::string HLenCommand(PARAMETERS_LIST);

/* set commands */
std::string SAddCommand(PARAMETERS_LIST);

std::string SIsMemberCommand(PARAMETERS_LIST);

std::string SMIsMemberCommand(PARAMETERS_LIST);

std::string SMembersCommand(PARAMETERS_LIST);

std::string SRemCommand(PARAMETERS_LIST);

std::string SCardCommand(PARAMETERS_LIST);

std::string SPopCommand(PARAMETERS_LIST);

/*　pub/sub commands */
std::string PubSubPublishCommand(PARAMETERS_LIST);

std::string PubSubSubscribeCommand(PARAMETERS_LIST);

std::string PubSubUnsubscribeCommand(PARAMETERS_LIST);

#undef PARAMETERS_LIST

# endif // __ENGINE_H__