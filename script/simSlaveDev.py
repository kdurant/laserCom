#-*- coding: UTF-8 -*-
"""
TCP server， 模拟接收机功能
实现：
1. 对0x20命令的响应
"""

import argparse
import socket
from protocol import Protocol

parser = argparse.ArgumentParser(description="simple tcp server program")
parser.add_argument("--ip", default="127.0.0.1", help="tcp server ip address")
parser.add_argument("--port", default=17, type=int, help="tcp server port")
args = parser.parse_args()

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

connection.close()
