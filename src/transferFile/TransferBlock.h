#ifndef __TRANSFER_FILE_
#define __TRANSFER_FILE_

#include <QObject>
class TransferBlock : public QObject
{
private:
    QString transferData;
    QChar   prefixChar;
    bool    isRecvData;

public slots:
    void recvNewData(QString& data);

public:
    TransferBlock() = default;

    void setTransferData(QString data)
    {
        this->transferData = data;
    };
    bool transfer(void);
};

#endif
