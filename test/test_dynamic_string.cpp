#include "../src/str.h"
#include <iostream>

using namespace std;

int main(int argc, char const *argv[]) {

  DynamicString str1("asd", 3);
  string stl_str = "C++JavaPython";
  DynamicString str2(stl_str);
  cout << str1 << endl;
  cout << str2 << endl;

  str1.Append(str2);
  cout << str1 << endl;
  cout << "str1.Length() = " << str1.Length() << ", str1.Allocated() = " << str1.Allocated() << endl;
  cout << "str2.Length() = " << str2.Length() << ", str2.Allocated() = " << str2.Allocated() << endl;
  str1.Append("hihi", 4);
  cout << "str1.Length() = " << str1.Length() << ", str1.Allocated() = " << str1.Allocated() << endl;

  for (size_t i = 0; i <= str1.Length(); i++) {
    if (str1[i] == '\0') {
      cout << "\nstr1 ends at " << i << endl;
    } else {
      cout << str1[i] << "|";
    }
  }

  DynamicString str3("ok\0okok", 7);
  for (size_t i = 0; i < str3.Length(); i++) {
    if (str3[i] == '\0') {
      cout << "~";
    } else {
      cout << str3[i];
    }
  }
  cout << endl;

  DynamicString str4("123456", 6);
  for (int i = -1; i >= -6; i--) {
    cout << str4[i] << "|";
  }
  cout << endl;

  // compare test
  DynamicString c1("hello", 5);
  DynamicString c2("hello", 5);

  if (c1 == c2) {
    cout << "c1 == c2\n";
  }

  DynamicString c3("ehllo", 5);
  if (c1 != c3) {
    cout << "c1 != c3\n";
  }
  c3.Clear();
  if (c3.Empty()) {
    cout << c3 << "is empty ||||\n";
  }

  // test reset string value
  c2.Reset("are you ok? I am really fine!!!");
  cout << "c2 = " << c2 << endl;
  cout << "c2.Length() = " << c2.Length() << ", c2.Allocated() = " << c2.Allocated() << endl;

  c2.Reset("okok");
  cout << "c2 = " << c2 << endl;
  cout << "c2.Length() = " << c2.Length() << ", c2.Allocated() = " << c2.Allocated() << endl;
  c2.Shrink();
  cout << "c2 = " << c2 << endl;
  cout << "c2.Length() = " << c2.Length() << ", c2.Allocated() = " << c2.Allocated() << endl;
  std::string cst2("monday");
  c2.Append(cst2);
  cout << "c2 = " << c2 << endl;
  cout << "c2.Length() = " << c2.Length() << ", c2.Allocated() = " << c2.Allocated() << endl;

  DynamicString cstr3;
  cstr3.Clear();
  cout << "Cleared empty str\n";
  cstr3.Reset("okok");
  cout << cstr3 << endl;
  cout << "cstr3.Length() = " << cstr3.Length() << ", cstr3.Allocated() = " << cstr3.Allocated() << endl;

  DynamicString cstr4;
  cstr4.Append("abc");
  cout << cstr4 << endl;
  cout << "cstr4.Length() = " << cstr4.Length() << ", cstr4.Allocated() = " << cstr4.Allocated() << endl;

  cstr4.Append("abcefg");
  cout << cstr4 << endl;
  cout << "cstr4.Length() = " << cstr4.Length() << ", cstr4.Allocated() = " << cstr4.Allocated() << endl;

  auto func = []() -> DynamicString
  {
    return DynamicString();
  };

  DynamicString cstr5;
  DynamicString cstr6 = func();
  cout << "cstr5 = " << cstr5 << endl;

  // test reset
  DynamicString ds1("10");
  cout << ds1 << endl;
  ds1.Clear();
  cout << ds1 << endl;
  ds1.Reset("changed");
  cout << ds1 << endl;

  // test empty string and null string
  // no memory allocated
  DynamicString empty;
  cout << empty.Length() << ", " << empty.Allocated() << endl;

  // memory allocated even for empty str
  DynamicString empty2("",0);
  cout << empty2.Length() << ", " << empty2.Allocated() << endl;

  // memory allocated even for empty str
  DynamicString empty3("");
  cout << empty3.Length() << ", " << empty3.Allocated() << endl;

  return 0;
}
