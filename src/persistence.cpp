#include <unistd.h>
#include <chrono>
#include <utility>
#include "persistence.h"
#include "net/protocol.h"

AppendableFile::AppendableFile(std::string location, size_t cache_size)
    : location_(std::move(location)), cache_max_size_(cache_size), stopped_(false) {
  cache1_.reserve(cache_size);
  cache2_.reserve(cache_size);
  cur_caches_ = &cache1_;
  backup_caches_ = &cache2_;
  /* open file */
  if (!OpenLocationFile()) {
    std::cerr << "Can not open appendable file '" << location << "' at initialization.\n";
  }
  /* if not empty, read and restore data */
  std::thread t(std::bind(&AppendableFile::BackgroundHandler, this));
  worker_.swap(t);
}

AppendableFile::~AppendableFile() {
  /* flush all existing caches and then clear space */
  stopped_ = true;
  cond_.notify_one();
  worker_.join();
  Switch();
  Flush();
  cache1_.clear();
  cache2_.clear();
  fs_.close();
}

void AppendableFile::BackgroundHandler() {
  while (!stopped_) {
    std::unique_lock<std::mutex> lck(mtx_);
    cond_.wait(lck, [this] {
      return backup_caches_->size() >= cache_max_size_ * 0.80 || stopped_;
    });
    /* worker wakes up and we have lock */
    Flush();
  }
}

void AppendableFile::Append(const CommandCache &cache) {
  /* always put cache into cur_caches_ */
  if (cur_caches_->size() >= cache_max_size_) {
    Switch();
    cond_.notify_one();
  }
  cur_caches_->push_back(cache);
}

void AppendableFile::FlushRightNow() {
  if (OpenLocationFile()) {
    fs_.flush();
  }
}

std::vector<CommandCache> AppendableFile::ReadFromScratch() {
  std::vector<CommandCache> ans;
  std::ifstream ifs(location_, std::ios::in);
  if (ifs.is_open()) {
    /* read all */
    ifs.seekg(0, std::ios::end);  /* go to end of file */
    int64_t length = ifs.tellg();  /* report length of the whole file */
    ifs.seekg(0, std::ios::beg);
    Buffer buffer;
    CommandCache cache;
    int64_t n_cur_read = 0;
    int times = 0;
    while (!ifs.eof()) {
      char buf[4096 * 8];
      memset(buf, 0, sizeof(buf));
      ifs.read(buf, sizeof(buf));
      size_t n_read = ifs.gcount();
//      std::cout << "n_read = " << n_read << std::endl;
      n_cur_read += n_read;
      /* split into small chunks and process one of them */
      if (n_read == 0) {
        break;
      }
      buffer.Append(buf, n_read);
      bool err = false;
      while (TryParseFromBuffer(buffer, cache, err)) {
        ans.push_back(cache);
        cache.Clear();
      }
    }
    ifs.close();
    // 有剩余的字节没有被解析完，尝试泵不能解析出来
    if (buffer.ReadableBytes() > 0) {
      bool err;
      while (TryParseFromBuffer(buffer, cache, err)) {
        ans.push_back(cache);
        cache.Clear();
      }
    }
    std::cout << "\nTotal " << ans.size() << " records loaded\n";
    std::cout << "File length = " << length << " bytes, total_read = "
    << n_cur_read << " bytes." << std::endl;
    return ans;
  }
  return ans;
}

void AppendableFile::Flush() {
  /* always flush backup_caches_ into disk */
  if (OpenLocationFile()) {
    for (const auto &item : *backup_caches_) {
      fs_ << item.ToProtocolString();
    }
    fs_.flush();
    backup_caches_->clear();
  }
}

bool AppendableFile::OpenLocationFile() {
  if (!fs_.is_open()) {
    fs_.open(location_, std::fstream::app);
  }
  return fs_.is_open();
}

void AppendableFile::Switch() {
  std::lock_guard<std::mutex> lck(mtx_);
  std::swap(cur_caches_, backup_caches_);
}