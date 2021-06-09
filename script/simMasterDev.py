#-*- coding: UTF-8 -*-
"""
TCP server， 模拟发送端功能
实现：
"""

import argparse
import socket
import time
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

    #while True:
    time.sleep(2)

    data = protocol.encode(0x20)
    connection.send(data)
    time.sleep(0.5)

    data = protocol.encode(0x30)
    connection.send(data[:2048])
    time.sleep(0.5)
    connection.send(data[2048:])

    # break

connection.close()
