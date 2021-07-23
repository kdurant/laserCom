#ifndef RECVFILE_H
#define RECVFILE_H

#include <algorithm>

#include <QtCore>
#include <QtMath>
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
public:
    RecvFile() :
        isRecvNewData(false)
    {
        quint8 head[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
        quint8 hail[] = {0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe};

        frameHead = QByteArray((char *)head, 8);
        frameTail = QByteArray((char *)hail, 8);
    };

    struct FileInfo
    {
        char          mode;
        QString       storePath;  // 文件实际路径
        int           fileSize;
        int           fileBlockNumber;  // 文件块的数量
        int           blockSize;
        QVector<bool> blockStatus;
        QFile *       file;
    };

    enum RecvState
    {
        IDLE = 0,
        RECV_HEAD,
        RECV_DATA,
        RECV_TAIL,
    };

    RecvState state{IDLE};

    QMap<QString, FileInfo> recvList;

    void setFileInfo(QByteArray &data)
    {
        int offset = data.indexOf('?');
        fileName   = data.mid(0, offset);

        offset++;
        int fileSize = Common::ba2int(data.mid(offset, 4));
        offset += 4;

        offset++;
        int blockSize = Common::ba2int(data.mid(offset, 4));
        offset += 4;

        char mode = data.mid(offset, 1)[0];

        recvList[fileName].storePath       = "cache/slave/" + fileName;
        recvList[fileName].fileSize        = fileSize;
        recvList[fileName].blockSize       = blockSize;
        recvList[fileName].fileBlockNumber = qCeil(fileSize / (qreal)blockSize);
        recvList[fileName].mode            = mode;

        qInfo() << fileName << ": is created."
                << " mode = " << mode;
        recvList[fileName].blockStatus.clear();
        for(int i = 0; i < recvList[fileName].fileBlockNumber; i++)
            recvList[fileName].blockStatus.append(false);
        recvList[fileName].file = new QFile();
        recvList[fileName].file->setFileName(recvList[fileName].storePath);
        recvList[fileName].file->open(QIODevice::WriteOnly);
    }
    /**
    * @brief 本次接受到的文件名
    *
    * @return 
    */
    QString getFileName(void)
    {
        return fileName;
    }

    QString getStorePath(QString name)
    {
        return recvList[name].storePath;
    }

    quint32 getBlockSize(QString name)
    {
        return recvList[name].blockSize;
    }

    void setBlockStatus(QString name, int index)
    {
        if(index >= recvList[name].blockStatus.size())
        {
            qInfo() << "setBlockStatus(): index = " << index
                    << ": vector size = " << recvList[name].blockStatus.size();
        }
        recvList[name].blockStatus[index] = true;
    }

    bool getBlockStatus(QString name, int index)
    {
        if(index >= recvList[name].blockStatus.size())
        {
            qInfo() << "getBlockStatus() : index = " << index
                    << ": vector size = " << recvList[name].blockStatus.size();
            return false;
        }
        return recvList[name].blockStatus[index];
    }

    /**
    * @brief 获得所有成功文件块的个数
    *
    * @param name
    *
    * @return 
    */
    int getBlockSuccessNumber(QString name)
    {
        return std::count(recvList[name].blockStatus.begin(), recvList[name].blockStatus.end(), true);
    }

    int getAllBlockNumber(QString name)
    {
        return recvList[name].blockStatus.size();
    }

    bool isRecvAllBlock(QString name)
    {
        bool status = std::all_of(recvList[name].blockStatus.begin(), recvList[name].blockStatus.end(), [](int i) {
            return i == true;
        });
        return status;
    }

    bool eraseFileNode(QString name)
    {
        auto node = recvList.find(name);
        recvList.erase(node);
        return true;
    }

    QByteArray packResponse(QString name, int blockNo, int validLen)
    {
        QByteArray frame;

        frame.append(name.toLatin1());
        frame.append('?');

        frame.append(Common::int2ba(recvList[name].fileBlockNumber));  // 1.文件被划分成文件块的总个数（4Byte）
        frame.append('?');

        frame.append(Common::int2ba(blockNo));  // 2.当前传输的文件块序号，从0开始（4Byte）
        frame.append('?');

        frame.append(Common::int2ba(validLen));  // 3.当前传输文件块有效字节数（4Byte）
        frame.append('?');

        frame.append(UserProtocol::SUCCESS);  // 4. 数据块接收成功

        //        frame.append('?');
        //        frame.append(1390, 0x33);

        return frame;
    }

    bool processFileBlock(QByteArray &data);

signals:
    void errorFileBlockReady(void);
    void fileBlockReady(QString fileName, quint32 blockNo, quint32 validLen, QByteArray &recvData);  // 收到正确的数据块信息
    void errorDataReady(QString &data);

public slots:
    void paserNewData(QByteArray data);

private:
    QString    fileName;
    bool       isRecvNewData;  // 是否收到数据
    QByteArray frameHead;
    QByteArray frameTail;
    QByteArray fileBlockData;
};
#endif
