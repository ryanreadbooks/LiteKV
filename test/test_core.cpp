#include <iostream>
#include "../src/core.h"

using namespace std;

int main(int argc, char const *argv[])
{

  KVContainer engine;
  cout << "sizeof(Key) = " << sizeof(Key) << endl;
  cout << "sizeof(ValueObject) = " << sizeof(ValueObject) << endl;
  engine.SetInt("int1", 15260);

  int errcode;
  ValueObjectPtr p1 = engine.Get("int1", errcode);
  if (errcode == kOkCode) {
    cout << reinterpret_cast<int64_t>(p1->ptr) << endl;
  }

  engine.SetInt("int2", -52);
  engine.SetInt("int3", 50);
  engine.SetInt("int1", 102);
  engine.SetInt("int4", 1529656622102);
  
  p1 = engine.Get("int1", errcode);
  cout << reinterpret_cast<int64_t>(p1->ptr) << endl;

  p1 = engine.Get("int2", errcode);
  cout << reinterpret_cast<int64_t>(p1->ptr) << endl;

  p1 = engine.Get("int3", errcode);
  cout << reinterpret_cast<int64_t>(p1->ptr) << endl;

  p1 = engine.Get("int4", errcode);
  cout << reinterpret_cast<int64_t>(p1->ptr) << endl;
  p1 = engine.Get("int4", errcode);
  cout << reinterpret_cast<int64_t>(p1->ptr) << endl;

  // set string
  engine.SetString("str1", "hello");
  engine.SetString("str2", "world");
  engine.SetString("str3", "okok");
  engine.SetString("str4", "s11s12");

  ValueObjectPtr p2 = engine.Get("str1", errcode);
  if (p2 && errcode == kOkCode) {
    cout << p2->ToStdString() << endl;
  }
  p2 = engine.Get("str2", errcode);
  if (p2 && errcode == kOkCode) {
    cout << p2->ToStdString() << endl;
  }
  p2 = engine.Get("str3", errcode);
  if (p2 && errcode == kOkCode) {
    cout << p2->ToStdString() << endl;
  }
  p2 = engine.Get("str4", errcode);
  if (p2 && errcode == kOkCode) {
    cout << p2->ToStdString() << endl;
  }

  engine.SetString("str4", "wonderful");
  cout << "New str4 value = ";
  p2 = engine.Get("str4", errcode);
  if (p2 && errcode == kOkCode) {
    cout << p2->ToStdString() << endl;
  }

  engine.SetInt("str4", 1100869); // set existing key of different type
  p2 = engine.Get("str4", errcode);
  cout << "changing str4(string) to str4(int), get value = " << p2->ToInt64() << endl;

  engine.SetString("int4", "changing from int4!!!!");
  p2 = engine.Get("int4", errcode);
  cout << "changing from int4(int), now is string, value = ";
  cout << p2->ToStdString() << endl;

  cout << "End of test\n";
  
  return 0;
}
