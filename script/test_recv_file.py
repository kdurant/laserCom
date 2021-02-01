#-*- coding: UTF-8 -*-
import argparse
import socket
import time
import hashlib

parser = argparse.ArgumentParser(description="simple tcp server program")
parser.add_argument("--ip", default="127.0.0.1", help="tcp server ip address")
parser.add_argument("--port", default=17, type=int, help="tcp server port")
args = parser.parse_args()

send_file = b'hello,world\n'
checksum = b'\xff' * 16
checksum += b'\x00' * 4

fd = hashlib.md5()
fd.update(send_file)
checksum += fd.digest()

send_file += checksum * 3

server = socket.socket()
server.bind((args.ip, args.port))
server.listen(5)

while True:
    try:
        print("wait for new tcp client to connect...")
        connection, addr = server.accept()
        print('new client addr is : {0}'.format(addr))

        time.sleep(3)
        for i in range(3):
            print("send data...")
            connection.send(send_file)
            time.sleep(0.1)

        connection.close()
        break
    except KeyboardInterrupt:
        print("User stop to send data!")
        break
