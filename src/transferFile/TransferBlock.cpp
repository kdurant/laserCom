#include "TransferBlock.h"

void TransferBlock::recvNewData(QByteArray& data)
{
    isRecvNewData = true;
    recvData      = data;
}

QByteArray TransferBlock::intToByte(int number)
{
    QByteArray abyte0;
    abyte0.resize(4);
    abyte0[0] = (uchar)(0x000000ff & number);
    abyte0[1] = (uchar)((0x0000ff00 & number) >> 8);
    abyte0[2] = (uchar)((0x00ff0000 & number) >> 16);
    abyte0[3] = (uchar)((0xff000000 & number) >> 24);
    return abyte0;
}

QByteArray TransferBlock::generateChecksum(QByteArray& data, qint32 number)
{
    QByteArray res;
    res.append(16, 0xfe);
    res.append(TransferBlock::intToByte(number));

    res.append(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex());
    return res.append(res.append(res));
}

bool TransferBlock::transfer(){

};
