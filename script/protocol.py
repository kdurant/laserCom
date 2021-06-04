import hashlib


# 如果子类定义了构造函数，而没有调用父类构造函数
# 将不具备父类的所有属性（包括使用到父类属性的父类方法）
class Protocol(object):

    def __init__(self):  #构造方法
        self.PROTOCAL_HEAD = b'\x01\x23\x45\x67\x89\xab\xcd\xef'
        self.PROTOCAL_TAIL = b'\x10\x32\x54\x76\x98\xba\xdc\xfe'
        self.MASTER_DEV = 0xa0
        self.SLAVE_DEV = 0x50

        self.HEART_BEAT = 0x10
        self.SET_FILE_INFO = 0x20
        self.RESPONSE_FILE_INFO = 0x21

        self.SET_FILE_DATA = 0x30
        self.RESPONSE_FILE_DATA = 0x31

        self.MASTER_ADDR_POS = 8,
        self.MASTER_ADDR_LEN = 1,
        self.SLAVE_ADDR_POS = 9,
        self.SLAVE_ADDR_LEN = 1,
        self.COMMAND_POS = 10,
        self.COMMAND_LEN = 1,
        self.DATA_LEN_POS = 11,
        self.DATA_LEN_LEN = 4
        self.DATA_POS = 15,

        self.md5 = hashlib.md5()

    def paserData(self, data):
        result = b''
        result += self.PROTOCAL_HEAD
        
        if self.getCommand(data) == self.HEART_BEAT:
            return data
        

        elif self.getCommand(data) == self.SET_FILE_INFO:
            result += self.SLAVE_DEV.to_bytes(1, byteorder='big')
            result += self.MASTER_DEV.to_bytes(1, byteorder='big')
            result += self.RESPONSE_FILE_INFO.to_bytes(1, byteorder='big')

            dataLen = self.getDataLen(data)
            dataField = self.getDataField(data)

            result += dataLen.to_bytes(1, byteorder='big')
            result += dataField

            self.md5.update(result)
            result += self.md5.digest()
            result += self.PROTOCAL_TAIL
            return result

    def getCommand(self, data):
        return data[self.COMMAND_POS:self.COMMAND_POS + self.COMMAND_LEN]

    def getMasterAddr(self, data):
        return data[self.MASTER_ADDR_POS:self.MASTER_ADDR_POS + self.MASTER_ADDR_LEN]

    def getDataLen(self, data):
        b = data[self.DATA_LEN_POS:self.DATA_LEN_POS + self.DATA_LEN_LEN]
        return int.from_bytes(b, byteorder='big')

    def getDataField(self, data):
        len = self. getDataLen(data)
        return data[self.DATA_POS:self.DATA_POS + len]


protocol = Protocol()