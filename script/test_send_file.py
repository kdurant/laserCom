#-*- coding: UTF-8 -*-
import argparse
import socket
import time

parser = argparse.ArgumentParser(description="simple tcp server program")
parser.add_argument("--ip", default="127.0.0.1", help="tcp server ip address")
parser.add_argument("--port", default=17, type=int, help="tcp server port")
args = parser.parse_args()

server = socket.socket()
server.bind((args.ip, args.port))
server.listen(5)

while True:
    print("wait for new tcp client to connect...")
    connection, addr = server.accept()
    print('new client addr is : {0}'.format(addr))

    #time.sleep(3)
    for i in range(50000000):
        connection.send(b"n" * 238)
        time.sleep(0.1)

    connection.close()
