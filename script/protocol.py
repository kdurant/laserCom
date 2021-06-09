import hashlib


# 如果子类定义了构造函数，而没有调用父类构造函数
# 将不具备父类的所有属性（包括使用到父类属性的父类方法）
class Protocol(object):

    def __init__(self):  #构造方法
        self.PROTOCAL_HEAD = b'\x01\x23\x45\x67\x89\xab\xcd\xef'
        self.PROTOCAL_TAIL = b'\x10\x32\x54\x76\x98\xba\xdc\xfe'
        self.MASTER_DEV: int = 0xa0
        self.SLAVE_DEV: int = 0x50
        print(type(self.SLAVE_DEV))

        self.HEART_BEAT: int = 0x10
        self.SET_FILE_INFO: int = 0x20
        self.RESPONSE_FILE_INFO: int = 0x21

        self.SET_FILE_DATA: int = 0x30
        self.RESPONSE_FILE_DATA: int = 0x31

        self.MASTER_ADDR_POS: int = 8
        self.MASTER_ADDR_LEN: int = 1
        self.SLAVE_ADDR_POS: int = 9
        self.SLAVE_ADDR_LEN: int = 1
        self.COMMAND_POS: int = 10
        self.COMMAND_LEN: int = 1
        self.DATA_LEN_POS: int = 11
        self.DATA_LEN_LEN: int = 4
        self.DATA_POS: int = 15

    def paserData(self, data):
        #print("receive data length : {0}".format(len(data)))
        result = b''
        result += self.PROTOCAL_HEAD

        command = self.getCommand(data)
        if command == self.HEART_BEAT:
            return data

        elif command == self.SET_FILE_INFO:
            result += self.SLAVE_DEV.to_bytes(1, byteorder='big')
            result += self.MASTER_DEV.to_bytes(1, byteorder='big')
            result += self.RESPONSE_FILE_INFO.to_bytes(1, byteorder='big')

            dataLen = self.getDataLen(data)
            dataField = self.getDataField(data)

            result += dataLen.to_bytes(4, byteorder='big')
            result += dataField

            md5 = hashlib.md5(result).digest()
            result += md5
            result += self.PROTOCAL_TAIL
            return result
        elif command == self.SET_FILE_DATA:
            result += self.SLAVE_DEV.to_bytes(1, byteorder='big')
            result += self.MASTER_DEV.to_bytes(1, byteorder='big')
            result += self.RESPONSE_FILE_DATA.to_bytes(1, byteorder='big')

            dataLen = self.getDataLen(data)
            dataField = self.getDataField(data)

            result += dataField[:15]
            result += b'\x11'

            print("result is : {}".format(result))
            md5 = hashlib.md5(result).digest()

            print("md5 is : {}".format(md5))
            result += md5
            result += self.PROTOCAL_TAIL

            return result
        else:
            return b'hello, world'

    def getCommand(self, data):
        command = data[self.COMMAND_POS:self.COMMAND_POS + self.COMMAND_LEN]
        s = int.from_bytes(command, byteorder='big')
        return s

    def getMasterAddr(self, data):
        return data[self.MASTER_ADDR_POS:self.MASTER_ADDR_POS + self.MASTER_ADDR_LEN]

    def getDataLen(self, data):
        b = data[self.DATA_LEN_POS:self.DATA_LEN_POS + self.DATA_LEN_LEN]
        return int.from_bytes(b, byteorder='big')

    def getDataField(self, data):
        len = self.getDataLen(data)
        return data[self.DATA_POS:self.DATA_POS + len]

    def encode(self, command):
        frame = b''
        frame += self.PROTOCAL_HEAD
        frame += self.MASTER_DEV.to_bytes(length=1, byteorder='big', signed=False)
        frame += self.SLAVE_DEV.to_bytes(length=1, byteorder='big', signed=False)

        if command == self.SET_FILE_INFO:
            frame += command.to_bytes(length=1, byteorder='big', signed=False)
            fileName = "test.txt"
            fileSize = 1024
            length = len(fileName) + 5
            frame += length.to_bytes(length=4, byteorder='big', signed=False)
            frame += bytes(fileName, encoding="utf8")
            frame += bytes('?', encoding="utf8")
            frame += fileSize.to_bytes(length=4, byteorder='big', signed=False)
        elif command == self.SET_FILE_DATA:
            # command
            frame += command.to_bytes(length=1, byteorder='big', signed=False)

            # data lenghth
            length = 4096 + 15
            frame += length.to_bytes(length=4, byteorder='big', signed=False)
            frame += bytes('?', encoding="utf8")
            """
            1.	文件被划分成文件块的总个数（4Byte）
            2.	当前传输的文件块序号，从0开始（4Byte）
            3.	当前传输文件块有效字节数（4Byte）
            4.	文件块具体内容
            """
            length = 1
            frame += length.to_bytes(length=4, byteorder='big', signed=False)
            frame += bytes('?', encoding="utf8")

            no = 0
            frame += no.to_bytes(length=4, byteorder='big', signed=False)
            frame += bytes('?', encoding="utf8")

            length = 4096
            frame += length.to_bytes(length=4, byteorder='big', signed=False)
            frame += bytes('?', encoding="utf8")

            frame += b'\x11' * 2048
            frame += b'\x22' * 2048

        md5 = hashlib.md5(frame).digest()
        frame += md5
        frame += self.PROTOCAL_TAIL
        return frame


protocol = Protocol()
