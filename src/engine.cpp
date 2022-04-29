#include <cassert>
#include "engine.h"

std::string Engine::HandleCommand(const CommandCache& cmds) {
  size_t argc = cmds.argc;
  const std::vector<std::string> &argv = cmds.argv;
  if (argc < 2) { /* not allowed */
    /* TODO argc at least is 2 */
  }
  assert(argc == argv.size());

  /* Commands contents
  *  +------------+----------+-------------------------+
  *  |   argv[0]  |  argv[1] |   argv[2] ... argv[n]   |
  *  +------------+----------+-------------------------+
  *  |   opcode   |   key    |         operands        |
  *  +------------+----------+-------------------------+
  */
  std::string opcode = argv[0];
  if (!OpCodeValid(opcode)) {
    /* TODO invalid operator code */
  }
  std::string key = argv[1];

  return "";
}

void Engine::HandleCommand(const CommandCache &cmds, Buffer &out_buf) {

}

bool Engine::OpCodeValid(const std::string& opcode) const {
//  static std::unordered_map<>
  return false;
}
