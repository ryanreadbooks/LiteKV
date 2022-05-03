#include "../src/net/server.h"

int main(int argc, char const *argv[]) {

  std::cout << "sizeof(EventLoop) = " << sizeof(EventLoop) << std::endl;
  std::cout << "sizeof(Server) = " << sizeof(Server) << std::endl;
  std::cout << "sizeof(Session) = " << sizeof(Session) << std::endl;
  std::cout << "sizeof(CommandCache) = " << sizeof(CommandCache) << std::endl;
  std::cout << "sizeof(Buffer) = " << sizeof(Buffer) << std::endl;
  std::cout << "sizeof(ProcFuncType) = " << sizeof(ProcFuncType) << std::endl;
  std::cout << "sizeof(std::string) = " << sizeof(std::string) << std::endl;
  std::cout << "sizeof(TimeEvent) = " << sizeof(TimeEvent) << std::endl;

  EventLoop loop;
  KVContainer container;
  Engine engine(&container);
  Server server(&loop, &engine, "127.0.0.1", 9527);
  loop.Loop();

  return 0;
}
