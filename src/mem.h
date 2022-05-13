#ifndef __MEM_H__
#define __MEM_H__

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/fcntl.h>
#include <unistd.h>

static std::ifstream ifs_app_status("/proc/self/status", std::ios::in);
static std::ifstream ifs_meminfo_sys("/proc/meminfo", std::ios::in);

static bool CatMemInfo(size_t &mem_total, size_t &mem_free, size_t &mem_avail) {
  /* unit: kB  */
  if (!ifs_meminfo_sys.is_open()) {
    ifs_meminfo_sys.open("/proc/meminfo", std::ios::in);
    if (!ifs_meminfo_sys.is_open()) {
      return false;
    }
  }
  ifs_meminfo_sys.seekg(std::ios::beg);
  /* get memory in kB */
  /* first line is MemTotal */
  std::string tmp;
  ifs_meminfo_sys >> tmp >> mem_total;
  ifs_meminfo_sys >> tmp;
  /* second line is MemFree */
  ifs_meminfo_sys >> tmp >> mem_free;
  ifs_meminfo_sys >> tmp;
  /* third line is MemAvailable */
  ifs_meminfo_sys >> tmp >> mem_avail;
  return true;
}

static bool CatSelfMemInfo(size_t &vmsize, size_t& rss) {
  if (!ifs_app_status.is_open()) {
    ifs_app_status.open("/proc/self/status", std::ios::in);
    if (!ifs_app_status.is_open()) {
      return false;
    }
  }
  ifs_app_status.seekg(std::ios::beg);
  std::stringstream ss;
  ss << ifs_app_status.rdbuf();

  std::string s = ss.str();
  size_t pos = s.find("VmSize:");
  /* vmsize */
  vmsize = std::stol(s.substr(pos + 7, std::string::npos)); /* kB */
  /* VmRSS */
  pos = s.find("VmRSS:");
  rss = std::stol(s.substr(pos + 6, std::string::npos));  /* kB */
//  ifs_app_status.close();
  return true;
}

/**
 * @brief Report program process memory usage overview
 *
 * @return std::string
 */
static std::string ProcessVmSizeAsString() {
  size_t vmsize = 0, vmrss = 0;
  CatSelfMemInfo(vmsize, vmrss);
  double vmsize_d = static_cast<double>(vmsize) / 1024.0;
  double vmrss_d = static_cast<double>(vmrss) / 1024.0;
  std::stringstream ss;
  ss << "Virtual memory: " << vmsize_d << " MB, RSS memory: " << vmrss_d << " MB";
  return ss.str();
}

#endif // __MEM_H__