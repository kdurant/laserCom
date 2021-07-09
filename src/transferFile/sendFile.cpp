#include "sendFile.h"

bool SendFile::sendFileInfo(QString name, int repeatNum)
{
    int sendCnt = 0;

    QByteArray frame;
    frame.append(name.toLatin1());
    frame.append('?');

    QFile   file(name);
    quint32 size = file.size();
    frame.append(Common::int2ba(size));
    frame.append('?');

    frame.append(Common::int2ba(sendList[name].blockSize));

send_frame:
    emit sendDataReady(UserProtocol::SET_FILE_INFO, frame);

    QElapsedTimer time;
    time.start();
    while(time.elapsed() < 100)
    {
        QCoreApplication::processEvents();
    }

    if(recvData.size() == 0)
        emit errorDataReady("SET_FILE_INFO:没有收到接收机响应");
    else
    {
        if(frame == recvData)
        {
            qDebug() << "sendFileInfo() success:  " << sendCnt;
            return true;
        }
        else
            emit errorDataReady("SET_FILE_INFO:收到的接收机响应不正确");
    }
    qDebug() << "sendFileInfo() failed:  " << sendCnt;

    if(++sendCnt < repeatNum)
        goto send_frame;

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
int SendFile::splitData(QString name, QVector<QByteArray>& allFileBlock)
{
    QByteArray data;
    QFile      file(name);
    if(!file.exists())
        return -1;
    file.open(QIODevice::ReadOnly);

    sendList[name].fileSize        = file.size();
    sendList[name].fileBlockNumber = qCeil(sendList[name].fileSize / (qreal)sendList[name].blockSize);
    quint32    curretFileBlock     = 0;
    quint32    validLen            = 0;
    char*      buffer              = new char[sendList[name].blockSize];
    QByteArray blockData;
    QByteArray frame;
    QByteArray tmp;

    while(!file.atEnd())
    {
        frame.append(name.toLatin1());
        frame.append('?');

        frame.append(Common::int2ba(sendList[name].fileBlockNumber));  // 1.文件被划分成文件块的总个数（4Byte）
        frame.append('?');

        frame.append(Common::int2ba(curretFileBlock));  // 2.当前传输的文件块序号，从0开始（4Byte）
        frame.append('?');

        validLen = file.read(buffer, sendList[name].blockSize);
        frame.append(Common::int2ba(validLen));  // 3.当前传输文件块有效字节数（4Byte）
        frame.append('?');

        blockData = QByteArray::fromRawData(buffer, validLen);
        frame.append(blockData);  // 4.文件块具体内容

        allFileBlock.append(frame);
        frame.clear();
        curretFileBlock++;
    }
    delete[] buffer;
    return sendList[name].fileBlockNumber;
}

/**
 * @brief 发送文件，流程如下
 * 1.         sendFlow->setFileName(filePath);
 * 2.         sendFlow->setFileBlockSize(sysPara.blockSize);

 * @param blockInterval
 * @param fileInterval
 * @param repeatNum
 * @return
 */
bool SendFile::send(QString name, int blockInterval, int fileInterval, int repeatNum)
{
    if(sendFileInfo(name, repeatNum) == false)
        return false;

    int sendCnt = 0;

    // 2. 分割文件
    QVector<QByteArray> allFileBlock;
    int                 fileBlockNumber = splitData(name, allFileBlock);
    if(fileBlockNumber <= 0)
    {
        return false;
    }
    initBlockStatus(name);

    sendCnt = 0;
    do
    {
        qDebug() << "send No. " << sendCnt;
        for(int i = 0; i < fileBlockNumber; i++)
        {
            if(getBlockStatus(name, i) == false)
            {
                emit          sendDataReady(UserProtocol::SET_FILE_DATA, allFileBlock[i]);
                QElapsedTimer time;
                time.start();
                while(time.elapsed() < blockInterval)
                {
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
                }
                emit successBlockNumber(getBlockSuccessNumber(name));
                qDebug() << "sendFlow blockNo = " << i;
            }
        }
        sendCnt++;

        qDebug() << "start to wait response";
        if(fileInterval != 0)
        {
            QElapsedTimer time;
            time.start();
            while(time.elapsed() < fileInterval)
            {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            }
        }

        qDebug() << "sendFlow->getBlockSuccessNumber() = " << getBlockSuccessNumber(name);
        emit successBlockNumber(getBlockSuccessNumber(name));

    }
    // 只要不是每个块都成功接受，就一直重发，最多重发5个循环
    while(isSendAllBlock(name) == false && sendCnt < repeatNum);

    return true;
}
