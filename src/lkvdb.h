#ifndef __LKVDB_H__
#define __LKVDB_H__

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "str.h"
#include "core.h"
#include "dlist.h"
#include "hashdict.h"
#include "hashset.h"
#include "net/net.h"
#include "net/server.h"

class KVContainer;
class EventLoop;
class Server;

#define LKVDB_MAGIC_NUMBER "LITEKV"
#define LKVDB_VERSION 1
#define LKVDB_HEADER_SIZE (strlen(LKVDB_MAGIC_NUMBER) + 4)

#define LKVDB_ITEM_START_FLAG 0xFF /* the flag to indicate an entry starts */
#define LKVDB_ITEM_END_FLAG 0xFE   /* the flag to indicate an entry ends */

struct MagicHeader {
  char magic[6];
  char version[4];
} __attribute__((packed));

class FileFdRAII {
public:
  explicit FileFdRAII(const std::string& name, int flag) : fd_(open(name.c_str(), flag)) {}

  ~FileFdRAII() {
    if (fd_ != -1) {
      close(fd_);
    }
  }

  FileFdRAII(const FileFdRAII&) = delete;

  FileFdRAII(FileFdRAII&&) = delete;

  FileFdRAII& operator=(const FileFdRAII&) = delete;

  int Fd() const { return fd_; }

private:
  int fd_ = -1;
};

class MmapRAII {
public:
  explicit MmapRAII(int fd, size_t size, int prot, int flags, off_t offset)
      : ptr_(mmap(nullptr, size, prot, flags, fd, offset)), size_(size) {}

  ~MmapRAII() {
    if (ptr_) {
      munmap(ptr_, size_);
    }
  }

  MmapRAII(const MmapRAII&) = delete;

  MmapRAII(MmapRAII&&) = delete;

  MmapRAII& operator=(const MmapRAII&) = delete;

  void* Ptr() const { return ptr_; }

private:
  void* ptr_ = nullptr;
  size_t size_;
};

/**
 * @brief Save the buf content in current process
 *
 * @param dst filename of destination
 * @param buf buffer to be saved
 * @return true save succeed
 * @return false save failed
 */
bool LiteKVSave(const std::string& dst, const std::vector<char>& buf);

/**
 * @brief Save the buf content in background process
 *
 * @param dst filename of destination
 * @param server pointer to Server instance
 * @param holder pointer to KVContainer instance
 * @return true save succeed
 * @return false save failed
 */
bool LiteKVBackgroundSave(const std::string& dst, Server* server, KVContainer* holder);

/**
 * @brief Load the content from source src
 *
 * @param src source filename
 * @param holder KVContainer
 * @param loop EventLoop for adding timer event
 * @param callback the callback function to update loading process
 */
void LiteKVLoad(const std::string& src, KVContainer* holder, EventLoop* loop,
                const std::function<void(size_t&, size_t&)>& callback);

#endif  // __LKVDB_H__