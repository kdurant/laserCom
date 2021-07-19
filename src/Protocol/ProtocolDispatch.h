#ifndef PROTOCOL_DISPATCH_H
#define PROTOCOL_DISPATCH_H

#include <QtCore>
#include "common.h"
#include "protocol.h"

class FrameField
{
public:
    enum fieldOffset
    {
        MASTER_ADDR_POS = 8,
        MASTER_ADDR_LEN = 1,
        SLAVE_ADDR_POS  = 9,
        SLAVE_ADDR_LEN  = 1,

        COMMAND_POS = 10,
        COMMAND_LEN = 1,

        DATA_POS = 11,
        DATA_LEN = 4
    };
};

/**
 * @brief 功能如下：
 * 1. 解析数据帧的命令字段内容，发射相应的信号给具体的处理模块
 * 2. 提供一个槽函数，需要按照协议打包数据的模块，可以通过信号将数据发送进来
 * 3. 数据打包后，发送信号，通知UDP将数据发送出去
 */
class ProtocolDispatch : public QObject
{
    Q_OBJECT

public:
    ProtocolDispatch()
    {
        //        cmdNum = 0;
    }
    ~ProtocolDispatch()
    {
    }
    void processCommand(QByteArray &frame);

    static uint32_t getCommand(QByteArray &data)
    {
        return data.mid(FrameField::COMMAND_POS, FrameField::COMMAND_LEN).toHex().toUInt(nullptr, 16);
    }

    static uint8_t getMasterAddr(QByteArray &data)
    {
        return data.mid(FrameField::MASTER_ADDR_POS, FrameField::MASTER_ADDR_LEN).toHex().toUInt(nullptr, 16);
    }
    static uint8_t getSlaveAddr(QByteArray &data)
    {
        return data.mid(FrameField::SLAVE_ADDR_POS, FrameField::SLAVE_ADDR_LEN).toHex().toUInt(nullptr, 16);
    }

    static uint32_t getDataLen(QByteArray &data)
    {
        return data.mid(FrameField::DATA_POS, FrameField::DATA_LEN).toHex().toUInt(nullptr, 16);
    }

    /**
    * @brief 根据协议，获得数据区的全部数据
    *
    * @param data, 协议帧的全部数据
    *
    * @return 
    */
    static QByteArray getData(QByteArray &data)
    {
        quint32    data_len = getDataLen(data);
        QByteArray frame;
        frame = data.mid(FrameField::DATA_POS + FrameField::DATA_LEN, data_len);
        return frame;
    }

    void setNewData(QByteArray &data)
    {
        emit readyRead(data);
    }

public slots:
    void encode(qint32 command, QByteArray &data);
    void parserFrame(QByteArray &data);

signals:
    void readyRead(QByteArray &data);
    /**
     * @brief 协议相关的信号
     */
    void heartBeatReady(quint32 number);
    void slaveFileInfoReady(QByteArray &data);    // 从机收到主机设置的文件信息
    void masterFileInfoReady(QByteArray &data);   // 主机收到从机对文件信息的应答
    void slaveFileBlockReady(QByteArray &data);   // 从机收到主机设置的文件块数据
    void masterFileBlockReady(QByteArray &data);  // 主机收到从机对文件数据块的应答

    void frameDataReady(QByteArray &data);
    void errorDataReady(QString &data);

private:
    QVector<quint8>  head;
    static quint32   cmdNum;
    quint32          cmdData;
    quint32          packetNum{0};
    quint32          validDataLen;
    QVector<quint16> data;
    quint32          checksum;
    QByteArray       frame;

    QString deviceVersion;
};
#endif
