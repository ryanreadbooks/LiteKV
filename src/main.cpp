#include <signal.h>
#include "config.h"
#include "net/server.h"
#ifdef TCMALLOC_FOUND
#include <gperftools/malloc_extension.h>
#include <gperftools/heap-profiler.h>
#endif

AppendableFile *history;

void SigIntHandler(int sig_num) {
  auto t = time(nullptr);
  printf("signal-handler(%lu). Received SIGINT(%d), shutting down...\n", t, sig_num);
  delete history;
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
#ifdef TCMALLOC_FOUND
  MallocExtension::Initialize();
  std::cout << "Tcmalloc Initialized.\n";
#endif
  std::cout << "Server starting...\n";
  /* sigint handler */
  struct sigaction new_act{}, old_act{};
  new_act.sa_handler = SigIntHandler;
  sigemptyset(&new_act.sa_mask);
  new_act.sa_flags = 0;
  int ret = sigaction(SIGINT, &new_act, &old_act);
  if (ret == -1) {
    std::cerr << "Can not register handler for SIGINT (errno=" << errno << ")\n";
    exit(EXIT_FAILURE);
  }

  /* Load config */
  std::string default_conf_filename = "../conf/litekv.conf";
  if (argc >= 2) {
    default_conf_filename = argv[1];
  }
  Config configs(default_conf_filename);

  EventLoop loop;
  KVContainer container;
  Engine engine(&container, &configs);
  std::string location = configs.GetDumpFilename();
  size_t cache_size = configs.GetDumpCacheSize();
  history = new AppendableFile(location, cache_size);

  auto begin = GetCurrentMs();
  engine.RestoreFromAppendableFile(&loop, history);
  auto spent = GetCurrentMs() - begin;
  std::cout << "DB loaded in " << spent << " ms.. " << std::endl;
  Server server(&loop, &engine, configs.GetIp(), configs.GetPort());

  std::cout << "The server is now ready to accept connections on port " << configs.GetPort() << std::endl;

  loop.Loop();

  return 0;
}