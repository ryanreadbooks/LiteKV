import socket

# gen_command(cmd, key, [argv0, argv1, ...])
def gen_command(cmd, key, argv):
  cmd_len = len(cmd)
  if key is None and argv is None:
    command = f'*1\r\n${cmd_len}\r\n{cmd}\r\n'
    return command
  key_len = len(key)
  argv_len = len(argv)
  total_len = argv_len + 2
  command = f'*{total_len}\r\n${cmd_len}\r\n{cmd}\r\n${key_len}\r\n{key}\r\n'
  for item in argv:
    command += f'${len(item)}\r\n{item}\r\n'

  return command

if __name__ == '__main__':
  s = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
  s.connect(("127.0.0.1", 9527))
  while True:
    inputcmd = input()
    if len(inputcmd) == 1 and inputcmd[0] == 'q':
      break;
    cmds = inputcmd.split(' ')
    if len(cmds) == 1:
      data = gen_command(cmds[0], None, None)
    elif len(cmds) == 2:
      data = gen_command(cmds[0], cmds[1], [])
    else:  
      data = gen_command(cmds[0], cmds[1], cmds[2:])

    # data = gen_command('a', 'b', ['c']) + gen_command('e', 'f', ['1', 'q'])
    toserver = bytes(data, encoding='utf8')
    # print(toserver, f'len = {len(toserver)}')
    s.sendall(toserver)
    recv_msg = s.recv(4096)
    print(f'Server response = {recv_msg}')
    # input()
  
  s.close()