#ifndef SENDFILE_H
#define SENDFILE_H
#include <QtCore>
#include <common.h>
#include "protocol.h"
#include "ProtocolDispatch.h"

class SendFile : public QObject
{
    Q_OBJECT
private:
    QString    fileName;
    quint32    blockSize;
    QByteArray recvData;

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

    bool sendFileInfo(void);
    void splitData(QVector<QByteArray> &allFileBlock);

    bool send(void);

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
