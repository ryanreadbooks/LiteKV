#include <iostream>
#include <memory>
#include <sys/epoll.h>
#include <unistd.h>
#include "net.h"

EventLoop::EventLoop() :
    epoller(new (std::nothrow) Epoller(1024)),
    tev_holder(new (std::nothrow) TimeEventHolder),
    stopped(false) {
  if (epoller == nullptr) {
    std::cerr << "Can not initialize epoller in event loop, program abort.\n";
    exit(EXIT_FAILURE);
  }
  if (tev_holder == nullptr) {
    std::cerr << "Can not initialize time event handler in event loop, program abort.\n";
    exit(EXIT_FAILURE);
  }
}

EventLoop::~EventLoop() {
  if (epoller != nullptr) {
    delete epoller;
    epoller = nullptr;
  }
  if (tev_holder != nullptr) {
    delete tev_holder;
    tev_holder = nullptr;
  }
  stopped.store(true);
}

void EventLoop::Loop() {
  if (epoller == nullptr) {
    return;
  }
  static int debug_n = 1;
  std::cout << "Event loop working...\n";
  while (!stopped) {
    // TODO change optimize timeout ms
    uint64_t ms = tev_holder->HowLongTillNextFired();
    int ready = epoller->Wait(ms);
    /* process events one by one */
    // std::cout << "ready = " << ready << std::endl;
    debug_n += ready;
    for (int i = 0; ready != -1 && i < ready; ++i) {
      uint32_t fired_events = epoller->GetEpollEvents()[i].events;
      Session *session = static_cast<Session *>(epoller->GetEpollEvents()[i].data.ptr);
      // std::cout << "fired_events = " << fired_events << std::endl;
      if (!session) {
        continue;
      }
      bool closed = false;  /*　flag to indicate whether session is freed to avoid memory issue */
      if (session->mask & fired_events & EPOLLIN) {
        session->read_proc(session, closed);
      }
      if (closed) {
        continue;
      }
      if (session->mask & fired_events & EPOLLOUT) {
        if (session->write_proc) {
          session->write_proc(session, closed);
        }
      }
    }
    /* handle time events */
    tev_holder->HandleFiredTimeEvent();
  }
}

TimeEvent* EventLoop::AddTimeEvent(uint64_t interval, std::function<void ()> callback, int count) const {
  return tev_holder->CreateTimeEvent(interval, std::move(callback), count);
}

bool EventLoop::UpdateTimeEvent(long id, uint64_t interval, int count) {
  return tev_holder->UpdateTimeEvent(id, interval, count);
}

bool EventLoop::RemoveTimeEvent(long id) {
  return tev_holder->RemoveTimeEvent(id);
}

Epoller::Epoller(size_t max_events) :
    epfd_(epoll_create(max_events)), ep_events_(max_events) {
}

Epoller::~Epoller() {
  ::close(epfd_);
}

bool Epoller::AddFd(int fd, uint32_t events) {
  if (epfd_ == -1) return false;
  struct epoll_event ev{0};
  ev.data.fd = fd;
  ev.events = events;
  return epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == 0;
}

bool Epoller::DelFd(int fd) {
  if (epfd_ == -1) return false;
  return epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr) == 0;
}

bool Epoller::ModifyFd(int fd, uint32_t events) {
  if (epfd_ == -1) return false;
  struct epoll_event ev{0};
  ev.data.fd = fd;
  ev.events = events;
  return epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) == 0;
}

int Epoller::Wait(int timeout_ms) {
  if (epfd_ == -1) {
    return -1;
  }
  return epoll_wait(epfd_, &ep_events_[0], static_cast<int>(ep_events_.size()), timeout_ms);
}

bool Epoller::AttachSession(Session *sev) {
  if (!sev) {
    return false;
  }
  epoll_event epev{0};
  epev.data.ptr = static_cast<void *> (sev);
  epev.events = sev->mask;
  int ans = -1;
  if (!sev->watched) {
    if ((ans = epoll_ctl(epfd_, EPOLL_CTL_ADD, sev->fd, &epev)) == 0) {
      std::cout << "fd = " << sev->fd << " attached\n";
      sev->watched = true;
    }
  } else {
    ans = epoll_ctl(epfd_, EPOLL_CTL_MOD, sev->fd, &epev);
  }
  return ans == 0;
}

bool Epoller::ModifySession(Session *sev) {
  return AttachSession(sev);
}

bool Epoller::DetachSession(Session *sev) {
  if (!sev) {
    return false;
  }
  int ans = -1;
  if ((ans = epoll_ctl(epfd_, EPOLL_CTL_DEL, sev->fd, nullptr)) == 0) {
    sev->watched = false;
  }
  return ans == 0;
}