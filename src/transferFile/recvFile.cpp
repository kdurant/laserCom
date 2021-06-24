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
        //        quint32    blockTotal = Common::ba2int(fileBlock.mid(0, 4));
        quint32    blockNo  = Common::ba2int(fileBlock.mid(5, 4));
        quint32    validLen = Common::ba2int(fileBlock.mid(10, 4));
        QByteArray recvData = fileBlock.mid(15, validLen);

        blockStatus[blockNo] = true;
        emit fileBlockReady(blockNo, validLen, recvData);
    }

    return true;
}

/**
 * @brief RecvFile::getFileBlock
 * 从数据流中，根据帧头和桢尾标志，截取出一个完整的文件块
 */
void RecvFile::paserNewData(QByteArray &data)
{
    QByteArray frame;
    fileBlockData.append(data);
    int headOffset = -1;
    int tailOffset = -1;

    headOffset = fileBlockData.indexOf(frameHead);
    tailOffset = fileBlockData.indexOf(frameTail);

    if(headOffset == -1 || tailOffset == -1)
    {
        // 文件块很大，一次TCP数据里没有帧头和帧尾
        if(headOffset == -1 && tailOffset == -1)
        {
            // 等下一次进来数据再接着处理
            return;
        }
        else if(headOffset == -1 && tailOffset > -1)
        {  // 没有帧头，但有帧尾，说明帧头数据丢失
            fileBlockData = fileBlockData.mid(tailOffset + 8);
        }
        else if(headOffset > -1 && tailOffset == -1)
        {  // 文件块数据的开始
            // 等下一次进来数据再接着处理
            return;
        }
    }
    else
    {
        if(headOffset == 0)
        {
            frame         = fileBlockData.mid(0, tailOffset + 8);
            fileBlockData = fileBlockData.mid(tailOffset + 8);
            processFileBlock(frame);
        }
        else
            qDebug() << "会出现吗?";
    }
}
