#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "core.h"
#include "net/buffer.h"
#include "net/net.h"

class Engine {
public:
  explicit Engine(KVContainer* container): container_(container) {};
  ~Engine() {};

  std::string HandleCommand(const CommandCache& cmds);

  void HandleCommand(const CommandCache &cmds, Buffer &out_buf);

  bool OpCodeValid(const std::string& opcode) const;

private:
  KVContainer* container_;
};

# endif // __ENGINE_H__