#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/fcntl.h>
#include <unistd.h>

/**
 * @brief Report program process memory usage overview
 *
 * @return std::string
 */
static std::string ProcessVmSize() {
  char buf[65536];
  memset(buf, 0, sizeof(buf));
  int fd = ::open("/proc/self/status", O_RDONLY | O_CLOEXEC);
  int ret = ::read(fd, buf, sizeof(buf));
  std::string overview_str(buf, sizeof(buf));
  /* VmSize */
  size_t pos = overview_str.find("VmSize:");
  double vmsize = static_cast<double>(std::stol(
                      overview_str.substr(pos + 7, std::string::npos))) / 1024.0;
  /* VmRSS */
  pos = overview_str.find("VmRSS:");
  double vmrss = static_cast<double>(std::stol(
                     overview_str.substr(pos + 6, std::string::npos))) / 1024.0;
  std::stringstream ss;
  ss << "Virtual memory: " << vmsize << " MiB, RSS memory: " << vmrss << " MiB";
  return ss.str();
}