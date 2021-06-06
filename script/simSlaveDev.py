#-*- coding: UTF-8 -*-
"""
TCP server， 模拟接收机功能
实现：
1. 对0x20命令的响应
"""

import argparse
import socket
import hashlib
from protocol import Protocol

parser = argparse.ArgumentParser(description="simple tcp server program")
parser.add_argument("--ip", default="127.0.0.1", help="tcp server ip address")
parser.add_argument("--port", default=17, type=int, help="tcp server port")
args = parser.parse_args()

FRAME_HEAD = b'\x01\x23\x45\x67\x89\xab\xcd\xef'
FRAME_TAIL = b'\x10\x32\x54\x76\x98\xba\xdc\xfe'
m = hashlib.md5()
SET_FILE_INFO = 0x20
RESPONSE_FILE_INFO = b'\x21'

server = socket.socket()
server.bind((args.ip, args.port))
server.listen(5)

protocol = Protocol()

while True:
    print("wait for new tcp client to connect...")
    connection, addr = server.accept()
    print('new client addr is : {0}'.format(addr))

    while True:
        data = connection.recv(102400)

        response = protocol.paserData(data)
        connection.send(response)

        # if data[10] == SET_FILE_INFO:
        #
        #     needCheckSum = data[:10]
        #     needCheckSum += RESPONSE_FILE_INFO
        #     needCheckSum += data[11:]
        #
        #     needCheckSum = needCheckSum[:-24]
        #     m.update(needCheckSum)
        #     needCheckSum += m.digest()
        #     needCheckSum += FRAME_TAIL
        #     connection.send(needCheckSum)

connection.close()
