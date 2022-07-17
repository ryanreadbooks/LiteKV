#ifndef __NET_H__
#define __NET_H__

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <sstream>
#include <vector>
#include <sys/epoll.h>

#include "buffer.h"
#include "time_event.h"


class Epoller;

struct EventLoop;
struct CommandCache;
struct Session;

typedef std::function<void(Session *, bool &)> ProcFuncType;
typedef struct epoll_event epoll_event;

struct CommandCache {
  bool inited = false;
  size_t argc = 0;
  std::vector<std::string> argv;

  void Clear() {
    inited = false;
    argc = 0;
    argv.clear();
  }

  friend std::ostream &operator<<(std::ostream &os, const CommandCache &cache) {
    for (size_t i = 0; i < cache.argv.size(); ++i) {
      if (i != cache.argv.size() - 1) {
        os << cache.argv[i] << " ";
      } else {
        os << cache.argv[i] << "\n";
      }
    }
    return os;
  }

  std::string ToString() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
  }

  std::string ToProtocolString() const {
    std::stringstream ss;
    ss << "*" << argc << "\r\n";
    for (const auto &item : argv) {
      if (item.empty()) {
        ss << "$0\r\n\r\n";
      } else {
        ss << "$" << item.size() << "\r\n"
            << item << "\r\n";
      }
    }
    return ss.str();
  }

  static CommandCache FromProtocolString(const std::string &protostr) {
    /* TODO construct from protocol string */
    CommandCache ret;
    return ret;
  }
};

#define SESSION_MODE_REGULAR (1u << 0u) /* session is in regular mode for read/write */
#define SESSION_MODE_PUBSUB (1u << 1u)  /* session is in pub/sub mode, the session is ok to be published messages */

struct Session {

  int fd; /* corresponding fd */
  /* our interested events */
  uint32_t mask;
  /* fired events */
  uint32_t events;

  ProcFuncType read_proc;  /* read handler */
  ProcFuncType write_proc; /* write handler */

  EventLoop *loop;

  Buffer read_buf; /* read buffer */
  Buffer write_buf;/* write buffer */

  CommandCache cache;
  std::string name; /* the name of this session */

  /* indicate session is being watched or not */
  bool watched = false;

  uint32_t modes = SESSION_MODE_REGULAR;  /* session mode */
  std::unordered_set<std::string> subscribed_channels = {};  /* the channels which this session has already subscribed to */

  Session(int fd, uint32_t mask, ProcFuncType rpr, ProcFuncType wpr,
          EventLoop *loop, std::string name) : fd(fd), mask(mask),
                                               read_proc(std::move(rpr)), write_proc(std::move(wpr)),
                                               loop(loop), name(std::move(name)) {}

  ~Session() {
    read_buf.Reset();
    write_buf.Reset();
    watched = false;
  }

  void SetRead() {
    mask = EPOLLIN;
  }

  void SetWrite() {
    mask |= EPOLLOUT;
  }

  void DisableWrite() {
    mask = EPOLLIN;
  }

  void SetReadWrite() {
    mask = EPOLLIN | EPOLLOUT;
  }

  void SetPubSubMode() {
    modes |= SESSION_MODE_PUBSUB;
  }

  void UnsetPubSubMode() {
    modes &= ~SESSION_MODE_PUBSUB;
  }

};

typedef std::shared_ptr<Session> SessionPtr;

struct EventLoop {
  Epoller *epoller = nullptr;
  TimeEventHolder *tev_holder = nullptr;
  std::atomic_bool stopped{false};

  EventLoop();

  ~EventLoop();

  void Loop();

  TimeEvent *AddTimeEvent(uint64_t interval, std::function<void()> callback, int count) const;

  bool UpdateTimeEvent(long id, uint64_t interval, int count);

  bool RemoveTimeEvent(long id);

};

class Epoller {
public:
  explicit Epoller(size_t max_events);

  ~Epoller();

  bool AddFd(int fd, uint32_t events);

  bool DelFd(int fd);

  bool ModifyFd(int fd, uint32_t events);

  int Wait(int timeout_ms);

  std::vector<epoll_event> &GetEpollEvents() { return ep_events_; }

  bool AttachSession(Session *sev);

  bool ModifySession(Session *sev);

  bool DetachSession(Session *sev);

private:
  int epfd_ = -1;
  /* struct epoll_event */
  std::vector<epoll_event> ep_events_;
};

#endif // __NET_H__
