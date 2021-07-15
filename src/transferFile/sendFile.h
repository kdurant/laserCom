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

public:
    SendFile()
    {
    }

    struct FileInfo
    {
        QString       storePath;
        int           fileSize;
        int           fileBlockNumber;  // 文件块的数量
        int           blockSize;
        QVector<bool> blockStatus;
    };

    /**
     * @brief setFileName
     * 需要去掉文件信息中的路径，只保留文件名
     * @param name, 需要文件全名，包括路径
     */
    void setFileName(QString const &name)
    {
        if(name.size() > 128)
            emit errorDataReady("文件名长度超过128");
        int     index    = name.lastIndexOf('/');
        QString fileName = name.mid(index + 1);

        sendList[fileName].storePath = name;
    }

    /**
     * @brief setFileBlockSize
     * @param name, 只需要文件名
     * @param size
     */
    void setFileBlockSize(QString name, quint32 size)
    {
        sendList[name].blockSize = size;
    }

    /**
     * @brief sendFileInfo, 从接收机发送0x20命令
     * @return
     */
    bool sendFileInfo(QString name, int repeatNum);

    /**
     * @brief splitData, 将文件分割成指定大小的页，并按照格式打包
     * @param allFileBlock
     * @return 文件被分割的页数
     */
    int splitData(QString name, QVector<QByteArray> &allFileBlock);

    bool sendFileBlock(QByteArray &fileBlock);

    bool send(QString name, int blockInterval, int repeatNum);

    void initBlockStatus(QString name)
    {
        sendList[name].blockStatus.clear();

        for(int i = 0; i < sendList[name].fileBlockNumber; i++)
            sendList[name].blockStatus.append(false);
    }

    void setBlockStatus(QString name, int i, bool status)
    {
        if(i > sendList[name].blockStatus.size())
            return;
        sendList[name].blockStatus[i] = status;
    }
    bool getBlockStatus(QString name, int i)
    {
        if(i > sendList[name].blockStatus.size())
            return false;
        return sendList[name].blockStatus[i];
    }
    bool isSendAllBlock(QString name)
    {
        return std::all_of(sendList[name].blockStatus.begin(), sendList[name].blockStatus.end(), [](int i) {
            return i == true;
        });
    }

    int getBlockSuccessNumber(QString name)
    {
        return std::count(sendList[name].blockStatus.begin(), sendList[name].blockStatus.end(), true);
    }

private:
    QByteArray              recvData;
    QMap<QString, FileInfo> sendList;

signals:
    void sendDataReady(qint32 command, QByteArray &data);  // 需要发送的数据已经准备好
    void responseDataReady(void);                          // 接收到响应数据
    void successBlockNumber(int number);
    void errorDataReady(QString error);

    void receivedNewBlock();

public slots:
    void setNewData(QByteArray const &data)
    {
        recvData = data;
        emit responseDataReady();
    }

    void getNewBlock(void)
    {
        emit receivedNewBlock();
    }
};
#endif
