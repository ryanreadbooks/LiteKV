#include <algorithm>
#include "buffer.h"

void Buffer::Append(const std::string& value) {
  Append(value.data(), value.size());
}

void Buffer::Append(const char* value, int len) {
  EnsureBytesForWrite(len);
  /* append value to writable */
  memcpy(BeginWrite(), value, len);
  /* update p_writer */
  p_writer_ += len;
}

void Buffer::Reset() {
  memset(data_.data(), 0, data_.capacity());
  p_reader_ = 0;
  p_writer_ = 0;
}

void Buffer::EnsureBytesForWrite(size_t n) {
  if (WritableBytes() < n) {
    size_t cur_free_space = PrependableBytes() + WritableBytes();
    if (cur_free_space >= n) {
      MoveReadableToHead();
    } else {
      /* need to alloc more space */
      data_.resize(p_writer_ + n + 2);
    }
  }
}

void Buffer::ReaderIdxForward(size_t len) {
  len = std::min(len, ReadableBytes());
  p_reader_ += len;
}

void Buffer::ReaderIdxBackward(size_t len) {
  len = std::min(len, PrependableBytes());
  p_reader_ -= len;
}

int Buffer::FindCRLFInReadable() {
  auto it_start = data_.begin() + p_reader_;
  auto it_end = data_.begin() + p_writer_;
  auto it = std::search(it_start, it_end,
                        CRLF, CRLF + 2);
  return (it == it_end) ? -1 : (it - it_start); /* offset with respect to p_reader_ */
}

std::string Buffer::ReadStdStringAndForward(size_t len) {
  if (len > ReadableBytes()) {
    return "";
  }
  std::string ans(BeginRead(), len);
  ReaderIdxForward(len);
  return ans;
}

std::string Buffer::ReadStdStringAndForward(const char *delim) {
  auto it_begin = data_.begin() + p_reader_;
  auto it_end = data_.begin() + p_writer_;
  size_t delim_len = strlen(delim);
  auto it = std::search(it_begin, it_end,
                        delim, delim + delim_len);
  if (it == it_end) {
    return "";
  }
  const char *begin_read = BeginRead();
  size_t offset = it - it_begin;
  ReaderIdxForward(offset + delim_len);
  return std::string(begin_read, offset);
}

std::string Buffer::ReadStdString(size_t len) {
  if (len > ReadableBytes()) {
    return "";
  }
  return std::string(BeginRead(), len);
}

DynamicString Buffer::ReadDynaStringAndForward(size_t len) {
  if (len > ReadableBytes()) {
    return DynamicString();
  }
  DynamicString ans(BeginRead(), len);
  ReaderIdxForward(len);
  return ans;
}

DynamicString Buffer::ReadDynaString(size_t len) {
  if (len > ReadableBytes()) {
    return DynamicString();
  }
  return DynamicString(BeginRead(), len);
}

DynamicString Buffer::ReadAndForward(const char* delim) {
  auto it_begin = data_.begin() + p_reader_;
  auto it_end = data_.begin() + p_writer_;
  size_t delim_len = strlen(delim);
  auto it = std::search(it_begin, it_end,
                        delim, delim + delim_len);
  if (it == it_end) { /* can not find */
    return DynamicString();
  }
  const char *begin_read = BeginRead();
  size_t offset = it - it_begin;
  ReaderIdxForward(offset + delim_len);
  return DynamicString(begin_read, offset);
}

long Buffer::ReadLongAndForward(size_t& step) {
  /* convert readable bytes to int till not digit */
  char *p_end;
  long num = strtoll(BeginRead(), &p_end, 10);
  step = p_end - BeginRead();
  ReaderIdxForward(step);
  return num;
}