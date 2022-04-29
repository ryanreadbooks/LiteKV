#ifndef __NET_H__
#define __NET_H__

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include <sys/epoll.h>
#include "buffer.h"

const static int kReadable = 1;
const static int kWritable = 2;

class Epoller;

struct EventLoop;
struct CommandCache;
struct Session;

typedef std::function<void(Session *, bool&)> ProcFuncType;
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
};

struct Session {

  int fd; /* corresponding fd */
  /* our interested events */
  uint32_t mask;
  /* fired events */
  uint32_t events;

  ProcFuncType read_proc;  /* read handler */
  ProcFuncType write_proc; /* write handler */

  EventLoop *loop;
  /* indicate session is being watched or not */

  Buffer read_buf; /* read buffer */
  Buffer write_buf;/* write buffer */

  CommandCache cache;
  std::string name;
  bool watched = false;

  Session(int fd, uint32_t mask, ProcFuncType rpr, ProcFuncType wpr,
          EventLoop *loop, std::string name) : fd(fd), mask(mask),
      read_proc(std::move(rpr)), write_proc(std::move(wpr)),
      loop(loop), name(std::move(name)) {}

  ~Session() {
    std::cout << "session closed\n";
  }

  void SetRead() {
    mask = EPOLLIN;
  }

  void SetWrite() {
    mask = EPOLLOUT;
  }

  void SetReadWrite() {
    mask = EPOLLIN | EPOLLOUT;
  }
};

typedef std::shared_ptr<Session> SessionPtr;

struct EventLoop {
  Epoller *epoller = nullptr;
  std::atomic_bool stopped{false};

  EventLoop();

  ~EventLoop();

  void Loop();

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
