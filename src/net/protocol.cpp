#include "protocol.h"

void AuxiliaryReadProcCleanup(Buffer &buffer, CommandCache &cache, size_t begin_idx, int nbytes) {
  /* clear cache and clean up invalid request in buffer */
  cache.Clear();
  buffer.Reset(); /* FIXME, should only discard invalid bytes and reserve those valid request bytes */
}

bool AuxiliaryReadProc(Buffer &buffer, CommandCache &cache, int nbytes) {
  /* Helper function to parse request command */
  size_t begin_idx = buffer.BeginReadIdx();
  while (buffer.ReadableBytes() > 0 && cache.argc > cache.argv.size()) {
    DynamicString line = buffer.ReadAndForwardTill(CRLF);
    if (line.Empty()) {
      /* can not form an whole item, wait for more data coming in */
      break;
    }
    if (line[0] != '$') {
      /* command parsing error, end parsing and reply immediately and clear cache and buffer */
      AuxiliaryReadProcCleanup(buffer, cache, begin_idx, nbytes);
      return false;
    }
    char *p_end;
    size_t arg_len = strtol(line.Data() + 1, &p_end, 10);
    if (buffer.ReadableBytes() < arg_len + 2) {
      /* can not read arg_len args, wait until next time */
      buffer.ReaderIdxBackward(line.Length() + 2);
      break;
    }
    std::string arg = buffer.ReadStdStringAndForward(arg_len);
    if (buffer.ReadStdString(2) != CRLF) {
      AuxiliaryReadProcCleanup(buffer, cache, begin_idx, nbytes);
      return false;
    }
    buffer.ReaderIdxForward(2);
    cache.argv.emplace_back(arg);
  }
  return true;
}

bool TryParseFromBuffer(Buffer &buffer, CommandCache &cache, bool &err) {
  if (buffer.ReadableBytes() <= 0) {
    return false;
  }
  /* a whole protocol must start from * */
  if (buffer.ReadStdString(1) != "*") {
    if (buffer.ReadableBytes() >= 1) {  /* have enough bytes, but can not find '*', invalid */
      err = true;
    }
    buffer.Reset();
    return false;
  }
  size_t step;
  size_t cur_idx = 1;
  long n_para = buffer.ReadLongFrom(cur_idx, step);
  if (step == 0) {
    if (buffer.ReadableBytes() > 0) {
      err = true;
    }
    cache.Clear();
    return false; /* protocol string not complete, till next time */
  }
  cache.argc = (size_t) n_para;
  /* next two bytes is \r\n */
  cur_idx += step;
  if (buffer.ReadStdStringFrom(cur_idx, 2) != "\r\n") {
    if (buffer.ReadableBytes() - cur_idx >= 2) {
      err = true;
    }
    cache.Clear();
    return false;
  }
  cur_idx += 2;
  while (n_para > 0) {
    if (cur_idx >= buffer.ReadableBytes()) {  /* not enough data */
      return false;
    }
    /* next is $ */
    if (buffer.ReadableCharacterAt(cur_idx) != '$') {
      if (buffer.ReadableBytes() > cur_idx) {
        err = true;
      }
      cache.Clear();
      return false;
    }
    ++cur_idx;
    long next_arg_len = buffer.ReadLongFrom(cur_idx, step);
    if (step == 0) {
      if (buffer.ReadableBytes() - cur_idx > 0) {
        err = true;
      }
      cache.Clear();
      return false;
    }
    cur_idx += step;
    /* next two bytes is \r\n */
    if (buffer.ReadStdStringFrom(cur_idx, 2) != "\r\n") {
      if (buffer.ReadableBytes() - cur_idx >= 2) {
        err = true;
      }
      cache.Clear();
      return false;
    }
    cur_idx += 2;
    /* read next_arg_len consecutive bytes */
    std::string arg = buffer.ReadStdStringFrom(cur_idx, next_arg_len);
    cache.argv.emplace_back(arg);
//    std::cout << arg << std::endl;
    /* arg might be empty and this is allowed */
    if (arg.size() != (size_t) next_arg_len) {
      if (buffer.ReadableBytes() - cur_idx >= (size_t) next_arg_len) {
        err = true;
      }
      cache.Clear();
      return false;
    }
    cur_idx += next_arg_len;
    /* next two bytes is \r\n */
    if (buffer.ReadStdStringFrom(cur_idx, 2) != "\r\n") {
      if (buffer.ReadableBytes() - cur_idx >= 2) {
        err = true;
      }
      cache.Clear();
      return false;
    }
    cur_idx += 2;
    --n_para;
  }
  /* finish parsing one */
  cache.inited = true;
  if (cache.argc != cache.argv.size()) {
    cache.Clear();
    return false;
  }
  buffer.ReaderIdxForward(cur_idx);
  return true;
}