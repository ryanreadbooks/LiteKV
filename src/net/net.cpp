#include <iostream>
#include <memory>
#include <sys/epoll.h>
#include <unistd.h>
#include <signal.h>
#include "net.h"

EventLoop::EventLoop() :
    epoller(new Epoller(1024)), stopped(false) {}

EventLoop::~EventLoop() {
  if (epoller != nullptr) {
    delete epoller;
    epoller = nullptr;
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
    int ready = epoller->Wait(-1);
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
  }
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