#include <unistd.h>
#include <cassert>
#include <chrono>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <cmath>
#include "persistence.h"
#include "net/protocol.h"
#include "str.h"

#define OP_TYPE_LIST 0
#define OP_TYPE_HASH 1
#define OP_TYPE_STRING 2
#define OP_TYPE_INTEGER 3
#define OP_TYPE_SET 4
#define OP_TYPE_OTHER 5

AppendableFile::AppendableFile(std::string location, size_t cache_size, bool auto_flush,
                               size_t flush_interval)
    : location_(std::move(location)),
      cache_max_size_(cache_size),
      stopped_(false),
      auto_flush_(auto_flush),
      last_flush_timestamp_(GetCurrentSec()),
      flush_interval_(flush_interval) {
  cache1_.reserve(cache_size);
  cache2_.reserve(cache_size);
  cur_caches_ = &cache1_;
  backup_caches_ = &cache2_;
  /* open file */
  if (!OpenLocationFile()) {
    if (!OpenLocationFile()) {
      /* TODO: is there a better way to handle file open error? */
      std::cerr << "Can not open appendable file '" << location << "' at initialization. Quit LiteKV.\n";
      abort();
    }
  }
  /* if not empty, read and restore data */
  std::thread t(std::bind(&AppendableFile::BackgroundHandler, this));
  worker_.swap(t);
  std::cout << "AOF BackgroundHandler started...\n";
}

AppendableFile::~AppendableFile() {
  /* flush all existing caches and then clear space */
  std::cout << "Saving database file into disk...\n";
  stopped_ = true;
  auto_flush_ = true;
  cond_.notify_one();
  worker_.join(); /* worker will flush backup_cache_ before exitting */
  Switch(); /* switch to flush cur_cache_ */
  Flush();
  cache1_.clear();
  cache2_.clear();
  fs_.close();
  std::cout << "Database file saved on disk. \n";
}

void AppendableFile::BackgroundHandler() {
  while (!stopped_) {
    std::unique_lock<std::mutex> lck(mtx_);
    cond_.wait_for(lck, std::chrono::seconds(flush_interval_), [this] {
      return auto_flush_ && (backup_caches_->size() >= cache_max_size_ * 0.80 || stopped_);
    });
    /* worker wakes up and we have lock */
    last_flush_timestamp_ = GetCurrentSec();
    Flush();
  }
}

void AppendableFile::Append(const CommandCache &cache) {
  /* always put cache into cur_caches_ */
  if (cur_caches_->size() >= cache_max_size_) {
    Switch();
    if (auto_flush_) {
      cond_.notify_one();
    }
  }
  cur_caches_->push_back(cache);
}

void AppendableFile::FlushRightNow() {
  if (OpenLocationFile()) {
    fs_.flush();
  }
}

static void CommonOperation(std::ifstream &ifs, const std::function<void(CommandCache &)>& whattodo) {
  ifs.seekg(0, std::ios::end);
  int64_t length = ifs.tellg();  /* report length of the whole file */
  ifs.seekg(0, std::ios::beg);
  if (length >= 1024 * 1024 * 100) {
    std::cout << "The size of dumpfile is larger than 100MB, loading it might take a while...\n";
  }
  Buffer buffer;
  CommandCache cache;
  int64_t n_cur_read = 0;
  int64_t n_records = 0;
  while (!ifs.eof()) {
    char buf[RESTORE_AOF_READ_BUF_SIZE];
    memset(buf, 0, sizeof(buf));
    ifs.read(buf, sizeof(buf));
    size_t n_read = ifs.gcount();
    n_cur_read += n_read;
    if (n_read == 0) {
      break;
    }
    buffer.Append(buf, n_read);
    bool err = false;
    while (TryParseFromBuffer(buffer, cache, err)) {
      ++n_records;
      /* perform unique operation */
      whattodo(cache);
      cache.Clear();
    }
  }
  ifs.close();
  /* try to parse the rest bytes in buffer */
  if (buffer.ReadableBytes() > 0) {
    bool err;
    while (TryParseFromBuffer(buffer, cache, err)) {
      whattodo(cache);
      ++n_records;
      cache.Clear();
    }
  }
  std::cout << "Total " << n_records << " records loaded\n";
  std::cout << "File length = " << length << " bytes, total read = "
            << n_cur_read << " bytes." << std::endl;
}

void AppendableFile::ReadFromScratch(Engine *engine, EventLoop *loop) {
  std::ifstream ifs(location_, std::ios::in);
  if (ifs.is_open()) {
    CommonOperation(ifs, [this, engine, loop](CommandCache &cache) {
      engine->HandleCommand(loop, cache, false);
    });
  }
}

std::vector<CommandCache> AppendableFile::ReadFromScratch() {
  std::vector<CommandCache> ans;
  std::ifstream ifs(location_, std::ios::in);
  if (ifs.is_open()) {
    CommonOperation(ifs, [&](CommandCache &cache) {
      ans.push_back(cache);
    });
    return ans;
  }
  return ans;
}

void AppendableFile::Flush() {
  /* the outsize should make sure we have mutex */
  if (backup_caches_->empty()) {
    if (cur_caches_->empty()) {
      return; /* nothing to flush */
    }
    std::swap(cur_caches_, backup_caches_);
  }
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
  std::lock_guard<std::mutex> lck(mtx_);  /* FIXME: may block request outside and cause slow response */
  std::swap(cur_caches_, backup_caches_);
}

/* TODO: NOT FULLY TESTED YET.
 * For now, only the following commands appear in aof file:
 *  generic: set, del, expireat,
 *  integer or string: incr, decr, incrby, decrby, append,
 *  list: lpush, rpush, lpop, rpop, lsetindex,
 *  hash: hset, hdel,
 *  set: sadd, srem
*/
void AppendableFile::RemoveRedundancy(const std::string &source_file) {
  /* refactor dumpfile, remove those redundant commands,
   * such as set one key, and then del this key, in this case, we do not need to set this key at all;
   * or if we set one key to value1, and the set this key to another value2, in this case, we can just set key to value2 directly.
  */
  /* load dump file and iterate the whole file to process */
  std::cout << "Starts analysing and removing redundancy...\n";
  /* map to store key-operation pairs: to store what operations that a key has */
  std::unordered_map<std::string, std::list<CommandCache>> cache_map;
  std::ifstream ifs(source_file, std::ios::in);
  if (ifs.is_open()) {
    auto populate_caches_func = [this, &cache_map](CommandCache &cache) {
      /* cache: operation key operands... */
      const std::string &key = cache.argv[1];
      /* put the operation command of the same key into one list */
      cache_map[key].push_back(cache);
    };
    std::cout << "Populating caches...\n";
    CommonOperation(ifs, populate_caches_func);
    size_t counter = 0;
    /* process all keys command operations */
    for (auto &&item : cache_map) {
      std::cout << "Processing (" << ++counter << "/" << cache_map.size() << ")...\n";
      /* commands on the same list have identical key */
      const std::string &key = item.second.front().argv[1];
      std::queue<CommandCache> sequential;  /* sequential is to store a serial of commands on one key in the original order */
      const std::list<CommandCache> commands = item.second;
      /* iterate every command of this key */
      for (auto &&cmd : commands) {
        const std::string &operation = cmd.argv[0];
        /* del command has the highest priority,
         * and set command will overwrite existing keys no matter what the type of key is,
         * expireat command will delete the key as well if current time is greater*/
        if (operation == "del" || operation == "set" || operation == "expireat") {
          bool clear_all = false;
          if (operation == "expireat") {
            /* check if key expire */
            /* if key expires, none of the previous commands make sense */
            uint64_t expireat = GetCurrentSec();
            CanConvertToUInt64(cmd.argv[2], expireat);
            if (GetCurrentSec() >= expireat) {
              clear_all = true; /* till now, key has expired */
            } else {
              /* till now, key not expired */
              sequential.push(cmd);
            }
          } else {
            /* if it is not 'expireat', we need to invalidate previous content
             * (because 'del' deletes key and 'set' overwrites key) */
            clear_all = true;
          }
          /* invalidate previous cached commands when meets 'del' or 'set' command, or 'expireat' command decides to invalidate */
          if (clear_all) {
            while (!sequential.empty()) {
              sequential.pop();
            }
          }
        }
        /* command still counts */
        if (operation != "del" && operation != "expireat") {
          sequential.push(cmd);
        }
      }
      /* finally get pre-processed commands stored in a queue 'sequential' for one key.
       * The following mainly handle list, hash and set structure operations */
      std::deque<std::string> aux_list; /* list insertion simulation */
      std::unordered_map<std::string, std::string> aux_hash; /* hash insertion simulation */
      std::unordered_set<std::string> aux_uset; /* set operation simulation */
      std::string aux_string; /* string operation simulation */
      std::int64_t aux_int64 = 0;
      CommandCache cache;
      uint8_t op_type;
      if (!sequential.empty()) {
        while (!sequential.empty()) {
          const std::vector<std::string>& operands = sequential.front().argv;
          const std::string &op = operands[0];
          if (op == "lpush" || op == "rpush" || op == "lpop" || op == "rpop" || op == "lsetindex") {
            op_type = OP_TYPE_LIST;
            /* if starts with list operation, the rest is list operation */
            if (op == "lpush") {
              for (size_t i = 2; i < operands.size(); ++i) {
                aux_list.push_front(operands[i]);
              }
            } else if (op == "rpush") {
              for (size_t i = 2; i < operands.size(); ++i) {
                aux_list.push_back(operands[i]);
              }
            } else if (op == "lpop") {
              aux_list.pop_front();
            } else if (op == "rpop") {
              aux_list.pop_back();
            } else if (op == "lsetindex") {
              const std::string& idx = operands[2];
              aux_list[std::stoul(idx)] = operands[3];
            }
          } else if (op == "hset" || op == "hdel") {
            op_type = OP_TYPE_HASH;
            /* if starts with hash operation, the rest is hash operation */
            if (op == "hset") {
              for (size_t i = 2; i < operands.size(); i += 2) {
                aux_hash[operands[i]] = operands[i + 1];
              }
            } else if (op == "hdel") {
              for (size_t i = 2; i < operands.size(); ++i) {
                aux_hash.erase(operands[i]);
              }
            }
          } else if (op == "sadd" || op == "srem") {
              /* if starts with set operation, the rest is set operation */
              op_type = OP_TYPE_SET;
              /* either all 'sadd' or all 'srem' in one loop */
              for (size_t i = 2; i < operands.size(); ++i) {
                if (op == "sadd") {
                  aux_uset.insert(operands[i]);
                } else if (op == "srem") {
                  aux_uset.erase(operands[i]);
                }
              }
          } else if (op == "append") {
            op_type = OP_TYPE_STRING;
            aux_string.append(operands[2]);
          } else if (op == "incr" || op == "decr" || op == "incrby" || op == "decrby") {
            op_type = OP_TYPE_INTEGER;
            if (CanConvertToInt64(aux_string, aux_int64) || aux_string.empty()) {
              if (op == "incr") {
                aux_string = std::to_string(aux_int64 + 1);
              } else if (op == "decr") {
                aux_string = std::to_string(aux_int64 - 1);
              } else if (op == "incrby") {
                aux_string = std::to_string(aux_int64 + std::stoll(operands[2]));
              } else {  /* op == "decrby */
                aux_string = std::to_string(aux_int64 - std::stoll(operands[2]));
              }
            }
          } else {
            op_type = OP_TYPE_OTHER;
            assert(op == "set");
            if (sequential.size() != 1) {
              aux_string = operands[2]; /*　init aux_string */
            } else {
              Append(sequential.front());/* set command alone */
            }
          }
          sequential.pop();
        } // finish processing one key
        /* After done processing a series of operations on this key, we can now restore the command that can generate the final result and add it to file */
        if (op_type == OP_TYPE_LIST) {
          /* sync list generation command into buffer */
          cache.argv.assign(aux_list.begin(), aux_list.end());
          cache.argv.insert(cache.argv.begin(), key);
          cache.argv.insert(cache.argv.begin(), "rpush");
          cache.argc = cache.argv.size();
          Append(cache);
          cache.Clear();
          aux_list.clear();
        } else if (op_type == OP_TYPE_HASH) {
          /* sync hash generation command into buffer */
          for (auto&& kv : aux_hash) {
            cache.argv.emplace_back(kv.first);
            cache.argv.emplace_back(kv.second);
          }
          cache.argv.insert(cache.argv.begin(), key);
          cache.argv.insert(cache.argv.begin(), "hset");
          cache.argc = cache.argv.size();
          Append(cache);
          cache.Clear();
          aux_hash.clear();
        } else if (op_type == OP_TYPE_SET) {
          /* sync set generation command into buffer */
          cache.argv.assign(aux_uset.begin(), aux_uset.end());
          cache.argv.insert(cache.argv.begin(), key);
          cache.argv.insert(cache.argv.begin(), "sadd");
          cache.argc = cache.argv.size();
          Append(cache);
          cache.Clear();
          aux_uset.clear();
        } else if (op_type == OP_TYPE_INTEGER || op_type == OP_TYPE_STRING) {
          cache.argv = {"set", key, aux_string};
          cache.argc = cache.argv.size();
          Append(cache);
          cache.Clear();
          aux_string.clear();
        }
      }
    }
  } else {
    std::cerr << "Can not open source file " << source_file << "[" << strerror(errno) << "]\n";
  }
}

void AppendableFile::SetAutoFlush(bool on) {
  auto_flush_.store(on);
  if (auto_flush_) {
    cond_.notify_one();
  }
}

#undef CommonOperation