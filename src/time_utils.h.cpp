#ifndef __TIME_UTILS_H__
#define __TIME_UTILS_H__

#include <cstdint>
#include <sys/time.h>


static uint64_t GetCurrentSec() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec;
}

static void GetCurrentTime(long *seconds, long *milliseconds) {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  *seconds = tv.tv_sec;
  *milliseconds = tv.tv_usec / 1000;
}

#endif // __TIME_UTILS_H__