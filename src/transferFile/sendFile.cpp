#include "sendFile.h"

bool SendFile::sendFileInfo(QString name, int repeatNum, char mode)
{
    quint32 size = QFileInfo(sendList[name].storePath).size();
    if(size == 0)
        return false;

    int sendCnt = 0;

    // 1. 文件名
    QByteArray frame;
    frame.append(name.toLatin1());
    frame.append('?');

    // 2. 文件大小
    frame.append(Common::int2ba(size));
    frame.append('?');

    // 3. 默认文件块大小
    frame.append(Common::int2ba(sendList[name].blockSize));

    // 4. 是否需要响应
    frame.append(mode);
    frame.append('?');

send_frame:
    qDebug() << name << ": sendFileInfo()"
             << ", repeatNum = " << repeatNum
             << ", sendCnt = " << sendCnt;
    emit sendDataReady(UserProtocol::SET_FILE_INFO, frame);

    if(mode == 'n')
        return true;

    QEventLoop waitLoop;  // 等待响应数据，或者1000ms超时
    connect(this, &SendFile::responseDataReady, &waitLoop, &QEventLoop::quit);

    QTimer timer;
    timer.start(500);

    connect(&timer, &QTimer::timeout, this, [&]() {
        if(waitLoop.isRunning())
        {
            timeOutFlag = true;
            waitLoop.quit();
        }
    });
    waitLoop.exec();
    timer.stop();

    if(timeOutFlag)
    {
        qDebug() << "sendFileInfo() timeout";
        timeOutFlag = false;
    }

    if(frame == recvData)
    {
        qDebug() << name << "sendFileInfo() success:  " << sendCnt;
        return true;
    }
    else
    {
        qDebug() << name << ": sendFileInfo() __ recvData.size()" << recvData.size()
                 << "\nsendFileInfo() failed:  " << sendCnt;
    }

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

    QFile file(sendList[name].storePath);

    if(!file.exists())
        return -1;
    if(file.size() == 0)
        return -1;

    file.open(QIODevice::ReadOnly);

    sendList[name].fileSize        = file.size();
    sendList[name].fileBlockNumber = qCeil(sendList[name].fileSize / (qreal)sendList[name].blockSize);
    if(sendList[name].fileBlockNumber <= 0)
    {
        qDebug() << "file.size() = " << file.size();
        file.close();
        return -1;
    }

    quint32    curretFileBlock = 0;
    quint32    validLen        = 0;
    char*      buffer          = new char[sendList[name].blockSize];
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
 *
 * @param name, 这里只需要指定文件名，不需要全部路径
 * @param blockInterval
 * @param repeatNum
 * @param mode,
 * @return
 */
bool SendFile::send(QString name, int blockInterval, int repeatNum, char mode)
{
    qDebug() << "---------------the start of sending file";
    int sendCnt = 0;
    // 2. 分割文件
    QVector<QByteArray> allFileBlock;
    int                 fileBlockNumber = splitData(name, allFileBlock);
    if(fileBlockNumber <= 0)
    {
        qDebug() << "fileBlockNumber = -1";
        return false;
    }
    initBlockStatus(name);

    if(sendFileInfo(name, repeatNum, mode) == false)
    {
        qDebug() << "sendFileInfo() failed";
        return false;
    }

    sendCnt = 0;
    if(mode == 'n')
    {
        for(int i = 0; i < fileBlockNumber; i++)
        {
            qDebug() << "sendCnt =  " << sendCnt << "; i = " << i;
            emit sendDataReady(UserProtocol::SET_FILE_DATA, allFileBlock[i]);
        }
        return true;
    }

    for(int i = 0, sendCnt = 0; i < fileBlockNumber && sendCnt < repeatNum; i++)
    {
        if(getBlockStatus(name, i) == false)
        {
            qDebug() << "sendCnt =  " << sendCnt << "; i = " << i;
            emit sendDataReady(UserProtocol::SET_FILE_DATA, allFileBlock[i]);

            QEventLoop waitLoop;  // 正常情况下大概需要30ms
            connect(this, &SendFile::receivedNewBlock, &waitLoop, &QEventLoop::quit);
            QTimer::singleShot(blockInterval, &waitLoop, &QEventLoop::quit);
            waitLoop.exec();

            emit successBlockNumber(getBlockSuccessNumber(name));
        }

        if(i == fileBlockNumber - 1)
        {
            qDebug("Round %d has sent over!", sendCnt);
            if(mode == 'n')
                break;

            qDebug() << "sendCnt++";
            i = -1;
            sendCnt++;
        }

        if(isSendAllBlock(name))
            break;
    }
    emit successBlockNumber(getBlockSuccessNumber(name));
    qDebug() << "---------------the end of sending file";

    return true;
}
