import argparse
from pickle import TRUE
import socket
from xmlrpc.client import TRANSPORT_ERROR

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

arg_parser = argparse.ArgumentParser(description='SimpleKV simple client')
arg_parser.add_argument('-a', '--address', type=str, default='127.0.0.1', dest='ip', help='The ip address of the database')
arg_parser.add_argument('-p', '--port', type=int, default=9527, dest='port', help='The port of the database')

args = arg_parser.parse_args()

if __name__ == '__main__':
  connected = False;
  ip = args.ip
  port = args.port
  s = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
  s.connect((ip, port))
  while True:
    print(f'{ip}:{port}> ', end='')
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
    s.sendall(toserver)
    recv_msg = s.recv(8192)
    print(f'Server response = {recv_msg}')
  
  s.close()