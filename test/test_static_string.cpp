#include <iostream>
#include <array>

#include "../str.h"

int main(int argc, char const *argv[])
{
  StaticString s("hello world");
  std::string s2 = "wonder";
  StaticString fs2(s2);
  std::cout << s.Length() << std::endl;
  std::cout << s << std::endl;
  std::cout << fs2.Length() << std::endl;
  std::cout << fs2 << std::endl;
  StaticString fs3("hello world");
  std::cout << std::boolalpha << (fs3 == s) << std::endl;

  StaticString fs4 = fs2;

  std::cout << fs4.ToStdString() << std::endl;

  fs4 = fs3;
  std::cout << fs4.ToStdString() << std::endl;

  std::cout << "sizeof(FixedSizeString)=" << sizeof(StaticString) << std::endl;

  std::cout << "sizeof(array)=" << sizeof(std::array<int, 16>) << std::endl; // 4 * 16 = 64

  return 0;
}
