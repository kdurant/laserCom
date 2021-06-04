#ifndef RECVFILE_H
#define RECVFILE_H
#include <QtCore>
#include "common.h"
#include "protocol.h"
#include "ProtocolDispatch.h"

class RecvFile : public QObject
{
    Q_OBJECT
private:
    bool       isRecvNewData;  // 是否收到数据
    QByteArray frameHead;
    QByteArray frameTail;
    QByteArray fileBlockData;

public:
    RecvFile()
    {
        isRecvNewData = false;
        quint8 head[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
        quint8 hail[] = {0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe};

        QByteArray frameHead = QByteArray((char *)head, 8);
        QByteArray frameTail = QByteArray((char *)hail, 8);
    };

    enum RecvState
    {
        IDLE = 0,
        RECV_HEAD,
        RECV_DATA,
        RECV_TAIL,
    };

    RecvState state{IDLE};

    bool processFileBlock(QByteArray &data);

signals:
    void errorFileBlockReady(void);

public slots:
    void paserFileBlock(QByteArray &data);
};
#endif
