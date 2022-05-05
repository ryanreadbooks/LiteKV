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
    DynamicString line = buffer.ReadAndForward(CRLF);
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