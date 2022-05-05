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
    while (!ifs.eof()) {
      char buf[4096];
      memset(buf, 0, sizeof(buf));
      size_t n_read = ifs.readsome(buf, sizeof(buf));
      /* split into small chunks and process one of them */
      if (n_read > 0) {
        buffer.Append(buf, n_read);
        size_t parse_start_idx = buffer.BeginReadIdx();
        if (cache.inited && cache.argc > cache.argv.size()) {
          if (!AuxiliaryReadProc(buffer, cache, n_read)) {
            continue;
          }
        } else { /* cache not inited or this is a brand new command request */
          parse_protocol_new_request:
          if (buffer.ReadStdString(1) == "*") { /* a brand new command */
            buffer.ReaderIdxForward(1);
            size_t step;
            cache.argc = buffer.ReadLongAndForward(step);
            if (buffer.ReadStdString(2) != kCRLF) {
              AuxiliaryReadProcCleanup(buffer, cache, parse_start_idx, n_read);
              continue;
            }
            if (buffer.ReadStdString(2) != kCRLF) {
              AuxiliaryReadProcCleanup(buffer, cache, parse_start_idx, n_read);
              continue;
            }
            buffer.ReaderIdxForward(2);
            cache.inited = true;
            if (!AuxiliaryReadProc(buffer, cache, n_read)) {
              continue;
            }
          } else {
            AuxiliaryReadProcCleanup(buffer, cache, parse_start_idx, n_read);
            continue;
          }
        }
        /* successfully parse one whole request, use it to operator the database */
        if (cache.argc != 0 && cache.argc == cache.argv.size()) {
          /*　one whole command fully received till now, process it */
          ans.push_back(cache);
          /* clear cache when one command is fully parsed */
          cache.Clear();
          /* handle multiple request commands received in one read */
          if (buffer.ReadableBytes() > 0 && buffer.ReadStdString(1) == "*") {
            goto parse_protocol_new_request;
          }
        }
      } else {
        break;
      }
    }
  }
  ifs.close();
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