#ifndef __ADDR_H__
#define __ADDR_H__

#include <memory>
#include <string>
#include <arpa/inet.h>


class Ipv4Addr {
public:
  typedef std::shared_ptr<Ipv4Addr> Ptr;

  Ipv4Addr();

//  explicit Ipv4Addr(uint32_t addr = INADDR_ANY, uint16_t port = 0);

  Ipv4Addr(const std::string &ip, uint16_t port);

  std::string ToString() const { return addr_str_ + ":" + std::to_string(port_); }

  uint16_t GetPort() const { return port_; }

  sockaddr *GetAddr() const { return (struct sockaddr *) &addr_; }

  socklen_t GetSockLen() const { return GetSockAddrLen(); }

  int GetFamily() const { return addr_.sin_family; }

  const struct sockaddr_in &GetSockAddrIn() const { return addr_; }

  socklen_t GetSockAddrLen() const { return sizeof(addr_); }

  void SyncPort();

private:
  void InitAddrStr();

private:
  std::string addr_str_ = "";
  uint16_t port_ = 0;
  struct sockaddr_in addr_;
};

#endif // __ADDR_H__