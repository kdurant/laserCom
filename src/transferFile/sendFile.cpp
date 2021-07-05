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

    // QEventLoop waitLoop;  // 等待响应数据，或者1000ms超时
    // connect(this, &SendFile::responseDataReady, &waitLoop, &QEventLoop::quit);
    // QTimer::singleShot(1000, &waitLoop, &QEventLoop::quit);
    // waitLoop.exec();
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
        return -1;
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

/**
 * @brief 发送文件，流程如下
 * 1.         sendFlow->setFileName(filePath);
 * 2.         sendFlow->setFileBlockSize(sysPara.blockSize);

 * @param blockInterval
 * @param fileInterval
 * @param repeatNum
 * @return
 */
bool SendFile::send(int blockInterval, int fileInterval, int repeatNum)
{
    int sendCnt = 0;

    // 1. 发送文件信息
    while(sendCnt < repeatNum)
    {
        if(sendFileInfo())
        {
            qDebug() << "sendFileInfo() success:  " << sendCnt;
            break;
        }
        else
        {
            qDebug() << "sendFileInfo() failed:  " << sendCnt;
            sendCnt++;
        }
    }
    if(sendCnt == repeatNum)
    {
        return false;
    }

    // 2. 分割文件
    QVector<QByteArray> allFileBlock;
    int                 fileBlockNumber = splitData(allFileBlock);
    if(fileBlockNumber <= 0)
    {
        return false;
    }
    initBlockStatus();

    sendCnt = 0;
    do
    {
        qDebug() << "send No. " << sendCnt;
        for(int i = 0; i < fileBlockNumber; i++)
        {
            if(getBlockStatus(i) == false)
            {
                emit          sendDataReady(UserProtocol::SET_FILE_DATA, allFileBlock[i]);
                QElapsedTimer time;
                time.start();
                while(time.elapsed() < blockInterval)
                {
                    QCoreApplication::processEvents();
                }
                emit successBlockNumber(getBlockSuccessNumber());
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
                QCoreApplication::processEvents();
            }
        }

        qDebug() << "sendFlow->getBlockSuccessNumber() = " << getBlockSuccessNumber();
        emit successBlockNumber(getBlockSuccessNumber());

    }
    // 只要不是每个块都成功接受，就一直重发，最多重发5个循环
    while(isSendAllBlock() == false && sendCnt < repeatNum);

    return true;
}
