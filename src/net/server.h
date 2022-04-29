#ifndef __SERVER_H__
#define __SERVER_H__

#include <string>
#include <unordered_map>
#include <signal.h>

#include "addr.h"
#include "net.h"
#include "../engine.h"

enum ErrType {
  WRONGREQ = 0, /* can not parse request */
  WRONGTYPE = 1,/* wrong type for operator */
  ERROR = 2       /* others */
};

static const std::array<const char*, 3> kErrStrTable{"WRONGREQ","WRONGTYPE","ERROR"};
static const char* kCRLF = "\r\n";
static const char* kWrongReqMsg = "unidentified request";

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

  void AuxiliaryReadProcCleanup(Buffer& buffer, CommandCache& cache, size_t begin_idx, int nbytes);

  bool AuxiliaryReadProc(Buffer& buffer, CommandCache& cache, int nbytes);

  void AuxiliaryReadProcParseErrorHandling(Session *session);

  void FillErrorMsg(Buffer& buffer, ErrType errtype, const char* msg) const;

  void FillResponseMsg(const Buffer& buffer, const std::string& msg);

  void _DebugWriteProc(Session* session);

private:
  EventLoop *loop_ = nullptr;
  Ipv4Addr addr_;
  int listen_fd_;
  std::unordered_map<std::string, SessionPtr> sessions_;
  static int next_session_id_;
  Session *listen_session_ = nullptr;
  Engine* engine_ = nullptr;
};

#endif // __SERVER_H__