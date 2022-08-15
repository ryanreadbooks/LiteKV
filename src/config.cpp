#include "config.h"
#include <algorithm>
#include "str.h"

#define DISPLAY_CONFIG(item_name, item_value)                          \
  do {                                                                 \
    std::cout << "[Server config " << item_name << "]: " << item_value \
              << '\n';                                                 \
  } while (0)

#define DISPLAY_INVALID_WARN(item_name, default_value)                      \
  do {                                                                      \
    std::cout << "[SERVER CONFIG WARN] " << item_name                       \
              << " invalid, use default " << default_value << " instead\n"; \
  } while (0)

static bool ValidateIpv4(const std::string &ip) {
  std::stringstream ss(ip);
  std::string split = "";
  char cnt = 0;
  while (std::getline(ss, split, '.')) {
    /* check valid number [0 ~ 255] */
    int val;
    if (!CanConvertToInt32(split, val) || 0 > val || val > 255) {
      break;
    }
    cnt++;
  }
  return cnt == 4;
}

Config::Config(std::string filename) : filename_(std::move(filename)) {
  std::ifstream ifs;
  ifs.open(filename_, std::ios::in);
  /* parse config */
  if (ifs.is_open()) {
    ifs.seekg(0, std::ios::end);
    int length = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::string first;
    std::string second;
    std::unordered_map<std::string, std::string> configs;
    std::string line;
    std::stringstream ss;
    while (!ifs.eof()) {
      ss.clear();
      std::getline(ifs, line);
      if (line.empty() || line[0] == '#') {
        continue;
      }
      /* parse line */
      ss << line;
      ss >> first;
      if (first.empty()) {
        continue;
      }
      ss >> second;
      if (second.empty()) {
        continue;
      }
      std::transform(first.begin(), first.end(), first.begin(), ::tolower);
      std::transform(second.begin(), second.end(), second.begin(), ::tolower);
      configs[first] = second;
    }
    std::cout << "Loading config file...\n";
    Init(configs);
    std::cout << "Config loaded in " << filename_ << '\n';
    ifs.close();
  } else {
    std::cerr << "Can not open config file in " << filename_ << '\n';
  }
}

void Config::Init(std::unordered_map<std::string, std::string> &configs) {
  for (auto &config : configs) {
    const std::string &key = config.first;
    if (key.empty() || key[0] == '#') {
      continue;
    }
    const std::string &value = config.second;
    if (key == "ip") {
      if (!ValidateIpv4(value)) {
        DISPLAY_INVALID_WARN(key, CONFIG_DEFAULT_IP);
      } else {
        ip_ = value;
      }
      DISPLAY_CONFIG(key, ip_);
    } else if (key == "port") {
      if (!CanConvertToInt32(value, port_)) {
        DISPLAY_INVALID_WARN(key, CONFIG_DEFAULT_PORT);
        port_ = CONFIG_DEFAULT_PORT;
      }
      DISPLAY_CONFIG(key, port_);
    } else if (key == "appendonly") {
      int b = 0;
      if (!CanConvertToInt32(value, b)) {
        DISPLAY_INVALID_WARN(key, CONFIG_DEFAULT_APPENDONLY_ENABLED);
      }
      appendonly_enabled_ = b != 0;
      DISPLAY_CONFIG(key, appendonly_enabled_);
    } else if (key == "dumpfile") {
      dumpfile_ = value;
      DISPLAY_CONFIG(key, dumpfile_);
    } else if (key == "dump-cachesize") {
      if (!CanConvertToInt32(value, dump_cachesize_)) {
        DISPLAY_INVALID_WARN(key, CONFIG_DEFAULT_DUMP_CACHESIZE);
        dump_cachesize_ = CONFIG_DEFAULT_DUMP_CACHESIZE;
      }
      DISPLAY_CONFIG(key, dump_cachesize_);
    } else if (key == "dump-flush-interval") {
      if (!CanConvertToUInt64(value, dump_flush_interval_)) {
        DISPLAY_INVALID_WARN(key, CONFIG_DEFAULT_DUMP_FLUSH_INTERVAL);
        dump_flush_interval_ = CONFIG_DEFAULT_DUMP_FLUSH_INTERVAL;
      }
      DISPLAY_CONFIG(key, dump_flush_interval_);
    } else if (key == "lru-enable") {
      int b = 0;
      if (!CanConvertToInt32(value, b)) {
        DISPLAY_INVALID_WARN(key, CONFIG_DEFAULT_LRU_ENABLED);
      }
      lru_enabled_ = b != 0;
      DISPLAY_CONFIG(key, lru_enabled_);
    } else if (key == "lru-trigger-ratio") {
      double f = CONFIG_DEFAULT_LRU_TRIGGER_RATIO;
      if (!CanConvertToDouble(value, f)) {
        DISPLAY_INVALID_WARN(key, CONFIG_DEFAULT_LRU_TRIGGER_RATIO);
      }
      if (f <= 0.0 || f > 1.0) {
        std::cerr
            << "[SERVER CONFIG WARN] lru-trigger-ratio must be in (0, 1]. "
               "Invalid value will be treated as default "
            << CONFIG_DEFAULT_LRU_TRIGGER_RATIO << '\n';
        f = std::max(0.0, f);
        f = std::min(1.0, f);
        if (f == 0.0f) {
          f = CONFIG_DEFAULT_LRU_TRIGGER_RATIO;
        }
      }
      lru_trigger_ratio_ = f;
      DISPLAY_CONFIG(key, lru_trigger_ratio_);
    } else if (key == "max-memory-limit") {
      size_t num = std::stoll(value);
      max_memory_limit_ = num;
      DISPLAY_CONFIG(key, max_memory_limit_);
    } else if (key == "keepalive-interval") {
      int interval = CONFIG_DEFAULT_KEEPALIVE_INTERVAL;
      if (!CanConvertToInt32(value, interval)) {
        DISPLAY_INVALID_WARN(key, CONFIG_DEFAULT_KEEPALIVE_INTERVAL);
      }
      if (interval < 30 || interval > 100) {
        std::cerr << "[SERVER CONFIG WARN] keepalive-interval must be in [30, "
                     "100]. Value of "
                  << interval << " will be treated as default value "
                  << CONFIG_DEFAULT_KEEPALIVE_INTERVAL << '\n';
        interval = CONFIG_DEFAULT_KEEPALIVE_INTERVAL;
      }
      keepalive_interval_ = interval;
      DISPLAY_CONFIG(key, keepalive_interval_);
    } else if (key == "keepalive-cnt") {
      int cnt = CONFIG_DEFAULT_KEEPALIVE_CNT;
      if (!CanConvertToInt32(value, cnt)) {
        DISPLAY_INVALID_WARN(key, CONFIG_DEFAULT_KEEPALIVE_CNT);
      }
      if (cnt < 1 || cnt > 15) {
        std::cerr << "[SERVER CONFIG WARN] keealive-cnt must be in [1, 15]. "
                  << "Value of " << cnt << " will be treated as defalut value "
                  << CONFIG_DEFAULT_KEEPALIVE_CNT << '\n';
        cnt = CONFIG_DEFAULT_KEEPALIVE_CNT;
      }
      keepalive_cnt_ = cnt;
      DISPLAY_CONFIG(key, keepalive_cnt_);
    } else {
      std::cout << "[SERVER CONFIG WARN] Config item [" << key
                << "] not recognized, skip..\n";
    }
  }
}
