import argparse
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
    command = f'*{total_len}\r\n'
    command += f'${cmd_len}\r\n{cmd}\r\n'
    command += f'${key_len}\r\n{key}\r\n'
    for item in argv:
        if item == "":
            command += f"$0\r\n\r\n"
        else:
            command += f'${len(item)}\r\n{item}\r\n'

    return command


def decode_reply(data):
  try:
    msg = data.decode(encoding='utf8')
    if msg.startswith('+'):
        # info string
        return msg[1:].splitlines()[0]
    elif msg.startswith('-'):
        # error
        return '(error) ' + msg[1:].splitlines()[0]
    elif msg.startswith(':'):
        # integer
        return '(integer) ' + msg[1:].splitlines()[0]
    elif msg.startswith('$'):
        # value string
        if msg == '$-1\r\n':
          return '(nil)'
        return f"\"{msg[1:].splitlines()[1]}\""
    elif msg.startswith('*'):
        if msg == '*0\r\n':
          return '(empty list)'
        # array
        resp = msg[1:].splitlines()
        ans = []
        res_len = int(resp[0])
        idx = 1
        n_cur_parsed = 0
        parsed_arr = ""
        while idx < len(resp) and n_cur_parsed < res_len:
          if resp[idx] != '$-1':
            ans.append(resp[idx + 1])
            parsed_arr += f"{n_cur_parsed + 1}) \"{resp[idx + 1]}\""
            if n_cur_parsed != res_len - 1:
              parsed_arr += '\n'
            idx += 2
            n_cur_parsed += 1
          elif resp[idx] == '$-1':
            ans.append('(nil)')
            parsed_arr += f"{n_cur_parsed + 1}) (nil)"
            if n_cur_parsed != res_len - 1:
              parsed_arr += '\n'
            idx += 1
            n_cur_parsed += 1
        return parsed_arr
    else:
        return "Response unrecognized!"
  except:
    return "Response unrecognized!"


def process_commands(data: str):
    data = data.strip()
    fsinglequote_idx = data.find("'")
    fquote_idx = data.find("\"")
    if fsinglequote_idx == -1 and fquote_idx == -1:
        # normal split will work
        tokens = []
        for i in data.split(' '):
            item = i.strip()
            if len(item) != 0:
                tokens.append(item)
        return tokens

    idx = 0
    tokens = list()
    next_idx = -1
    while idx < len(data):
        while idx < len(data) and data[idx] != "'" and data[idx] != "\"":
            idx += 1
        token = data[next_idx + 1: idx]
        if len(token) != 0 and token != "" and not token.isspace():
            inner = token.strip().split(' ')
            tokens.extend(inner)

        if idx >= len(data):
            break

        if data[idx] == "'":
            merge_flag = False
            if idx > 1 and data[idx - 1] != ' ':
                merge_flag = True
            next_idx = data.find("'", idx + 1)
            if next_idx == -1:
                return None
            if not merge_flag:
                tokens.append(data[idx + 1: next_idx])
            else:
                tokens[-1] += data[idx + 1: next_idx]
            idx = next_idx + 1
        elif data[idx] == "\"":
            merge_flag = False
            if idx > 1 and data[idx - 1] != ' ':
                merge_flag = True
            next_idx = data.find("\"", idx + 1)
            if next_idx == -1:
                return None
            if not merge_flag:
                tokens.append(data[idx + 1: next_idx])
            else:
                tokens[-1] += data[idx + 1: next_idx]
            idx = next_idx + 1

    return tokens


arg_parser = argparse.ArgumentParser(description='LiteKV simple client')
arg_parser.add_argument('-a', '--address', type=str, default='127.0.0.1', dest='ip', help='The ip address of the database')
arg_parser.add_argument('-p', '--port', type=int, default=9527, dest='port', help='The port of the database')
arg_parser.add_argument('-r', '--raw', action='store_true', default=False, dest='raw', help='Show raw response from server')
arg_parser.add_argument('-d', '--debug', action='store_true', default=False, dest='debug', help='Turn on debug mode')

args = arg_parser.parse_args()

if __name__ == '__main__':
    ip = args.ip
    port = args.port
    show_raw_response = args.raw
    debug = args.debug
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
    s.connect((ip, port))
    while True:
        print(f'{ip}:{port}> ', end='')
        inputcmd = input()
        argvs = process_commands(inputcmd)
        if debug:
          print(f'input arguments: {argvs}')
        if argvs is None:
            print('Invalid arguments')
            continue
        if len(inputcmd) == 1 and inputcmd[0] == 'q':
            break
        if len(argvs) == 1:
            data = gen_command(argvs[0], None, None)
        elif len(argvs) == 2:
            data = gen_command(argvs[0], argvs[1], [])
        else:
            data = gen_command(argvs[0], argvs[1], argvs[2:])
        toserver = bytes(data, encoding='utf8')
        s.sendall(toserver)
        recv_msg = s.recv(8192)
        reply_msg = decode_reply(recv_msg)
        if show_raw_response:
            print('Raw response = ', recv_msg)
        print(reply_msg)

    print('Goodbye.')
    s.close()
