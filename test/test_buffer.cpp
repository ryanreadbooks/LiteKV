#include <iostream>
#include "../src/net/buffer.h"

using namespace std;

int main(int argc, char **argv) {

  auto show_buffer = [](Buffer &s) {
    for (auto &c : s.ReadableAsString()) {
      if (c == '\r') {
        std::cout << "\\r";
      } else if (c == '\n') {
        std::cout << "\\n";
      } else {
        std::cout << c;
      }
    }
    cout << endl;
  };

//  Buffer buf(8);
//
//  std::string s5 = "hell0";
//  buf.Append(s5);
//  std::string s6 = "wonder";
//  buf.Append(s6);
//
//  char ch7[7] = {'1', '2', '3', '4', '5', '6', '7'};
//
//  show_buffer(buf);
//
//  buf.Append(ch7, 7);
//
//  show_buffer(buf);
//
//  buf.ReaderIdxForward(14);
//  show_buffer(buf);
//
//  string s7 = "qwertyu";
//
//  buf.Append(s7);
//  show_buffer(buf);
//
//  cout << "-------------------------------\n";
//  Buffer buf2(10);
//  for (int i = 0; i < 6; i++) {
//    char buf[6] = {'1', '2', '3', '4', '5', '6'};
//    buf2.Append(buf, 6);
//    cout << "i = " << i << ": ";
//    show_buffer(buf2);
//  }

  cout << "-------------------------------\n";
  Buffer buf3(10);
  for (int i = 0; i < 5; i++) {
    char buf[5] = {'1', '2', '3', '4', '5'};
    buf3.Append(buf, 5);
    show_buffer(buf3);
    buf3.ReaderIdxForward(3);
  }

  return 0;
}