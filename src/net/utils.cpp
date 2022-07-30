#include "utils.h"

int ReadToBuf(int fd, char *buf, int len) {
  int total_read = 0;
  int bytes_read = 0;
  memset(buf, 0, len);
  while (total_read < len) {
    bytes_read = ::read(fd, buf + bytes_read, len - total_read);
    if (bytes_read > 0) { /* read normally */
      total_read += bytes_read;
    } else if (bytes_read == 0) { /* remote peer closed */
      break;
    } else if (bytes_read == -1) { /* error */
      if (errno == EINTR) {
        continue;
      } else {
        /* errno == EAGAIN or errno == EWOULDBLOCK means can read no more */
        break;
      }
    }
  }
  return total_read;
}

int WriteFromBuf(int fd, const char *buf, int len) {
  int total_written = 0;
  int bytes_written = 0;
  while (total_written < len) {
    bytes_written = ::send(fd, buf + bytes_written, len - total_written, 0);
    if (bytes_written > 0) {
      total_written += bytes_written;
    } else if (bytes_written == -1) { /* error */
      if (errno == EINTR) {
        continue;
      } else {
        break;
      }
    }
  }
  return total_written;
}

int SetFdNonBlock(int fd) {
  int flags = fcntl(fd, F_GETFL);
  if (flags == -1) {
    std::cerr << "fcntl(F_GETFL): " << strerror(errno) << std::endl;
    return ERR;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    std::cerr << "fcntl(F_SETFL, O_NONBLOCK): " << strerror(errno) << std::endl;
    return ERR;
  }
  return OK;
}

int SetTcpNoDelayOption(int fd, int val) {
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1) {
    std::cerr << "setsockopt TCP_NODELAY: " << strerror(errno) << std::endl;
    return ERR;
  }
  return OK;
}

int EnableTcpNoDelay(int fd) {
  return SetTcpNoDelayOption(fd, 1);
}

int DisableTcpNoDelay(int fd) {
  return SetTcpNoDelayOption(fd, 0);
}

int SetReuseAddress(int fd) {
  int val = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) == -1) {
    std::cerr << "setsockopt SO_REUSEADDR: " << strerror(errno) << std::endl;
    return ERR;
  }
  return OK;
}

int SetReusePort(int fd) {
  int val = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) == -1) {
    std::cerr << "setsockopt SO_REUSEPORT: " << strerror(errno) << std::endl;
    return ERR;
  }
  return OK;
}

int SetKeepAlive(int fd, int idle, int interal, int cnt) {
  int val = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1) {
    std::cerr << "setsockopt SO_KEEPALIVE: " << strerror(errno) << std::endl;
    return ERR;
  }
  val = idle;
  if (setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &val, sizeof(val)) == -1) {
    std::cerr << "setsockopt TCP_KEEPIDLE: " << strerror(errno) << std::endl;
    return ERR;
  }
  val = interal;
  if (setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &val, sizeof(val)) == -1) {
    std::cerr << "setsockopt TCP_KEEPINTVL: " << strerror(errno) << std::endl;
    return ERR;
  }
  val = cnt;
  if (setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &val, sizeof(val)) == -1) {
    std::cerr << "setsockopt TCP_KEEPCNT: " << strerror(errno) << std::endl;
    return ERR;
  }
  return OK;
}
