#ifndef RECVFILE_H
#define RECVFILE_H

#include <algorithm>

#include <QtCore>
#include "common.h"
#include "protocol.h"
#include "ProtocolDispatch.h"

/**
* @brief 接收文件处理流程
* 1. 主程序接收到0x20命令后，会将文件相关信息发送给本模块.
* 2. 主程序发送0x21命令，发送端接收到响应就就会发送文件块数据
* 3. 文件块数据比较大，一般不可能在同一个TCP包中传输过来，所有使用paserNewData()获得完整的文件块包
* 4. processFileBlock() 得到有效数据及其他必要信息，进行写文件操作
*/
class RecvFile : public QObject
{
    Q_OBJECT
private:
    QString       fileName;
    int           fileSize;
    int           fileBlockNumber;  // 文件块的数量
    int           blockSize;
    QVector<bool> blockStatus;
    bool          isRecvNewData;  // 是否收到数据
    QByteArray    frameHead;
    QByteArray    frameTail;
    QByteArray    fileBlockData;

public:
    RecvFile() :
        fileSize(0), blockSize(0), isRecvNewData(false)
    {
        quint8 head[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
        quint8 hail[] = {0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe};

        frameHead = QByteArray((char *)head, 8);
        frameTail = QByteArray((char *)hail, 8);
    };

    enum RecvState
    {
        IDLE = 0,
        RECV_HEAD,
        RECV_DATA,
        RECV_TAIL,
    };

    RecvState state{IDLE};

    void setFileInfo(QByteArray &data)
    {
        fileName  = data.mid(0, data.indexOf('?'));
        fileSize  = Common::ba2int(data.mid(data.indexOf('?') + 1, 4));
        blockSize = Common::ba2int(data.mid(data.lastIndexOf('?') + 1, 4));

        fileBlockNumber = qCeil(fileSize / (qreal)blockSize);
        blockStatus.clear();
        for(int i = 0; i < fileBlockNumber; i++)
            blockStatus.append(false);
    }
    QString getFileName(void)
    {
        return fileName;
    }

    quint32 getBlockSize(void)
    {
        return blockSize;
    }

    void setBlockStatus(int index)
    {
        blockStatus[index] = true;
    }
    bool isRecvAllBlock(void)
    {
        bool status = std::all_of(blockStatus.begin(), blockStatus.end(), [](int i) {
            return i == true;
        });
        return status;
    }

    QByteArray packResponse(int blockNo, int validLen)
    {
        QByteArray frame;
        frame.append(Common::int2ba(fileBlockNumber));  // 1.文件被划分成文件块的总个数（4Byte）
        frame.append('?');

        frame.append(Common::int2ba(blockNo));  // 2.当前传输的文件块序号，从0开始（4Byte）
        frame.append('?');

        frame.append(Common::int2ba(validLen));  // 3.当前传输文件块有效字节数（4Byte）
        frame.append('?');

        frame.append(UserProtocol::SUCCESS);  // 4. 数据块接收成功

        return frame;
    }

    bool processFileBlock(QByteArray &data);

signals:
    void errorFileBlockReady(void);
    void fileBlockReady(quint32 blockNo, quint32 validLen, QByteArray &recvData);  // 收到正确的数据块信息
    void errorDataReady(QString &data);

public slots:
    void paserNewData(QByteArray &data);
};
#endif
