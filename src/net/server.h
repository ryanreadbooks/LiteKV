#ifndef __SERVER_H__
#define __SERVER_H__

#include <string>
#include <unordered_map>
#include <signal.h>

#include "addr.h"
#include "net.h"
#include "commands.h"

constexpr int NET_READ_BUF_SIZE = 1024 * 64;

class InitIgnoreSigpipe {
public:
  /* ignore signal SIGPIPE */
  InitIgnoreSigpipe() {
    ::signal(SIGPIPE, SIG_IGN);
  }
};

class Server {
public:
  Server(EventLoop* loop, Engine* engine, const std::string& ip, uint16_t port);
  ~Server();

private:
  void InitListenSession();

  void FreeListenSession();

  void AcceptProc(Session* session, bool&);

  void ReadProc(Session* session, bool&);

  void WriteProc(Session* session, bool&);

  void AuxiliaryReadProcParseErrorHandling(Session *session);

  void FillErrorMsg(Buffer& buffer, ErrType errtype, const char* msg) const;

private:
  EventLoop *loop_ = nullptr;
  Engine* engine_ = nullptr;
  Ipv4Addr addr_;
  int listen_fd_;
  std::unordered_map<std::string, SessionPtr> sessions_;
  static int next_session_id_;
  Session *listen_session_ = nullptr;
};

#endif // __SERVER_H__