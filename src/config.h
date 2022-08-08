#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <sstream>

#define CONFIG_DEFAULT_IP "127.0.0.1"
#define CONFIG_DEFAULT_PORT 9527

#define CONFIG_DEFAULT_APPENDONLY_ENABLED false
#define CONFIG_DEFAULT_DUMPFILE "dump.aof"
#define CONFIG_DEFAULT_DUMP_CACHESIZE 1024

#define CONFIG_DEFAULT_LRU_ENABLED false
#define CONFIG_DEFAULT_LRU_TRIGGER_RATIO 0.9f
#define CONFIG_DEFAULT_MAX_MEMORY 2048 /* unit: MB */

#define CONFIG_DEFAULT_KEEPALIVE_INTERVAL 100 /* unit: second */
#define CONFIG_DEFAULT_KEEPALIVE_CNT 3

class Config {
public:
  explicit Config(std::string filename);

  inline std::string ConfigFilename() const { return filename_; }

  inline std::string GetIp() const { return ip_; }

  inline int GetPort() const { return port_; }

  inline bool AppendonlyEnabled() const { return appendonly_enabled_; }

  inline std::string GetDumpFilename() const { return dumpfile_; }

  inline int GetDumpCacheSize() const { return dump_cachesize_; }

  inline bool LruEnabled() const { return lru_enabled_; }

  inline double LruTriggerRatio() const { return lru_trigger_ratio_; }

  inline size_t MaxMemLimit() const { return max_memory_limit_; }

  inline int KeepAliveInterval() const { return keepalive_interval_; }

  inline int KeepAliveCnt() const { return keepalive_cnt_; }

private:
  void Init(std::unordered_map<std::string, std::string>& configs);

private:
  /* default values */
  std::string filename_;
  std::string ip_ = CONFIG_DEFAULT_IP;
  int port_ = CONFIG_DEFAULT_PORT;

  volatile bool appendonly_enabled_ = CONFIG_DEFAULT_APPENDONLY_ENABLED;
  std::string dumpfile_ = CONFIG_DEFAULT_DUMPFILE;
  int dump_cachesize_ = CONFIG_DEFAULT_DUMP_CACHESIZE;

  volatile bool lru_enabled_ = CONFIG_DEFAULT_LRU_ENABLED;
  double lru_trigger_ratio_ = CONFIG_DEFAULT_LRU_TRIGGER_RATIO;
  size_t max_memory_limit_ = CONFIG_DEFAULT_MAX_MEMORY; 

  int keepalive_interval_ = CONFIG_DEFAULT_KEEPALIVE_INTERVAL;
  int keepalive_cnt_ = CONFIG_DEFAULT_KEEPALIVE_CNT;
};

#endif // __CONFIG_H__
