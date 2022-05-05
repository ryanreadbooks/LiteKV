#include <signal.h>
#include "net/server.h"

AppendableFile *history;

void SigIntHandler(int sig_num) {
  auto t = time(nullptr);
  printf("signal-handler(%lu). Received SIGINT(%d), shutting down...\n", t, sig_num);
  delete history;
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
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

  EventLoop loop;
  KVContainer container;
  Engine engine(&container);
  std::string location = "test.aof";
  size_t cache_size = 1024;
  history = new AppendableFile(location, cache_size);

  auto begin = GetCurrentMs();
  engine.RestoreFromAppendableFile(&loop, history);
  auto spent = GetCurrentMs() - begin;
  std::cout << "DB loaded in " << spent << " ms.. " << std::endl;
  Server server(&loop, &engine, "127.0.0.1", 9527);
  loop.Loop();

  return 0;
}