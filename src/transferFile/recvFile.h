#ifndef RECVFILE_H
#define RECVFILE_H
#include <QtCore>

class RecvFile : public QObject
{
    Q_OBJECT
private:
    bool isRecvNewData;  // 是否收到数据
public:
    RecvFile()
    {
        isRecvNewData = false;
    };

    enum RecvState
    {
        IDLE = 0,
        RECV_HEAD,
        RECV_DATA,
        RECV_TAIL,
    };

    RecvState state{IDLE};

signals:
    void errorFileBlockReady(void);

public slots:
    void setNewData(QByteArray &data)
    {
        isRecvNewData = true;
    }
};
#endif
