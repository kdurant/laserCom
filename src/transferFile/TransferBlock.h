#ifndef __TRANSFER_FILE_
#define __TRANSFER_FILE_

#include <QObject>
#include <QEventLoop>
#include <QCryptographicHash>
#include <QTcpSocket>

class TransferBlock : public QObject
{
private:
    QByteArray transferData;
    QChar      prefixChar;
    bool       isRecvNewData;
    QByteArray recvData;

public slots:
    void recvNewData(QByteArray& data);

public:
    TransferBlock() = default;

    static QByteArray intToByte(int number);

    static QByteArray generateChecksum(QByteArray& data, qint32 number);

    void setTransferData(QByteArray data)
    {
        this->transferData = data;
    };
    bool sendBlockData(void);
};

#endif
