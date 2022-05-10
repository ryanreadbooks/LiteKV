#include "../src/net/protocol.h"

int main(int argc, char **argv) {
  {
    Buffer buf;
    std::string ps = "*36\r\n$4\r\n";

    buf.Append(ps);
    buf.ReaderIdxForward(1);

    std::cout << buf.ReadableAsString() << std::endl;
    size_t step;
    std::cout << buf.ReadLong(step) << std::endl;
    std::cout << step << std::endl;
    std::cout << "------\n";
  }

  {
    Buffer buf;
    std::string ps = "*36\r\n$4\r\n";

    buf.Append(ps);

    std::cout << buf.ReadableAsString() << std::endl;
    size_t step;
    std::cout << buf.ReadLongFrom(1, step) << std::endl;
    std::cout << step << std::endl;
    std::cout << "------\n";
  }

  {
    std::string s = "*3\r\n$3\r\nset\r\n$2\r\nab\r\n$3\r\ndce\r\n";
    Buffer buf;
    buf.Append(s);
    CommandCache cache;
    bool err;
    TryParseFromBuffer(buf, cache, err);
    std::cout << cache;
    std::cout << "------\n";
  }

  return 0;
}