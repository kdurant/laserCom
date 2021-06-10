#ifndef SENDFILE_H
#define SENDFILE_H
#include <algorithm>
#include <QtCore>
#include <common.h>
#include "protocol.h"
#include "ProtocolDispatch.h"

class SendFile : public QObject
{
    Q_OBJECT
private:
    QString       fileName;
    quint32       fileSize;
    quint32       blockSize;
    quint32       fileBlockNumber;
    QByteArray    recvData;
    QVector<bool> blockStatus;

public:
    SendFile() :
        fileName(""), blockSize(0)
    {
    }
    void setFileName(QString const &name)
    {
        if(name.size() > 128)
            emit errorDataReady("文件名长度超过128");
        fileName = name;
    }

    void setFileBlockSize(quint32 size)
    {
        blockSize = size;
    }

    /**
     * @brief sendFileInfo, 从接收机发送0x20命令
     * @return
     */
    bool sendFileInfo(void);

    /**
     * @brief splitData, 将文件分割成指定大小的页，并按照格式打包
     * @param allFileBlock
     * @return 文件被分割的页数
     */
    int splitData(QVector<QByteArray> &allFileBlock);

    bool sendFileBlock(QByteArray &fileBlock);

    bool send(void);

    void initBlockStatus(void)
    {
        blockStatus.clear();

        for(int i = 0; i < fileBlockNumber; i++)
            blockStatus.append(false);
    }

    void setBlockStatus(int i, bool status)
    {
        if(i > blockStatus.size())
            return;
        blockStatus[i] = status;
    }
    bool getBlockStatus(int i)
    {
        if(i > blockStatus.size())
            return false;
        return blockStatus[i];
    }
    bool isSendAllBlock(void)
    {
        return std::all_of(blockStatus.begin(), blockStatus.end(), [](int i) {
            return i == true;
        });
    }

signals:
    void sendDataReady(qint32 command, QByteArray &data);  // 需要发送的数据已经准备好
    void responseDataReady(void);                          // 接收到响应数据
    void errorDataReady(QString error);

public slots:
    void setNewData(QByteArray const &data)
    {
        recvData = data;
        emit responseDataReady();
    }
};
#endif
