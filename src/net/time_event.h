#ifndef __TIME_EVENT_H__
#define __TIME_EVENT_H__

#include <cstdint>
#include <functional>
#include <list>
#include <sys/time.h>

static uint64_t GetCurrentSec() {
  struct timeval tv{};
  gettimeofday(&tv, nullptr);
  return tv.tv_sec;
}

static uint64_t GetCurrentMs() {
  struct timeval tv{};
  gettimeofday(&tv, nullptr);
  return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

static uint64_t GetCurrentUs() {
  struct timeval tv{};
  gettimeofday(&tv, nullptr);
  return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}

static void GetCurrentTime(long *seconds, long *milliseconds) {
  struct timeval tv{};
  gettimeofday(&tv, nullptr);
  *seconds = tv.tv_sec;
  *milliseconds = tv.tv_usec / 1000;
}

static uint64_t AddSecondToNowSec(uint64_t seconds) {
  return GetCurrentSec() + seconds; /* seconds */
}

static uint64_t AddMillSecToNowMillSec(uint64_t milliseconds) {
  return GetCurrentMs() + milliseconds; /* milliseconds */
}

static uint64_t AddSecondToNowMillSec(uint64_t seconds) {
  return GetCurrentMs() + seconds * 1000ul; /* milliseconds */
}

#define NO_TIME_EVENT -1
#define FIRE_FOREVER -1

struct TimeEvent {
  /* time event unique id */
  long id;
  /* unix time when this time event fires. unit: millisecond */
  uint64_t when;
  /* interval, unit: millisecond */
  uint64_t interval;
  /* when event fires, callback function is called */
  std::function<void()> callback;
  /* the number of times the event can be fired, negative means unlimited count */
  int count;

  TimeEvent(long id, uint64_t interval, std::function<void()> callback, int count = 1) :
      id(id), interval(interval), callback(std::move(callback)), count(count) {
    when = AddMillSecToNowMillSec(interval);
  }

  inline bool FireForever() const { return count < 0; }

  inline bool CanFire() const { return count > 0 || count < 0; }

  inline bool CannotFire() const { return count == 0; }

  inline void Consume() { --count; }

};

class TimeEventHolder {
public:
  TimeEventHolder() = default;

  ~TimeEventHolder();

  TimeEvent* CreateTimeEvent(uint64_t interval, std::function<void()> callback, int count = 1);

  bool UpdateTimeEvent(long id, uint64_t interval, int count = 1);

  bool RemoveTimeEvent(long tev_id);

  TimeEvent *FindNearestTimeEvent(uint64_t now) const;

  int HandleFiredTimeEvent();

  uint64_t HowLongTillNextFired() const;

private:
  std::list<TimeEvent *> event_list_;
  static long sTimeEventId;
};

#endif // __TIME_EVENT_H__
