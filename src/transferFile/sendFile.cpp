#include "sendFile.h"

bool SendFile::sendFileInfo(void)
{
    QByteArray frame;
    frame.append(fileName.toLatin1());
    frame.append('?');

    QFile   file(fileName);
    quint32 size = file.size();
    frame.append(Common::int2ba(size));
    frame.append('?');

    frame.append(Common::int2ba(blockSize));

    emit sendDataReady(UserProtocol::SET_FILE_INFO, frame);

    QEventLoop waitLoop;  // 等待响应数据，或者1000ms超时
    connect(this, &SendFile::responseDataReady, &waitLoop, &QEventLoop::quit);
    QTimer::singleShot(100, &waitLoop, &QEventLoop::quit);
    waitLoop.exec();
    if(recvData.size() == 0)
        emit errorDataReady("SET_FILE_INFO:没有收到接收机响应");
    else
    {
        if(frame == recvData)
            return true;
        else
            emit errorDataReady("SET_FILE_INFO:收到的接收机响应不正确");
    }

    return false;
}

/**
 * @brief SendFile::splitData
 * 按照0x30命令中的数据段，打包好文件块数据
 * 1.文件被划分成文件块的总个数（4Byte）
 * 2.当前传输的文件块序号，从0开始（4Byte）
 * 3.当前传输文件块有效字节数（4Byte）
 * 4.文件块具体内容
 */
int SendFile::splitData(QVector<QByteArray>& allFileBlock)
{
    QByteArray data;
    QFile      file(fileName);
    if(!file.exists())
        return 0;
    file.open(QIODevice::ReadOnly);

    fileSize                   = file.size();
    fileBlockNumber            = qCeil(fileSize / (qreal)blockSize);
    quint32    curretFileBlock = 0;
    quint32    validLen        = 0;
    char*      buffer          = new char[blockSize];
    QByteArray blockData;
    QByteArray frame;
    QByteArray tmp;

    while(!file.atEnd())
    {
        frame.append(Common::int2ba(fileBlockNumber));  // 1.文件被划分成文件块的总个数（4Byte）
        frame.append('?');

        frame.append(Common::int2ba(curretFileBlock));  // 2.当前传输的文件块序号，从0开始（4Byte）
        frame.append('?');

        validLen = file.read(buffer, blockSize);
        frame.append(Common::int2ba(validLen));  // 3.当前传输文件块有效字节数（4Byte）
        frame.append('?');

        blockData = QByteArray::fromRawData(buffer, validLen);
        frame.append(blockData);  // 4.文件块具体内容

        allFileBlock.append(frame);
        frame.clear();
        curretFileBlock++;
    }
    return fileBlockNumber;
}

bool SendFile::send(void)
{
    QByteArray frame;
    QEventLoop waitLoop;
    QTimer::singleShot(100, &waitLoop, &QEventLoop::quit);

    return true;
}
