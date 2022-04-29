#ifndef __UTILS_H__
#define __UTILS_H__

#include <iostream>
#include <cstdint>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <endian.h>


const static int OK = 0;
const static int ERR = -1;

int ReadToBuf(int fd, char *buf, int len);

int WriteFromBuf(int fd, const char *buf, int len);

int SetFdNonBlock(int fd);

int SetTcpNoDelayOption(int fd, int val);

int EnableTcpNoDelay(int fd);

int DisableTcpNoDelay(int fd);

int SetReuseAddress(int fd);

int SetReusePort(int fd);

static uint16_t HostToNet16(uint16_t host16) {
  return htobe16(host16);
}

static uint32_t HostToNet32(uint32_t host32) {
  return htobe32(host32);
}

static uint64_t HostToNet64(uint64_t host) {
  return htobe64(host);
}

static uint16_t NetToHost16(uint16_t net16) {
  return be16toh(net16);
}

static uint32_t NetToHost32(uint32_t net32) {
  return be32toh(net32);
}

static uint32_t NetToHost64(uint64_t net64) {
  return be64toh(net64);
}


#endif // __UTILS_H__