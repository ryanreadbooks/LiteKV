#ifndef __PERSISTENCE_H__
#define __PERSISTENCE_H__

#include <iostream>
#include <fstream>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "net/net.h"
#include "net/commands.h"

#define RESTORE_AOF_READ_BUF_SIZE 1024 * 64

class Engine;

class AppendableFile {
  using CmdCacheVector = std::vector<CommandCache>;

public:
  AppendableFile(std::string location, size_t cache_size = 1024);

  ~AppendableFile();

  inline std::string DumpLocation() const { return location_; }

  void Append(const CommandCache &cache);

  void FlushRightNow();

  std::vector<CommandCache> ReadFromScratch();

private:

  void Flush();

  bool OpenLocationFile();

  void Switch();

  void BackgroundHandler();

private:
  /* appendable file location */
  const std::string location_;
  const size_t cache_max_size_;
  std::fstream fs_;
  /* cache1 for commands */
  CmdCacheVector cache1_;
  /* cache2 for commands */
  CmdCacheVector cache2_;
  CmdCacheVector *cur_caches_;
  CmdCacheVector *backup_caches_;
  std::mutex mtx_;
  std::condition_variable cond_;
  std::thread worker_;
  bool stopped_;
};

#endif //__PERSISTENCE_H__
