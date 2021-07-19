#include "recvFile.h"

/**
 * @brief RecvFile::processFileBlock
 * 只有收到完全正确的文件块数据才会响应
 * @param data
 * @return
 */
bool RecvFile::processFileBlock(QByteArray &data)
{
    QByteArray head = Common::QString2QByteArray(FrameHead);
    QByteArray tail = Common::QString2QByteArray(FrameTail);

    int offset = 0;
    if(data.startsWith(head) && data.endsWith(tail))
    {
        quint32    len        = data.size();
        QByteArray validData  = data.mid(0, len - 24);
        QByteArray recv_md5   = data.mid(len - 24, 16);
        QByteArray expect_md5 = QCryptographicHash::hash(validData, QCryptographicHash::Md5);
        if(recv_md5 != expect_md5)
        {
            return false;
        }

        QByteArray fileBlock = ProtocolDispatch::getData(data);

        offset = fileBlock.indexOf('?');

        QString fileName = fileBlock.mid(0, offset);
        offset++;
        quint32    blockNo  = Common::ba2int(fileBlock.mid(offset + 5, 4));
        quint32    validLen = Common::ba2int(fileBlock.mid(offset + 10, 4));
        QByteArray recvData = fileBlock.mid(offset + 15, validLen);

        //        blockStatus[blockNo] = true;
        emit fileBlockReady(fileName, blockNo, validLen, recvData);
    }

    return true;
}

/**
 * @brief RecvFile::getFileBlock
 * 从数据流中，根据帧头和桢尾标志，截取出一个完整的文件块
 */
void RecvFile::paserNewData(QByteArray data)
{
    processFileBlock(data);
}
