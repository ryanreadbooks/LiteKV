#include <algorithm>
#include "config.h"
#include "str.h"

Config::Config(std::string filename) :filename_(std::move(filename)) {
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
    while (!ifs.eof()) {
      ifs >> first;
      ifs >> second;
      std::transform(first.begin(), first.end(), first.begin(), ::tolower);
      std::transform(second.begin(), second.end(), second.begin(), ::tolower);
      configs[first] = second;
    }
    Init(configs);
    std::cout << "Config loaded in " << filename_ << std::endl;
    ifs.close();
  } else {
    std::cerr << "Can not open config file in " << filename_ << std::endl;
  }
}

void Config::Init(std::unordered_map<std::string, std::string>& configs) {
  for (auto & config : configs) {
    const std::string& key = config.first;
    const std::string& value = config.second;
    if (key == "ip") {
      ip_ = value;
    } else if (key == "port") {
      if (!CanConvertToInt32(value, port_)) {
        std::cerr << "port invalid, use default port 9527 instead\n";
        port_ = 9527;
      }
    } else if (key == "dumpfile") {
      dumpfile_ = value;
    } else if (key == "dump-cachesize") {
      if (!CanConvertToInt32(value, dump_cachesize_)) {
        std::cerr << "dump-cachesize invalid, use default value 1024 instead\n";
        dump_cachesize_ = 1024;
      }
    }
  }
}
