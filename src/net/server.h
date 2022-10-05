#ifndef __SERVER_H__
#define __SERVER_H__

#include <string>
#include <unordered_map>
#include <list>
#include <csignal>

#include "addr.h"
#include "net.h"
#include "commands.h"
#include "../config.h"

constexpr int NET_READ_BUF_SIZE = 1024 * 64;

class Engine; /* in commands.h */

enum ErrType {
  WRONGREQ = 0, /* can not parse request */
  WRONGTYPE = 1,/* wrong type for operator */
  ERROR = 2       /* others */
};

class InitIgnoreSigpipe {
public:
  /* ignore signal SIGPIPE */
  InitIgnoreSigpipe() {
    ::signal(SIGPIPE, SIG_IGN);
  }
};

// FIXME: this is a temporary plan, may be we should not ignore SIGCHLD
class InitIgnoreSigchild {
public:
  InitIgnoreSigchild() {
    ::signal(SIGCHLD, SIG_IGN);
  }
};

/* global InitIgnoreSigpipe instance to ignore SIGPIPE signal */
static InitIgnoreSigpipe sIgnoreSIGPIPEIniter;
static InitIgnoreSigchild sIgnoreSIGCHLDIniter;

class Server {
public:
  Server(EventLoop* loop, Engine* engine, Config *config, const std::string& ip, uint16_t port);

  ~Server();

  /**
   * Add session into subscription_sessions_.
   * @param chan_name The name of the subscription channel.
   * @param sess The session to be added.
   * @return
   */
  bool AddSessionToSubscription(const std::string& chan_name, Session* sess);

  /**
   * Remove the session from subscription
   * @param chan_name The name of the subscription channel.
   * @param sess The session to be removed.
   */
  void RemoveSessionFromSubscription(const std::string& chan_name, Session* sess);

  bool HasSubscriptionChannel(const std::string& chan_name);

  std::list<SessionPtr>& GetSubscriptionSessions(const std::string& chan_name) {
    return subscription_sessions_[chan_name];
  }

  void StopServeSockets();

private:
  void InitListenSession();

  void FreeListenSession();

  void FreeClientSessions();

  void AcceptProc(Session* session, bool&);

  void ReadProc(Session* session, bool&);

  void WriteProc(Session* session, bool&);

  void AuxiliaryReadProcParseErrorHandling(Session *session);

  void FillErrorMsg(Buffer& buffer, ErrType errtype, const char* msg) const;

private:
  EventLoop *loop_ = nullptr;
  Engine* engine_ = nullptr;
  Config* config_ = nullptr;
  Ipv4Addr addr_;
  int listen_fd_;
  /* all connected sessions */
  std::unordered_map<std::string, SessionPtr> sessions_;
  /* all the connected sessions that have subscribed channels */
  std::unordered_map<std::string, std::list<SessionPtr>> subscription_sessions_;
  static int next_session_id_;
  Session *listen_session_ = nullptr;
};

/**
 * The optional parameters wrapper for command handler function.
 * TODO: If more parameters should be passed to command handler function, add them in `struct OptionalHandlerParams` below
 */
struct OptionalHandlerParams {
  Server* server;
};

static OptionalHandlerParams sOptionalHandlerParamsObj; /* global */


#endif // __SERVER_H__