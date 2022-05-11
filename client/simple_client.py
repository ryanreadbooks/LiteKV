import argparse
import random
import socket
from time import sleep

from cv2 import add


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


def generate_pesudo_cmds(n):
    kcan = '0123456789abcdefg'
    def random_str(a, b):
        return kcan[random.randint(0, 16)] * random.randint(0, a) + kcan[random.randint(0, 16)] * random.randint(0, b)
    ret = None
    cmds = []
    for i in range(n):
        option = random.randint(1, 4)
        key = random_str(8, 3)
        if option == 1:
            # int
            get_or_set = random.randint(0, 1)
            if get_or_set == 0:
                # get
                ret = ['get', key]
            else:
                # set
                ret = ['set', key, str(random.randint(10, 10000))]
        elif option == 2:
            # string
            get_or_set = random.randint(0, 1)
            if get_or_set == 0:
                # get
                ret = ['get', key]
            else:
                # set
                val = random_str(5, 5)
                ret = ['set', key, val]
        elif option == 3:
            # list
            list_op = random.randint(0, 3)
            if list_op == 0:
                # lpush
                ret = ['lpush', key]
                vals = [str(i) for i in range(random.randint(1, 8))]
                ret.extend(vals)
            elif list_op == 1:
                # rpush
                ret = ['rpush', key]
                vals = [str(i) for i in range(random.randint(1, 8))]
                ret.extend(vals)
            elif list_op == 2:
                # lpop
                ret = ['lpop', key]
            else:
                # rpop
                ret = ['rpop', key]
        else:
            # hash
            hop = random.randint(0, 7)
            field = random_str(2, 6)
            if hop == 0:
                n_pair = random.randint(1, 10)
                fvs = []
                for j in range(n_pair):
                    fvs.append(random_str(2, 5))
                    fvs.append(random_str(2, 5))
                ret = ['hset', key, *fvs]
            elif hop == 1:
                # get
                ret = ['hget', key, field]
            elif hop == 2:
                # del
                ret = ['hdel', key, field]    
            elif hop == 3:
                # exists
                ret = ['hexists', key, field]
            elif hop == 4:
                # getall
                ret = ['hgetall', key]
            elif hop == 5:
                # keys
                ret = ['hkeys', key]
            elif hop == 6:
                # vals
                ret = ['hvals', key]
            else:
                # len
                ret = ['hlen', key]

        cmds.append(ret)

    return cmds



arg_parser = argparse.ArgumentParser(description='LiteKV simple client')
arg_parser.add_argument('-a', '--address', type=str, default='127.0.0.1', dest='ip', help='The ip address of the database')
arg_parser.add_argument('-p', '--port', type=int, default=9527, dest='port', help='The port of the database')
arg_parser.add_argument('-r', '--raw', action='store_true', default=False, dest='raw', help='Show raw response from server')
arg_parser.add_argument('-d', '--debug', action='store_true', default=False, dest='debug', help='Turn on debug mode')
arg_parser.add_argument('-f', '--flood', action='store_true', default=False, dest='flood', help='Send massive commands to server. Turn on flood mode')
arg_parser.add_argument('-n', '--n-cmds', type=int, default=1000, dest='ncmds', help='Number of commands per request sending to server when in flood mode')
arg_parser.add_argument('-q', '--n-request', type=int, default=10000, dest='nreq', help='Number of total requests when in flood mode')

args = arg_parser.parse_args()

if __name__ == '__main__':
    ip = args.ip
    port = args.port
    show_raw_response = args.raw
    debug = args.debug
    flood_mode = args.flood
    n_cmds = args.ncmds
    n_req = args.nreq
    if not flood_mode:
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
    else:
        # flood the server
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
        s.connect((ip, port))
        print(f'Connected to server ({ip}, {port})')
        print(f'{n_req} requests are performing, every request has {n_cmds} commands in it... ')
        # random generate commands
        total_bytes = 0
        for i in range(n_req):
            pesudo_cmds = generate_pesudo_cmds(n=n_cmds)    # List[List]
            data = ''
            if debug:
                print(f'len of commands in this request = {len(pesudo_cmds)}')
            for argvs in pesudo_cmds:
                if len(argvs) == 1:
                    data += gen_command(argvs[0], None, None)
                elif len(argvs) == 2:
                    data += gen_command(argvs[0], argvs[1], [])
                else:
                    data += gen_command(argvs[0], argvs[1], argvs[2:])
                if debug:
                    print(argvs)
            toserver = bytes(data, encoding='utf8')
            total_bytes += len(toserver)
            if debug:
                print(f'total bytes = {len(toserver)}')
            # print(toserver)
            s.sendall(toserver)
            sleep(0.0001)
        print(f'Total bytes sent = {total_bytes}')
        print('Done')
        sleep(8)
        s.close()