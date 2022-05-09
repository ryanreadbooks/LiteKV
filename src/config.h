#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <sstream>

class Config {
public:
  explicit Config(std::string filename);

  inline std::string ConfigFilename() const { return filename_; }

  inline std::string GetIp() const { return ip_; }

  inline int GetPort() const { return port_; }

  inline std::string GetDumpFilename() const { return dumpfile_; }

  inline int GetDumpCacheSize() const { return dump_cachesize_; }

private:
  void Init(std::unordered_map<std::string, std::string>& configs);

private:
  /* default values */
  std::string filename_;
  std::string ip_ = "127.0.0.1";
  int port_ = 9527;
  std::string dumpfile_ = "dump.aof";
  int dump_cachesize_ = 1024;
};

#endif // __CONFIG_H__
