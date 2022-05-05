#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <cstring>
#include <vector>
#include <cstdint>
#include <string>

#include "../str.h"

static const char *CRLF = "\r\n";

class Buffer {
public:
  explicit Buffer(size_t init_bufsize = 64) :
    data_(init_bufsize), p_reader_(0), p_writer_(0) {
  }

  ~Buffer() {
    data_.clear();
  }

  inline size_t Capacity() const { return data_.capacity(); }

  inline size_t BeginReadIdx() const { return p_reader_; }

  inline size_t BeginWriteIdx() const { return p_writer_; }

  inline size_t WritableBytes() const { return Capacity() - p_writer_; }

  inline size_t ReadableBytes() const { return p_writer_ - p_reader_; }

  inline size_t PrependableBytes() const { return p_reader_; }

  inline char *BufferFront() { return &data_[0]; }
  
  std::string ReadableAsString() const { return std::string(data_.data() + p_reader_, ReadableBytes()); }

  DynamicString ReadableAsDynaString() const {return DynamicString(data_.data() + p_reader_, ReadableBytes()); }

  inline char *BeginRead() { return data_.data() + p_reader_; }

  inline char *BeginWrite() { return data_.data() + p_writer_; }

  void MoveReadableToHead() {
    memcpy(data_.data(), data_.data() + p_reader_, ReadableBytes());
    size_t can_free = PrependableBytes();
    memset(data_.data() + p_writer_ - can_free, 0, can_free);
    /* update pointer */
    p_reader_ = 0;
    p_writer_ = p_writer_ - can_free;
  }

  void Append(const std::string &value);

  void Append(const char *value, int len);

  int FindCRLFInReadable();

  void Reset();

  /* pointer to left */
  void ReaderIdxForward(size_t len);

  /* pointer to right */
  void ReaderIdxBackward(size_t len);

  std::string ReadStdStringAndForward(size_t len);

  std::string ReadStdString(size_t len);

  std::string ReadStdStringAndForward(const char* delim = "\r\n");

  DynamicString ReadDynaStringAndForward(size_t len);

  DynamicString ReadDynaString(size_t len);

  DynamicString ReadAndForward(const char *delim = "\r\n");

  long ReadLongAndForward(size_t& step);

private:
  void EnsureBytesForWrite(size_t n);

private:
  std::vector<char> data_;
  size_t p_reader_;
  size_t p_writer_;
};

#endif // __BUFFER_H__