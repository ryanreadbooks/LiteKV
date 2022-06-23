//
// Created by ryan on 2022/6/21.
// TODO: Not finished yet: liteKV-benchmark is for benchmarking the litekv server
// Can use redis-benchmark tool in Redis instead
//

#include <iostream>
#include <thread>
#include <sstream>
#include <vector>
#include <unordered_map>

#include "../src/net/addr.h"
#include "../src/net/time_event.h"

// global config for testing
struct BenchMarkConfig {
  // host of the server, default 127.0.0.1
  std::string host = "127.0.0.1";
  // port of the server, default 9527
  size_t port = 9527;
  // number of clients that run in parallel, default 50
  size_t n_clients = 50;
  // total number of request per client, default 100000
  size_t n_requests = 100000;
  // length of key to append to default key, default 10
  size_t keyspace_len = 10;
  // commands for testing, default is empty (benchmark default commands)
  std::vector<std::string> testing_cmds = {};
  std::string testing_cmds_str = "";

  std::string ToString() const {
    std::stringstream ss;
    ss << "BenchMarkConfig: \n"
       << "==> host: " << host << std::endl
       << "==> port: " << port << std::endl
       << "==> n_clients: " << n_clients << std::endl
       << "==> n_requests: " << n_requests << std::endl
       << "==> keyspace_len: " << keyspace_len << std::endl
       << "==> testing_cmds: " << testing_cmds_str << std::endl;
    return ss.str();
  }
} g_config;


/**
 * launch one client to do requests
 */
static void LaunchClient() {

}

/**
 * parse arguments and initialize g_config object
 * @param argc
 * @param argv
 */
static void ParseArgs(int argc, char** argv) {
  int i;
  bool last;
  if (argc == 1) {
    // use default
  } else {
    for (i = 1; i < argc; ++i) {
      last = (i == (argc - 1));
      std::string arg = std::string(argv[i]);
      if (arg == "-h") {
        if (last) goto invalid;
        g_config.host = std::string(argv[++i]);
      } else if (arg == "-p") {
        if (last) goto invalid;
        g_config.port = std::atol(argv[++i]);
      } else if (arg == "-c") {
        if (last) goto invalid;
        g_config.n_clients = std::atol(argv[++i]);
      } else if (arg == "-n") {
        if (last) goto invalid;
        g_config.n_requests = std::atol(argv[++i]);
      } else if (arg == "-k") {
        if (last) goto invalid;
        g_config.keyspace_len = std::atol(argv[++i]);
      } else if (arg == "-t") {
        if (last) goto invalid;
        // separate using commas
        std::string tests_cmds = argv[++i];
        std::stringstream ss(tests_cmds);
        std::string tmp;
        while (std::getline(ss, tmp, ',')) {
          if (!tmp.empty()) {
            g_config.testing_cmds.emplace_back(tmp);
          }
        }
        g_config.testing_cmds_str = tests_cmds;
      }
    }
  }
  return;

invalid:
  std::cerr << "Invalid argument '" << argv[i] << "'\n";

usage:
  std::cout << "Usage: litekv-benchmark [-h <host>] [-p <port>] [-c <clients>] [-n <requests>] [-k <keyspace length>] [-t <testing commands, comma separated>]\n";
}

int main(int argc, char **argv) {
  ParseArgs(argc, argv);
  std::cout << g_config.ToString();
  return 0;
}