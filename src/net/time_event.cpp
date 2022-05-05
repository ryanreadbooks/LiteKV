#include <iostream>
#include "time_event.h"

long TimeEventHolder::sTimeEventId = 0;

TimeEventHolder::~TimeEventHolder() {
  /* free all time event structs */
  for (auto &ev : event_list_) {
    delete ev;
  }
}

TimeEvent* TimeEventHolder::CreateTimeEvent(uint64_t interval, std::function<void()> callback, int count) {
  ++sTimeEventId;
  /* create new time event struct */
  TimeEvent *tev = new(std::nothrow) TimeEvent(sTimeEventId, interval, std::move(callback), count);
  if (tev == nullptr) {
    return nullptr;
  }
  /* put into head */
  event_list_.push_front(tev);
  return tev;
}

bool TimeEventHolder::UpdateTimeEvent(long id, uint64_t interval, int count) {
  for (auto &ev : event_list_) {
    if (ev->id == id) {
      ev->count = count;
      ev->interval = interval;
      ev->when = AddMillSecToNowMillSec(interval);
      return true;
    }
  }
  return false;
}

bool TimeEventHolder::RemoveTimeEvent(long tev_id) {
  /* O(n) time to find corresponding TimeEvent object */
  for (auto it = event_list_.begin(); it != event_list_.end(); it++) {
    if ((*it)->id == tev_id) {
      auto deleted_event = *it;
      delete deleted_event;
      event_list_.erase(it);
      return true;
    }
  }
  return false;
}

TimeEvent *TimeEventHolder::FindNearestTimeEvent(uint64_t now) const {
  uint64_t min_diff = INT64_MAX;
  TimeEvent *tev = nullptr;
  for (auto &it : event_list_) {
    if (it->CannotFire()) continue;
    uint64_t diff = it->when - now;
    if (min_diff > diff) {
      tev = it;
      min_diff = diff;
    }
  }
  return tev;
}

int TimeEventHolder::HandleFiredTimeEvent() {
  /* handle all already expired time events, execute their callbacks */
  uint64_t now = GetCurrentMs();
  int processed = 0;
  for (auto it = event_list_.begin(); it != event_list_.end();) {
    auto &ev = *it;
    bool executed = false;
    if (ev->when <= now && ev->CanFire()) {
      ev->callback();
      executed = true;
      processed++;
      if (!ev->FireForever()) {
        ev->Consume();
      }
    }
    if (ev->CannotFire()) {
      /* free time event */
      delete ev;
      it = event_list_.erase(it);
      /* after executing callbacks, remove those expired events */
    } else {
      if (executed) {
        /* update next fire time */
        ev->when = AddMillSecToNowMillSec(ev->interval);
      }
      it++;
    }
  }
  return processed;
}

uint64_t TimeEventHolder::HowLongTillNextFired() const {
  auto now = GetCurrentMs();
  auto tev = FindNearestTimeEvent(now);
  if (tev == nullptr) return 50;
  return tev->when - now; /* ms */
}
