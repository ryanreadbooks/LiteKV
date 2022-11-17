#include <iostream>
#include <sstream>
#include <cstring>
#include <cerrno>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <cassert>
#include "server.h"
#include "utils.h"
#include "protocol.h"

using namespace std::placeholders;

int Server::next_session_id_ = 1;

Server::Server(EventLoop *loop, Engine *engine, Config *config,
               const std::string &ip, uint16_t port)
    : loop_(loop), engine_(engine), config_(config), addr_(ip, port) {
  if (loop == nullptr) {
    std::cerr << "No loop is specified for the server\n";
    exit(EXIT_FAILURE);
  }
  assert(config_ != nullptr);
  InitListenSession();
}

Server::~Server() {
  /* close all connections and release all sessions */
  FreeListenSession();
  FreeClientSessions();
}

bool Server::AddSessionToSubscription(const std::string& chan_name, Session *sess) {
  if (sess != nullptr && sessions_.find(sess->name) != sessions_.end()) {
    subscription_sessions_[chan_name].push_back(sessions_[sess->name]);
    return true;
  }
  return false;
}

void Server::RemoveSessionFromSubscription(const std::string& chan_name, Session* sess) {
  subscription_sessions_[chan_name].remove_if([&sess](const SessionPtr& sess_ptr) { return sess_ptr->name == sess->name; });
}

bool Server::HasSubscriptionChannel(const std::string &chan_name) {
  return subscription_sessions_.find(chan_name) != subscription_sessions_.end();
}

void Server::StopServeSockets() {
  loop_->Stop();  /* we also need to stop the event loop, this go first */
  FreeListenSession();
  FreeClientSessions();
}

void Server::InitListenSession() {
  /* init socket and address */
  if ((listen_fd_ = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    std::cerr << "Can not init listen socket: " << strerror(errno) << std::endl;
    exit(-1);
  }
  SetReuseAddress(listen_fd_);
  SetReusePort(listen_fd_);
  SetFdNonBlock(listen_fd_);
  if (bind(listen_fd_, addr_.GetAddr(), addr_.GetSockAddrLen()) == -1) {
    std::cerr << "Can not bind socket: " << strerror(errno) << std::endl;
    exit(-1);
  }
  /* attach into loop */
  listen_session_ = new(std::nothrow) Session(
      listen_fd_, EPOLLIN,
      std::bind(&Server::AcceptProc, this, _1, _2),
      nullptr,
      loop_, "listen_session");
  assert(listen_session_ != nullptr);
  /* attach listen fd into epoll */
  loop_->epoller->AttachSession(listen_session_);
  listen(listen_fd_, SOMAXCONN);
  listen_session_->watched = true;
}

void Server::FreeListenSession() {
  if (listen_session_) {
    loop_->epoller->DetachSession(listen_session_);
    delete listen_session_;
    listen_session_ = nullptr;
  }
}

void Server::FreeClientSessions() {
  /* free subscription_sessions_ and sessions_ */
  subscription_sessions_.clear();
  /* close all connected sessions */
  if (!sessions_.empty()) {
    for (auto&& session_pair : sessions_) {
      auto& sess = session_pair.second;
      loop_->epoller->DetachSession(sess.get());
      close(sess->fd);
    }
  }
  sessions_.clear();
}

void Server::AcceptProc(Session *session, bool &closed) {
  /* accept incoming connection(session) and process */
  Ipv4Addr addr;
  socklen_t socklen = addr.GetSockAddrLen();
  int remote_fd = accept(listen_fd_, addr.GetAddr(), &socklen);
  if (remote_fd != -1) {
    addr.SyncPort();
    // std::cout << "accepted connection from: " << addr.ToString() << std::endl;
    char buf[64];
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof buf, "*%s#%d", addr.ToString().c_str(), next_session_id_++);
    std::string sess_name = buf;
    /* create a new session for every accepted connection */
    Session *sess = new(std::nothrow) Session(
        remote_fd, EPOLLIN,
        std::bind(&Server::ReadProc, this, _1, _2),
        std::bind(&Server::WriteProc, this, _1, _2),
        loop_, sess_name);
    // std::cout << "accepted  session = " << sess << std::endl;
    if (sess != nullptr) {
      /* attach new session into epoll */
      SetFdNonBlock(remote_fd);
      int interval = config_->KeepAliveInterval();
      int idle = interval * 3;
      int cnt = config_->KeepAliveCnt();
      /* we use tcp keepalive to close broken socket connection */
      SetKeepAlive(remote_fd, idle, interval, cnt);
      if (loop_->epoller->AttachSession(sess)) {
        // std::cout << "fd=" << sess->fd << " added into eventloop watch\n";
        sess->watched = true;
        sessions_[sess_name] = std::shared_ptr<Session>(sess);
      }
    }
  }
}

void Server::AuxiliaryReadProcParseErrorHandling(Session *session) {
  if (!session) return;
  /* Fill write buffer with error msg and send them back to client */
  FillErrorMsg(session->write_buf, ErrType::WRONGREQ, "unidentified request\r\n");
  /* trigger write */
  session->SetWrite();
  loop_->epoller->ModifySession(session);
}

/* FIXME: Can not handle huge flow of request coming in */
void Server::ReadProc(Session *session, bool &closed) {
  // static int64_t n_total_bytes_recv = 0;
  // static int64_t n_response = 0;

  int fd = session->fd;
  Buffer &buffer = session->read_buf;
  CommandCache &cache = session->cache;
  char buf[NET_READ_BUF_SIZE];
  int nbytes = ReadToBuf(fd, buf, sizeof(buf));
  if (nbytes == 0) {
    /* close connection */
    session->watched = false;
    buffer.Reset();
    close(fd);
    /* remove sessions from subscription */
    if (!session->subscribed_channels.empty()) {
      for (auto it = session->subscribed_channels.begin(); it != session->subscribed_channels.end(); it++) {
        subscription_sessions_[*it].remove_if([=] (const SessionPtr& sess_ptr) { return sess_ptr->name == session->name; });
      }
    }
    sessions_.erase(session->name);
    // std::cout << "Client-" << session->fd << " exit, now close connection...\n";
    closed = true;
    return;
  }
  // std::cout << "Received bytes = " << nbytes << std::endl;
  // n_total_bytes_recv += nbytes;
  buffer.Append(buf, nbytes);

  bool err = false;
  while (TryParseFromBuffer(buffer, cache, err) && !err) {
    sOptionalHandlerParamsObj.server = this;
    std::string handle_result = engine_->HandleCommand(loop_, cache, true, session, &sOptionalHandlerParamsObj);
    session->write_buf.Append(handle_result);
    // n_response++;
    /* clear cache when one command is fully parsed */
    cache.Clear();
  }
  session->SetWrite();
  loop_->epoller->ModifySession(session);
  /* if err occurs, then we assume the command syntax is invalid */
  if (err) {
    AuxiliaryReadProcParseErrorHandling(session);
  }
}

void Server::FillErrorMsg(Buffer &buffer, ErrType errtype, const char *msg) const {
  const char *errtype_str = kErrStrTable[errtype];
  auto err_str = "-" + std::string(errtype_str) + ' ' + msg + kCRLF;
  buffer.Append(err_str);
}

void Server::WriteProc(Session *session, bool &closed) {
  int fd = session->fd;
  Buffer &buffer = session->write_buf;
  if (buffer.ReadableBytes() <= 0) {
    /* nothing to write */
    session->SetRead();
    loop_->epoller->ModifySession(session);
    return;
  }
  // std::cout << "Doing write process, write buffer is => " << buffer.ReadableAsString() << std::endl;
  /* ensure all data has been sent, then unregister EPOLLOUT to this fd */
  size_t readable_bytes = buffer.ReadableBytes(); /* the number of bytes ready to send */
  int nbytes = WriteFromBuf(fd, static_cast<const char *>(buffer.BeginRead()), readable_bytes);
  if ((size_t) nbytes > 0) {
    if ((size_t) nbytes == readable_bytes) {
      /* all bytes have been sent. no need to send in the recent future.*/
      session->SetRead();
      loop_->epoller->ModifySession(session);
      /* clear buffer */
      buffer.Reset();
    } else {
      /* consume nbytes in buffer */
      buffer.ReaderIdxForward(nbytes);
    }
  }
  if (nbytes == 0 && buffer.ReadableBytes() != 0) {
    session->watched = false;
    buffer.Reset();
    close(fd);
    // std::cout << "[Server::WriteProc] Client exit, now close connection from " << session->name << '\n';
    sessions_.erase(session->name);
    closed = true;
  }
}